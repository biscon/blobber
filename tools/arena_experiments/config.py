"""Strict, bounded experiment inputs; scenario validity remains authoritative in C++."""
import copy
import hashlib
import itertools
import json
from pathlib import Path
import re

MIB = 1024 * 1024
DEFAULT_LIMITS = dict(ticks=60000, decisions=20000, mutations=20000, requests=100000,
                      previews=256, trial_seconds=120, request_seconds=10,
                      startup_seconds=30, batch_seconds=3600, output_bytes=4096*MIB,
                      sidecar_bytes=16*MIB, failures=1, stall_decisions=256,
                      stall_ticks=6000, stall_rounds=12)
CEILINGS = dict(DEFAULT_LIMITS, ticks=360000, failures=100)
POLICY_DEFAULTS = {
    "focus": {"heal_fraction": 0.25, "reserve_mana": 0},
    "conservative": {"heal_fraction": 0.5, "reserve_mana": 2},
    "aggressive": {"heal_fraction": 0.25, "reserve_mana": 0},
}


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False)


def digest(value):
    return hashlib.sha256(canonical(value).encode()).hexdigest()


def file_digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for data in iter(lambda: stream.read(MIB), b""):
            h.update(data)
    return h.hexdigest()


def code_identity():
    root = Path(__file__).resolve().parent.parent
    files = [root/"arena_experiment.py", root/"headless_client.py", *sorted((root/"arena_experiments").glob("*.py"))]
    return {str(p.relative_to(root)): file_digest(p) for p in files}


def unique_object(pairs):
    result = {}
    for k, v in pairs:
        if k in result:
            raise ValueError(f"Duplicate JSON field: {k}")
        result[k] = v
    return result


def read_json(path, limit=MIB):
    with Path(path).open("rb") as stream:
        raw = stream.read(limit + 1)
    if len(raw) > limit:
        raise ValueError(f"Input exceeds {limit} bytes: {path}")
    try:
        value = json.loads(raw, object_pairs_hook=unique_object,
                           parse_constant=lambda s: (_ for _ in ()).throw(ValueError(f"Invalid number {s}")))
        def depth(v, n=0):
            if n > 32:
                raise ValueError("JSON nesting exceeds 32")
            if isinstance(v, dict):
                for x in v.values(): depth(x, n+1)
            elif isinstance(v, list):
                for x in v: depth(x, n+1)
        depth(value)
        return value
    except RecursionError as exc:
        raise ValueError("Excessive JSON nesting") from exc


def fields(value, required, optional=()):
    if not isinstance(value, dict) or set(value) - set(required) - set(optional) or set(required) - set(value):
        raise ValueError(f"Expected fields {required}, optional {optional}")


def integer(value, low, high, name):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f"{name} must be an integer in {low}..{high}")
    return value


def identifier(value):
    if not isinstance(value, str) or not re.fullmatch(r"[a-z][a-z0-9_-]{0,47}", value):
        raise ValueError(f"Invalid experiment ID: {value!r}")
    return value


def entries(value, maximum, name):
    if not isinstance(value, list) or not 1 <= len(value) <= maximum:
        raise ValueError(f"{name} requires 1..{maximum} entries")
    return value


def named(value, maximum, name):
    items = entries(value, maximum, name)
    ids = [identifier(x.get("id")) if isinstance(x, dict) else identifier(None) for x in items]
    if len(set(ids)) != len(ids):
        raise ValueError(f"Duplicate {name} ID")
    return items


def resolve(value, base, *, embedded=False):
    fields(value, ("version", "parties", "rosters", "policies", "seeds"),
           ("mode", "reaction_ticks", "limits", "retention", "comparisons"))
    integer(value["version"], 1, 1, "version")
    result = copy.deepcopy(value)
    parties = named(result["parties"], 16, "parties")
    for p in parties:
        fields(p, ("id", "scenario"))
        if not embedded:
            if not isinstance(p["scenario"], str):
                raise ValueError("Party scenario must be a filename")
            p["scenario"] = read_json(Path(base)/p["scenario"])
        s = p["scenario"]
        fields(s, ("schema_version", "layout_version", "name", "seed", "party", "monsters"))
        integer(s["schema_version"], 1, 1, "scenario version")
        integer(s["layout_version"], 1, 1, "layout version")
        if not isinstance(s["party"], list) or len(s["party"]) != 4:
            raise ValueError("Exactly four party snapshots required")
        ids = []
        for c in s["party"]:
            if not isinstance(c, dict) or type(c.get("level")) is not int or c["level"] != 1:
                raise ValueError("Only level-one starting snapshots supported")
            ids.append(c.get("id"))
        if any(not isinstance(i, str) or not i for i in ids) or len(set(ids)) != 4:
            raise ValueError("Four distinct character IDs required")
    for r in named(result["rosters"], 16, "rosters"):
        fields(r, ("id", "monsters"))
        monsters = entries(r["monsters"], 32, "roster")
        seen = set(); total = 0
        for m in monsters:
            fields(m, ("monster", "quantity"))
            if not isinstance(m["monster"], str) or not m["monster"] or m["monster"] in seen:
                raise ValueError("Invalid/duplicate monster ID")
            seen.add(m["monster"])
            total += integer(m["quantity"], 1, 256, "quantity")
        if total > 256:
            raise ValueError("Roster exceeds 256 actors")
    for p in named(result["policies"], 8, "policies"):
        fields(p, ("id", "kind", "version"), ("parameters",))
        if not isinstance(p["kind"], str) or p["kind"] not in POLICY_DEFAULTS:
            raise ValueError("Unknown policy")
        integer(p["version"], 1, 1, "policy version")
        params = p.setdefault("parameters", {})
        fields(params, (), ("heal_fraction", "reserve_mana"))
        p["parameters"] = dict(POLICY_DEFAULTS[p["kind"]], **params)
        fraction = p["parameters"]["heal_fraction"]
        if type(fraction) not in (int, float) or not 0 <= fraction <= 1:
            raise ValueError("heal_fraction must be in 0..1")
        integer(p["parameters"]["reserve_mana"], 0, 10, "reserve_mana")
    seeds = entries(result["seeds"], 1024, "seeds")
    for seed in seeds: integer(seed, 0, 2**32-1, "seed")
    if len(set(seeds)) != len(seeds):
        raise ValueError("Duplicate seeds are not independent trials")
    count = len(parties)*len(result["rosters"])*len(result["policies"])*len(seeds)
    if count > 10000:
        raise ValueError("Matrix exceeds 10,000 trials")
    result.setdefault("mode", "diagnostic")
    if result["mode"] not in ("diagnostic", "visual"):
        raise ValueError("Unknown observation mode")
    integer(result.setdefault("reaction_ticks", 25), 0, 1000, "reaction_ticks")
    supplied = result.setdefault("limits", {})
    fields(supplied, (), tuple(DEFAULT_LIMITS))
    result["limits"] = dict(DEFAULT_LIMITS, **supplied)
    for k, v in result["limits"].items(): integer(v, 1, CEILINGS[k], k)
    if result.setdefault("retention", "selected") not in ("selected", "all"):
        raise ValueError("retention must be selected or all")
    comparisons = result.setdefault("comparisons", [])
    if not isinstance(comparisons, list) or len(comparisons) > 16:
        raise ValueError("At most 16 declared party comparisons")
    seen_pairs = set()
    for pair in comparisons:
        if not isinstance(pair, list) or len(pair) != 2 or any(x not in [p["id"] for p in parties] for x in pair) or pair[0] == pair[1]:
            raise ValueError("Comparison requires two distinct party IDs")
        if tuple(pair) in seen_pairs: raise ValueError("Duplicate comparison")
        seen_pairs.add(tuple(pair))
    if len(canonical(result).encode()) > 32*MIB:
        raise ValueError("Resolved manifest exceeds 32 MiB")
    return result


def load(path):
    path = Path(path)
    return resolve(read_json(path), path.parent)


def trials(config, code_hash):
    for p, r, policy, seed in itertools.product(config["parties"], config["rosters"], config["policies"], config["seeds"]):
        scenario = copy.deepcopy(p["scenario"])
        scenario.update(seed=seed, monsters=r["monsters"], name="Balance trial")
        identity = dict(party=p["id"], roster=r["id"], scenario=scenario, policy=policy, mode=config["mode"],
                        reaction_ticks=config["reaction_ticks"], limits=config["limits"], code=code_hash)
        yield dict(id=digest(identity)[:24], party=p["id"], roster=r["id"], policy=policy,
                   seed=seed, scenario=scenario, mode=config["mode"],
                   reaction_ticks=config["reaction_ticks"], limits=config["limits"])
