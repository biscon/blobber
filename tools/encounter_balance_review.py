#!/usr/bin/env python3
"""Explicit bounded density/guardian study. Run one case, inspect, then run the next."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import time
from campaign_route_review import Route, POLICY, MIB
from arena_experiments.trial import StopTrial, resources
from headless_client import ClientError


class ReviewRoute(Route):
    def __init__(self, *args, **kwargs):
        self.audit = (args[2]/"journal.jsonl").open("w")
        self.audit_bytes = 0
        self.audit_complete = False
        super().__init__(*args, **kwargs, record=False)
        self.log(dict(kind="header", journal_version=1, replayable=False, scenario=self.scenario))

    def log(self, value):
        line = json.dumps(value,separators=(",", ":"))+"\n"
        if self.audit_bytes+len(line.encode()) > 256*MIB:
            raise StopTrial("artifact_failure", "compact_journal_limit")
        self.audit.write(line)
        self.audit_bytes += len(line.encode())

    def events(self, chunk):
        self.log(dict(kind="events", chunk=chunk))
        super().events(chunk)

    def call(self, op, **fields):
        self.log(dict(kind="request", operation=self.client.sequence+1, op=op, fields=fields))
        try:
            return super().call(op, **fields)
        finally:
            if self.state:
                self.log(dict(kind="boundary", ticks=self.state["ticks"], phase=self.state["phase"],
                              position=self.state["position"], round=self.state["round"],
                              party=[{k:p[k] for k in ("hp","mana","life","level","xp")} for p in self.state["party"]]))

    def close(self):
        try:
            super().close()
            self.audit_complete = not self.client.failed
            self.log(dict(kind="footer", complete=self.audit_complete))
        finally:
            self.audit.close()


class GuardianRoute(ReviewRoute):
    def run(self):
        self.call("start", scenario=self.scenario)
        layout = self.state["generation"]
        encounter = next(e for e in layout["encounters"] if e["role"] == "guardian")
        self.expected = set(range(encounter["first"], encounter["first"]+encounter["count"]))
        self.checkpoints.append(dict(depth=layout["depth"], resources=resources(self.state)))
        self.act(dict(kind="interact", fixture=layout["objective"]["approach_door"]))
        self.fight(encounter)
        raise StopTrial("victory", "guardian_defeated")


def cases():
    guardians = [("guardian", party, seed, depth, False)
                 for depth in (1, 2, 3) for party in ("balanced", "no_wizard") for seed in (5, 8)]
    direct = [("campaign", party, seed, 1, False) for party in ("balanced", "no_wizard") for seed in (5, 8)]
    full = [("campaign", party, 5, 1, True) for party in ("balanced", "no_wizard")]
    return guardians+direct+full


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--guardian-runner", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case", type=int, choices=range(1, 19), required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=args.case != 1)
    root = Path(__file__).resolve().parents[1]
    profiles = root/"experiments/encounter_density/parties"
    previous = json.loads((args.output/"summary.json").read_text()) if args.case > 1 else []
    if len(previous) != args.case-1 or any(r["status"] not in ("victory", "combat_loss") or not r["trace_complete"] for r in previous):
        raise ValueError("Inspect completed prior cases; no implicit repeat/expansion")
    inputs = [args.runner, args.guardian_runner, args.assets/"rules/catalog.json", args.assets/"npcs/level.json",
              Path(__file__), root/"tools/campaign_route_review.py", root/"tools/headless_client.py",
              profiles/"balanced.json", profiles/"no_wizard.json",
              *sorted((root/"tools/arena_experiments").glob("*.py")),
              *[args.assets/"npcs"/name for name in json.loads((args.assets/"npcs/level.json").read_text())["definitions"]]]
    provenance = dict(inputs={str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
                      policy=POLICY, guardian_seconds=60, route_seconds=180, study_seconds=1800, output_bytes=3*1024*MIB)
    if args.case == 1:
        provenance["deadline_wall"] = time.time()+1800
        (args.output/"provenance.json").write_text(json.dumps(provenance,indent=2)+"\n")
    else:
        old = json.loads((args.output/"provenance.json").read_text()); provenance["deadline_wall"] = old["deadline_wall"]
        if provenance != old: raise ValueError("Study inputs changed; preserve evidence and stop")
    used = sum(p.stat().st_size for p in args.output.rglob("*") if p.is_file())
    if used+300*MIB > 3*1024*MIB or shutil.disk_usage(args.output).free < 300*MIB:
        raise ValueError("Study storage ceiling")
    kind, party, seed, depth, reserve = cases()[args.case-1]
    name = f"{args.case:02}-{kind}-{party}-{seed}-d{depth}-{'full' if reserve else 'direct'}"
    directory = args.output/name; directory.mkdir()
    package = json.loads((profiles/(party+".json")).read_text())
    scenario = dict(kind="generated_floor" if kind == "guardian" else "campaign", schema_version=1,
                    generator_version=12, template_version=9, seed=seed, party=package["party"])
    if kind == "guardian": scenario["depth"] = depth
    (directory/"scenario.json").write_text(json.dumps(scenario,indent=2)+"\n")
    deadline = time.monotonic()+max(0,provenance["deadline_wall"]-time.time())
    deadline = min(deadline,time.monotonic()+(60 if kind == "guardian" else 180))
    route = (GuardianRoute if kind == "guardian" else ReviewRoute)(args.guardian_runner if kind == "guardian" else args.runner,
                args.assets,directory,scenario,reserve,deadline)
    started = time.monotonic(); status, reason = "runner_failure", "not_started"
    try:
        route.run()
    except StopTrial as exc: status, reason = exc.status, exc.reason
    except KeyboardInterrupt: status, reason = "cancelled", "user_interrupt"
    except (ClientError,OSError,ValueError,KeyError) as exc: status, reason = "runner_failure", repr(exc)
    finally:
        try: route.close()
        except Exception as exc: status, reason = "runner_failure", "close: "+repr(exc)
    if route.merges and status in ("victory", "combat_loss"):
        status, reason = "coverage_failure", "unexpected_encounter_merge"
    complete, size = route.audit_complete, route.audit_bytes
    result = dict(id=name,status=status,reason=reason,seconds=time.monotonic()-started,trace_complete=complete,
                  trace_bytes=size,journal_kind="compact requests/events/resources; not replayable",metrics=route.metrics.result(),consumed_items=dict(route.consumed),
                  checkpoints=route.checkpoints,rests=route.rests,steps=route.steps,retraced_steps=route.retraced,
                  merges=route.merges,coverage=route.coverage,final=route.state)
    (directory/"result.json").write_text(json.dumps(result,indent=2)+"\n")
    (directory/"decisions.jsonl").write_text("".join(json.dumps(x)+"\n" for x in route.journal))
    previous.append({k:v for k,v in result.items() if k != "final"})
    (args.output/"summary.json").write_text(json.dumps(previous,indent=2)+"\n")
    print(name,status,reason,f"{result['seconds']:.2f}s",flush=True)
    return 0 if status in ("victory","combat_loss") and complete else 2


if __name__ == "__main__":
    raise SystemExit(main())
