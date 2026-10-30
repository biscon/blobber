"""Serial, explicitly invoked experiments. No import/build/test-triggered playthroughs."""
import copy
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys
import time

from headless_client import Client, Cancelled
from .config import MIB, canonical, code_identity, digest, file_digest, read_json, trials
from .policies import coverage
from .report import atomic_json, compact, keep_traces, rows, write_reports
from .trial import run_trial, validate_observation

RESERVE = (256+16+16)*MIB  # One whole trace, sidecar and final journal/report margin.


def preflight(config, runner, assets, *, deadline=None, cancelled=None):
    checks = []
    limits = config["limits"]
    with Client(runner, assets, config["mode"], timeout=limits["request_seconds"],
                startup_timeout=limits["startup_seconds"], deadline=deadline, cancelled=cancelled) as client:
        for p in config["parties"]:
            for r in config["rosters"]:
                scenario = dict(p["scenario"], monsters=r["monsters"], seed=config["seeds"][0], name="Experiment validation")
                response = client.request("start", scenario=scenario)
                if not response["ok"]:
                    checks.append(dict(party=p["id"], roster=r["id"], ok=False, error=response["error"]))
                    continue
                state = response["data"]
                validate_observation(state)
                described = client.request("actions")
                if not described["ok"]: raise RuntimeError("Action discovery failed during preflight")
                for policy in config["policies"]:
                    covered = coverage(state, described["data"], policy["kind"])
                    checks.append(dict(party=p["id"], roster=r["id"], policy=policy["id"],
                                       ok=not covered["unsupported"], coverage=covered))
    return dict(ok=all(c["ok"] for c in checks), checks=checks)


def provenance(runner, assets, config):
    runner, assets = Path(runner).resolve(), Path(assets).resolve()
    content = [assets/"rules/catalog.json", assets/"npcs/level.json"]
    npc = read_json(assets/"npcs/level.json")
    content += [assets/"npcs"/p for p in npc["definitions"]]
    if config["mode"] == "visual":
        content += [assets/p for p in ("preparation.json", "prepared/metadata.json", "prepared/scene.bin", "prepared/npcs.bin", "prepared/loot.bin")]
    files = {str(p.relative_to(assets)): file_digest(p) for p in content}
    root = Path(__file__).resolve().parents[2]
    def git(*args):
        p = subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True, timeout=5)
        return p.stdout.strip() if p.returncode == 0 else None
    return dict(version=1, runner=str(runner), assets=str(assets), binary_sha256=file_digest(runner),
                content_sha256=files, code=code_identity(), source_revision=git("rev-parse", "HEAD"),
                working_tree=git("status", "--porcelain"), python=sys.version, platform=platform.platform(),
                processor=platform.processor(), protocol=1, timing="fixed-10ms-v1", manifest_hash=digest(config))


def trial_index(records, keep, runner, assets, root):
    result = []
    cli = Path(__file__).resolve().parent.parent/"arena_experiment.py"
    for row in records:
        result.append(dict(row, trace_retained=row["id"] in keep,
            replay_command=shlex.join([str(runner), "--assets", str(assets), "--mode", row["mode"], "--replay", str(root/row["trace"])])
                           if row["id"] in keep and row["trace_complete"] else None,
            rerun_command=shlex.join([sys.executable, str(cli), "rerun", str(root), "--trial", row["id"],
                                     "--output", str(root.with_name(root.name+"-rerun-"+row["id"]))])))
    return result


def run_batch(config, runner, assets, output, *, cancelled=None, progress=print):
    root = Path(output).resolve()
    if len(json.dumps(config, indent=2).encode()) + RESERVE > config["limits"]["output_bytes"]:
        raise ValueError("Output budget cannot reserve one bounded trial and its reports")
    root.mkdir(parents=True, exist_ok=False)
    for name in ("traces", "decisions", "scenarios"): (root/name).mkdir()
    started = time.monotonic(); deadline = started+config["limits"]["batch_seconds"]
    batch = dict(status="running", reason="", started=0, planned=len(config["parties"])*len(config["rosters"])*len(config["policies"])*len(config["seeds"]))
    records = []; retained = set(); used = 0; failure_count = 0; engine_identity = None
    atomic_json(root/"manifest.json", config)
    atomic_json(root/"batch.json", batch)
    with (root/"trials.jsonl").open("xb") as journal:
        try:
            prov = provenance(runner, assets, config)
            atomic_json(root/"provenance.json", prov)
            runner, assets = prov["runner"], prov["assets"]
            if cancelled and cancelled(): raise Cancelled("user_interrupt")
            checked = preflight(config, runner, assets, deadline=deadline, cancelled=cancelled)
            atomic_json(root/"preflight.json", checked)
            if not checked["ok"]:
                batch.update(status="invalid", reason="preflight_rejected; see preflight.json")
            else:
                used = sum(p.stat().st_size for p in root.iterdir() if p.is_file())
                for spec in trials(config, digest(prov["code"])):
                    if cancelled and cancelled():
                        batch.update(status="cancelled", reason="user_interrupt"); break
                    if time.monotonic() >= deadline:
                        batch.update(status="incomplete", reason="batch_wall_limit"); break
                    if used+RESERVE > config["limits"]["output_bytes"] or shutil.disk_usage(root).free < RESERVE:
                        batch.update(status="incomplete", reason="output_budget_or_disk_space"); break
                    scenario_path = root/"scenarios"/(spec["id"]+".json")
                    with scenario_path.open("x") as f: f.write(canonical(spec["scenario"])+"\n")
                    batch["started"] += 1
                    batch["active_trial"] = spec["id"]
                    atomic_json(root/"batch.json", batch)
                    row = run_trial(spec, runner, assets, root, batch_deadline=deadline, cancelled=cancelled)
                    row["code_identity"] = digest(prov["code"])
                    if row["engine_identity"] is not None:
                        identity = digest(row["engine_identity"])
                        if engine_identity is None: engine_identity = identity
                        elif identity != engine_identity:
                            row["artifact_errors"].append("engine_content_changed_during_batch")
                            row["balance_eligible"] = False
                    line = (canonical(row)+"\n").encode()
                    if len(line) > MIB: raise OSError("trial_summary_limit")
                    journal.write(line); journal.flush(); os.fsync(journal.fileno())
                    records.append(compact(row))
                    batch.pop("active_trial", None)
                    used += len(line)+row["trace_bytes"]+row.get("decision_bytes",0)+scenario_path.stat().st_size
                    new_keep = {r["id"] for r in records} if config["retention"] == "all" else keep_traces(records)
                    # Commit the summary and retention index before removing a previously selected success.
                    index_path = root/"trial_index.json"
                    old_size = index_path.stat().st_size if index_path.exists() else 0
                    atomic_json(index_path, trial_index(records, new_keep, runner, assets, root))
                    used += index_path.stat().st_size-old_size
                    for trial_id in (retained | {row["id"]}) - new_keep:
                        path = root/"traces"/(trial_id+".jsonl")
                        size = path.stat().st_size
                        path.unlink(); used -= size
                    retained = new_keep
                    atomic_json(root/"batch.json", batch)
                    progress(f"{batch['started']}/{batch['planned']} {spec['party']} / {spec['roster']} / {spec['policy']['id']} seed={spec['seed']}: {row['status']} ({row['ticks']} ticks)")
                    if row["status"] == "policy_failure": failure_count += 1
                    if row["status"] in ("cancelled", "runner_failure") or row["artifact_errors"] or failure_count >= config["limits"]["failures"]:
                        batch.update(status="cancelled" if row["status"] == "cancelled" else "incomplete", reason=row["reason"] if not row["artifact_errors"] else "; ".join(row["artifact_errors"]))
                        break
                else:
                    batch.update(status="complete", reason="all_planned_trials_recorded")
        except Cancelled:
            batch.update(status="cancelled", reason="user_interrupt")
        except Exception as exc:
            batch.update(status="incomplete", reason=type(exc).__name__+": "+str(exc))
        finally:
            batch["wall_seconds"] = time.monotonic()-started
            batch["unstarted"] = batch["planned"]-batch["started"]
            atomic_json(root/"batch.json", batch)
    try:
        summary = write_reports(root)
    except (OSError, ValueError) as exc:
        batch.update(status="incomplete", reason="report_failure: "+str(exc))
        atomic_json(root/"batch.json", batch)
        # A failed report must not turn a partial journal into a completed batch.
        summary = dict(planned=batch["planned"], started=batch["started"], unstarted=batch["unstarted"],
                       batch=batch, counts={})
    return summary


def rerun(directory, trial_id, output, *, runner=None, assets=None, cancelled=None, progress=print):
    root = Path(directory)
    original = next((r for r in rows(root) if r["id"] == trial_id), None)
    if original is None: raise ValueError("Trial ID is not in this run")
    config = read_json(root/"manifest.json", 32*MIB)
    old = read_json(root/"provenance.json", 4*MIB)
    if code_identity() != old["code"]: raise ValueError("Policy/tool code changed; strict rerun refused")
    runner, assets = runner or old["runner"], assets or old["assets"]
    current = provenance(runner, assets, config)
    if any(current[k] != old[k] for k in ("binary_sha256", "content_sha256")):
        raise ValueError("Executable/content changed; strict rerun refused")
    one = copy.deepcopy(config)
    one["parties"] = [p for p in one["parties"] if p["id"] == original["party"]]
    one["rosters"] = [r for r in one["rosters"] if r["id"] == original["roster"]]
    one["policies"] = [p for p in one["policies"] if p["id"] == original["policy"]]
    one["seeds"] = [original["seed"]]; one["comparisons"] = []; one["retention"] = "all"
    summary = run_batch(one, runner, assets, output, cancelled=cancelled, progress=progress)
    new = next(rows(output), None)
    fields = ("id", "status", "reason", "decision_hash", "final_state_hash", "ticks", "metrics")
    differences = [k for k in fields if new is None or new.get(k) != original.get(k)]
    check = dict(original_trial=trial_id, matched=not differences, differing_fields=differences,
                 note="Wall-clock timeouts and external failures are not promised reproducible.")
    atomic_json(Path(output)/"rerun.json", check)
    summary["rerun"] = check
    return summary
