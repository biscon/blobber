"""Streaming report input, explicit denominators, bounded descriptive statistics."""
from collections import Counter, defaultdict
import csv
import io
import json
import math
from pathlib import Path
import random
import statistics

from .config import canonical, digest, MIB, read_json
from .trial import STATUSES


def atomic_json(path, value):
    atomic_text(path, json.dumps(value, indent=2, allow_nan=False)+"\n")


def atomic_text(path, value):
    path = Path(path)
    temp = path.with_name(path.name+".tmp")
    created = False
    try:
        with temp.open("x", encoding="utf-8") as stream:
            created = True
            stream.write(value)
            stream.flush()
        temp.replace(path)
    except OSError:
        if created:
            temp.unlink(missing_ok=True)
        raise


def rows(directory):
    path = Path(directory)/"trials.jsonl"
    if not path.exists(): return
    with path.open("rb") as stream:
        count = 0
        while True:
            line = stream.readline(MIB+1)
            if not line: break
            if len(line) > MIB or not line.endswith(b"\n"):
                raise ValueError("Incomplete or oversized trial journal row; original retained")
            count += 1
            if count > 10000: raise ValueError("Trial journal exceeds 10,000 rows")
            yield json.loads(line)


def wilson(wins, total):
    if not total: return None
    z = 1.959963984540054
    p = wins/total
    d = 1+z*z/total
    middle = (p+z*z/(2*total))/d
    half = z*math.sqrt(p*(1-p)/total+z*z/(4*total*total))/d
    return [max(0, middle-half), min(1, middle+half)]


def descriptive(values):
    if not values: return None
    ordered = sorted(values)
    return dict(n=len(values), minimum=ordered[0], median=statistics.median(ordered), maximum=ordered[-1])


def group_key(row):
    return (row["party"], row["roster"], row["policy"], row["mode"], row["reaction_ticks"])


def keep_traces(compact):
    """All failures/defeats, first victory and duration extremes per compatible group."""
    keep = {r["id"] for r in compact if r["status"] != "victory" or not r["balance_eligible"]}
    groups = defaultdict(list)
    for row in compact:
        if row["status"] == "victory" and row["balance_eligible"]:
            groups[group_key(row)].append(row)
    for group in groups.values():
        keep.add(group[0]["id"])
        keep.add(min(group, key=lambda r: (r["ticks"], r["id"]))["id"])
        keep.add(min(group, key=lambda r: (-r["ticks"], r["id"]))["id"])
    return keep


def compact(row):
    keys = ("id", "party", "roster", "policy", "seed", "mode", "reaction_ticks", "status",
            "reason", "balance_eligible", "ticks", "trace", "decisions", "trace_complete")
    return {k: row[k] for k in keys}


def aggregate(source, config):
    groups = {}; identities = set(); code_ids = set(); statuses = Counter(); paired = {}; party_details = {}
    total_wall = 0.; total_ticks = 0; artifacts = 0
    count = 0; seen = set()
    for row in source:
        if row["id"] in seen: raise ValueError("Duplicate trial ID in journal")
        seen.add(row["id"]); count += 1
        if count > 10000: raise ValueError("Report exceeds 10,000 trials")
        if row["status"] not in STATUSES: raise ValueError("Unknown trial status")
        if row.get("engine_identity"):
            identities.add(digest(row["engine_identity"]))
        if len(identities) > 1: raise ValueError("Cannot pool different engine/content identities")
        if row.get("code_identity"): code_ids.add(row["code_identity"])
        if len(code_ids) > 1: raise ValueError("Cannot pool different policy/tool implementations")
        statuses[row["status"]] += 1
        total_wall += row["timings"]["total_seconds"]
        total_ticks += row["ticks"]
        artifacts += bool(row["artifact_errors"])
        if row["metrics"]["initial"]: party_details.setdefault(row["party"], row["metrics"]["initial"])
        key = group_key(row)
        if key not in groups:
            groups[key] = dict(party=key[0], roster=key[1], policy=key[2], mode=key[3], reaction_ticks=key[4],
                               started=0, counts=Counter(), valid_completions=0, wins=0, losses=0,
                               artifact_failures=0, durations=[], casualty_samples=[], mana_samples=[],
                               hp_samples=[], mana_spent_samples=[], consumption=Counter(), actions=Counter(),
                               coverage=row.get("coverage"), wall_seconds=0.)
        g = groups[key]
        g["started"] += 1; g["counts"][row["status"]] += 1
        g["artifact_failures"] += bool(row["artifact_errors"])
        g["wall_seconds"] += row["timings"]["total_seconds"]
        g["actions"].update(row["metrics"]["action_counts"])
        if row["balance_eligible"]:
            if row["status"] not in ("victory", "combat_loss"): raise ValueError("Invalid balance eligibility")
            g["valid_completions"] += 1
            g["wins"] += row["status"] == "victory"
            g["losses"] += row["status"] == "combat_loss"
            g["durations"].append(row["simulation_ms"]/1000)
            g["casualty_samples"].append(row["metrics"]["permanent_deaths"])
            g["mana_samples"].append(sum(p["mana"] for p in row["metrics"]["final"]))
            g["hp_samples"].append(sum(p["hp"] for p in row["metrics"]["final"]))
            g["mana_spent_samples"].append(sum(row["metrics"]["mana_spent"]))
            g["consumption"].update(row["metrics"]["consumed_items"])
            pair_key = (row["party"], row["roster"], row["policy"], row["seed"])
            if pair_key in paired: raise ValueError("Duplicate paired seed")
            paired[pair_key] = int(row["status"] == "victory")
    for g in groups.values():
        n = g["valid_completions"]
        g.update(planned=len(config["seeds"]), unstarted=len(config["seeds"])-g["started"],
                 completion_rate=n/g["started"], wins_per_started=g["wins"]/g["started"],
                 victory_rate=g["wins"]/n if n else None, victory_wilson_95=wilson(g["wins"], n),
                 sample_scope="valid victories and combat losses; unfinished trials excluded")
        for source_name, target in (("durations", "simulation_seconds"), ("casualty_samples", "permanent_deaths"),
                                    ("mana_samples", "remaining_mana"), ("hp_samples", "remaining_hp"),
                                    ("mana_spent_samples", "mana_spent")):
            g[target] = descriptive(g.pop(source_name))
        g["counts"] = {s: g["counts"][s] for s in STATUSES}
        g["consumption"] = dict(g["consumption"]); g["actions"] = dict(g["actions"])
    comparisons = []
    for a, b in config["comparisons"]:
        for r in config["rosters"]:
            for p in config["policies"]:
                differences = []
                for seed in config["seeds"]:
                    ka, kb = (a, r["id"], p["id"], seed), (b, r["id"], p["id"], seed)
                    if ka in paired and kb in paired: differences.append(paired[kb]-paired[ka])
                n = len(differences); interval = None
                if n >= 20:
                    rng = random.Random(int(digest([a,b,r["id"],p["id"],config["seeds"]]), 16))
                    means = sorted(sum(rng.choices(differences, k=n))/n for _ in range(2000))
                    interval = [means[49], means[1949]]
                comparisons.append(dict(baseline=a, alternative=b, roster=r["id"], policy=p["id"],
                                        paired=n, missing_pairs=len(config["seeds"])-n,
                                        victory_difference=sum(differences)/n if n else None,
                                        bootstrap_95=interval,
                                        uncertainty="paired_bootstrap_2000" if interval else "insufficient_pairs_minimum_20"))
    planned = len(config["parties"])*len(config["rosters"])*len(config["policies"])*len(config["seeds"])
    return dict(version=1, planned=planned, started=count, unstarted=planned-count,
                counts={s: statuses[s] for s in STATUSES}, artifact_failures=artifacts,
                trial_wall_seconds=total_wall, simulation_seconds=total_ticks/100,
                simulated_seconds_per_wall_second=total_ticks/100/total_wall if total_wall else None,
                completed_trials_per_minute=(statuses["victory"]+statuses["combat_loss"])*60/total_wall if total_wall else None,
                groups=list(groups.values()), comparisons=comparisons, parties=party_details)


def percentage(v):
    return "N/A" if v is None else f"{100*v:.1f}%"


def render(summary, config, batch):
    lines = ["# Arena playtesting report", "", f"Batch status: **{batch['status']}** ({batch.get('reason', '')}).",
             f"Planned {summary['planned']}; started {summary['started']}; unstarted {summary['unstarted']}.", "",
             "This is an exploratory policy comparison, not a target that every party should win.",
             "Level one; fixed starter gear/preparation; no rest, loot or advancement during trials.",
             f"Information: **{config['mode']}**. Real-time party action interval: {config['reaction_ticks']*10} ms; initiative decisions have no added delay.",
             "Diagnostic mode knows all enemy positions/HP. Visual mode uses prepared body picking. Preview is a zero-time legality oracle.",
             "Seeds are shared across configurations; different actions need not consume the same rolls.", "",
             "| Party | Roster | Policy | Started / planned | Valid W / L | Win rate (95% Wilson) | Other outcomes |",
             "| --- | --- | --- | --- | --- | --- | --- |"]
    for g in summary["groups"]:
        ci = g["victory_wilson_95"]
        rate = percentage(g["victory_rate"])+(f" ({percentage(ci[0])}–{percentage(ci[1])})" if ci else "")
        other = ", ".join(f"{s}: {v}" for s,v in g["counts"].items() if v and s not in ("victory", "combat_loss")) or "none"
        if g["artifact_failures"]: other += f"; artifact failures: {g['artifact_failures']}"
        lines.append(f"| {g['party']} | {g['roster']} | {g['policy']} | {g['started']} / {g['planned']} | {g['wins']} / {g['losses']} | {rate} | {other} |")
    lines += ["", "Win-rate denominator is valid wins + combat losses only. Completion rates, wins/started and every failure count are also in summary.json.",
              "Casualty, remaining/consumed-resource and duration statistics in summary.json include valid wins and losses; unfinished runs are separate in trials.csv/jsonl.", "",
              "| Comparison (alternative − baseline) | Roster | Policy | Valid pairs | Difference | Uncertainty |",
              "| --- | --- | --- | --- | --- | --- |"]
    for c in summary["comparisons"]:
        interval = c["bootstrap_95"]
        unc = (f"{percentage(interval[0])} to {percentage(interval[1])}" if interval else "fewer than 20 pairs; preliminary")
        lines.append(f"| {c['alternative']} − {c['baseline']} | {c['roster']} | {c['policy']} | {c['paired']} | {percentage(c['victory_difference'])} | {unc} |")
    lines += ["", "Party packages (whole-party comparisons; race, gear and resources can differ):", ""]
    for name, party in summary["parties"].items():
        lines.append(f"- **{name}**, front-to-back: "+", ".join(p["race"]+" "+p["class"] for p in party)+".")
        for p in party:
            spells = sorted(set(p["cantrips"]+p["prepared"]+p["domain"]+p["racial"]))
            lines.append(f"  - Slot {p['character']}: abilities {p['abilities']}, HP {p['max_hp']}, AC {p['armor_class']}, mana {p['max_mana']}; items {canonical(p['items'])}; spells {', '.join(spells) or 'none'}.")
    lines += ["", "Coverage is recorded per trial/group. Focus deliberately preserves offensive mana; other policies use only their documented supported subset.",
              "Unsupported active abilities stop a trial. A bot failure or stall is never counted as a combat loss.",
              "Full coverage, resource distributions, action counts and paired sample sizes are in summary.json.", "",
              f"Total trial wall time: {summary['trial_wall_seconds']:.3f} s (includes startup and failed trials).",
              f"Completed trials/min: {summary['completed_trials_per_minute'] or 0:.3f}; simulated seconds/wall second: {summary['simulated_seconds_per_wall_second'] or 0:.3f}.",
              "These measurements include Python/IPC/recording and are not pure simulation benchmarks. report_seconds is separate in summary.json.",
              "See provenance.json for executable/content/code identity. trial_index.json lists retained artifacts and exact replay/rerun commands.",
              "No experiment is registered in the routine test suite.", ""]
    return "\n".join(lines)


def write_reports(directory):
    import time
    started = time.monotonic()
    root = Path(directory)
    config = read_json(root/"manifest.json", 32*MIB)
    batch = read_json(root/"batch.json")
    summary = aggregate(rows(root), config)
    summary["recorded_trials"] = summary["started"]
    summary["unrecorded_started_trials"] = max(0, batch["started"]-summary["started"])
    summary["started"] = batch["started"]
    summary["unstarted"] = summary["planned"]-summary["started"]
    summary["batch"] = batch
    data = io.StringIO(); writer = csv.writer(data)
    writer.writerow(["trial", "party", "roster", "policy", "seed", "status", "reason", "balance_eligible", "ticks",
                     "rounds", "deaths", "unconscious", "remaining_hp", "remaining_mana", "mana_spent", "mana_recovered",
                     "consumed_items", "wall_seconds", "trace_complete", "artifact_errors"])
    for row in rows(root):
        m = row["metrics"]; final = m["final"]
        writer.writerow([row["id"], row["party"], row["roster"], row["policy"], row["seed"], row["status"], row["reason"],
                         row["balance_eligible"], row["ticks"], m["rounds_completed"], m["permanent_deaths"], m["unconscious_survivors"],
                         sum(p["hp"] for p in final) if final else "", sum(p["mana"] for p in final) if final else "",
                         sum(m["mana_spent"]), sum(m["mana_recovered"]), canonical(m["consumed_items"]),
                         row["timings"]["total_seconds"], row["trace_complete"], "; ".join(row["artifact_errors"])])
    summary["report_seconds"] = time.monotonic()-started
    outputs = {"trials.csv": data.getvalue(), "report.md": render(summary, config, batch),
               "summary.json": json.dumps(summary, indent=2, allow_nan=False)+"\n"}
    used = sum(p.stat().st_size for p in root.rglob("*") if p.is_file())
    additional = sum(len(text.encode())-(root/name).stat().st_size if (root/name).exists()
                     else len(text.encode()) for name, text in outputs.items())
    # Atomic replacement temporarily retains the old file; include the largest one.
    scratch = max(((root/name).stat().st_size for name in outputs if (root/name).exists()), default=0)
    if used+additional+scratch > config["limits"]["output_bytes"]:
        raise OSError("output_budget_for_reports")
    for name, text in outputs.items():
        atomic_text(root/name, text)
    return summary
