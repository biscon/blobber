"""One bounded trial: checked commands, fixed-step scheduling and streaming artifacts."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import time

from headless_client import Client, ClientError, DeadlineExceeded, Cancelled, ProtocolError, RunnerError
from .config import canonical, digest, MIB
from .policies import Policy, Candidate, coverage, policy_view, CONSCIOUS, DEAD, DYING, STABLE

STATUSES = ("victory", "combat_loss", "stall", "timeout", "unsupported_decision", "policy_failure", "runner_failure", "cancelled")


class StopTrial(Exception):
    def __init__(self, status, reason):
        self.status, self.reason = status, reason
        super().__init__(reason)


class ArtifactError(OSError):
    pass


class DecisionLog:
    def __init__(self, path, limit):
        self.stream = Path(path).open("xb")
        self.limit, self.bytes, self.count = limit, 0, 0
        self.hash = hashlib.sha256()

    def write(self, record):
        data = (canonical(dict(sequence=self.count, **record)) + "\n").encode()
        if len(data) > self.limit-self.bytes:
            raise ArtifactError("decision_sidecar_limit")
        try:
            self.stream.write(data)
            self.stream.flush()
        except OSError as exc:
            raise ArtifactError(f"decision_write: {exc}") from exc
        self.hash.update(data)
        self.bytes += len(data); self.count += 1

    def close(self):
        self.stream.close()


def validate_observation(o):
    if not isinstance(o, dict) or len(o.get("party", [])) != 4 or len(o.get("enemies", [])) > 256:
        raise ProtocolError("Observation party/enemy bounds")
    if [p.get("character") for p in o["party"]] != list(range(4)):
        raise ProtocolError("Observation character identities")
    if any(p.get("life") not in (CONSCIOUS, DYING, STABLE, DEAD) for p in o["party"]):
        raise ProtocolError("Unknown life enum")
    required = ("phase", "terminal", "position", "heading", "ticks", "moving", "round", "gameplay_ms",
                "awaiting_action", "resolving_action", "can_end_turn", "active_character", "formation", "mode")
    if any(k not in o for k in required) or o["phase"] not in (
            "exploration", "contact", "turns", "victory", "defeat", "approach", "facing", "round_end"):
        raise ProtocolError("Unknown observation phase/schema")


def terminal_status(o):
    if o.get("terminal"):
        if o["phase"] == "victory": return "victory"
        if o["phase"] == "defeat": return "combat_loss"
        raise ProtocolError("Terminal without a result phase")
    return None


def resources(o):
    if o is None: return None
    keys = ("character", "name", "class", "race", "level", "hp", "max_hp", "mana", "max_mana", "life",
            "temporary_hp", "death_successes", "death_failures", "hit_dice", "second_wind", "arcane_recovery",
            "concentration", "armor_class", "abilities", "base_abilities", "cantrips", "prepared", "domain", "racial")
    result = []
    for p in o["party"]:
        quantities = Counter()
        for item in p["inventory"]: quantities[item["item"]] += item["quantity"]
        result.append(dict({k: p[k] for k in keys}, items=dict(quantities),
                           equipment=[{k: v for k, v in item.items() if k != "handle"}
                                      for item in p["inventory"] if item["cell"] >= 48]))
    return result


class Metrics:
    def __init__(self):
        self.events = Counter()
        self.actions = Counter()
        self.reasons = Counter()
        self.mana_spent = [0]*4; self.mana_recovered = [0]*4
        self.hp_lost = [0]*4; self.hp_gained = [0]*4
        self.downings = [0]*4
        self.consumed = Counter()
        self.previous = None
        self.initial = None
        self.rounds = self.encounters = 0

    def event(self, chunk):
        for e in chunk["events"]:
            self.events[e["event"]] += 1
            if e["event"] == "round": self.rounds += 1
            if e["event"] == "started": self.encounters += 1

    def observe(self, o):
        now = resources(o)
        if self.initial is None: self.initial = now
        if self.previous is not None:
            for i, (old, new) in enumerate(zip(self.previous, now)):
                delta = new["mana"]-old["mana"]
                self.mana_spent[i] += max(0, -delta); self.mana_recovered[i] += max(0, delta)
                delta = new["hp"]-old["hp"]
                self.hp_lost[i] += max(0, -delta); self.hp_gained[i] += max(0, delta)
                if old["life"] == CONSCIOUS and new["life"] in (DYING, STABLE, DEAD): self.downings[i] += 1
                for item, quantity in old["items"].items():
                    self.consumed[item] += max(0, quantity-new["items"].get(item, 0))
        self.previous = now

    def result(self):
        return dict(initial=self.initial, final=self.previous, events=dict(self.events),
                    action_counts=dict(self.actions), policy_reasons=dict(self.reasons),
                    mana_spent=self.mana_spent, mana_recovered=self.mana_recovered,
                    hp_lost_observed=self.hp_lost, hp_gained_observed=self.hp_gained,
                    downings_observed_lower_bound=self.downings, consumed_items=dict(self.consumed),
                    rounds_completed=self.rounds, encounters=self.encounters,
                    permanent_deaths=sum(p["life"] == DEAD for p in self.previous) if self.previous else None,
                    unconscious_survivors=sum(p["life"] in (DYING, STABLE) for p in self.previous) if self.previous else None)


class ProgressWatch:
    def __init__(self, limits):
        self.limits = limits
        self.last = None
        self.visits = set()
        self.enemy_health = {}
        self.last_tick = self.last_round = 0
        self.signatures = Counter()

    def check(self, o, rounds, encounters, decision=False):
        party = [(p["hp"], p["life"], p["mana"], p["temporary_hp"], p["second_wind"], p["arcane_recovery"],
                  tuple(sorted(s["name"] for s in p["statuses"])),
                  tuple((i["item"], i["quantity"]) for i in p["inventory"])) for p in o["party"]]
        changed_enemy = False
        for e in o["enemies"]:
            value = (e["health_fraction"], tuple(sorted(s["name"] for s in e["statuses"])))
            if self.enemy_health.get(e["actor"]) != value: changed_enemy = True
            self.enemy_health[e["actor"]] = value
        new_cell = tuple(o["position"]) not in self.visits
        self.visits.add(tuple(o["position"]))
        state = (party, encounters)
        if self.last != state or changed_enemy or new_cell:
            self.last = state
            self.last_tick, self.last_round = o["ticks"], rounds
            self.signatures.clear()
        if o["phase"] in ("exploration", "approach") and o["ticks"]-self.last_tick >= self.limits["stall_ticks"]:
            return "no_realtime_progress"
        if rounds-self.last_round >= self.limits["stall_rounds"]:
            return "no_round_progress"
        if decision:
            signature = (o["phase"], tuple(o["position"]), o["heading"], o["active_character"])
            self.signatures[signature] += 1
            if sum(self.signatures.values()) >= self.limits["stall_decisions"]:
                return "repeated_decisions_without_progress"
        return None


def trace_metadata(path):
    try:
        with Path(path).open("rb") as stream:
            first = stream.readline(4*MIB+1)
            header = json.loads(first)
            stream.seek(0, 2)
            size = stream.tell()
            stream.seek(max(0, size-256))
            tail = stream.read().splitlines()
            complete = bool(tail and json.loads(tail[-1]) == {"type": "complete", "data": {"complete": True}})
            return header.get("data") if header.get("type") == "header" else None, complete, size
    except (OSError, ValueError):
        return None, False, Path(path).stat().st_size if Path(path).exists() else 0


def run_trial(spec, runner, assets, output, *, batch_deadline=None, cancelled=None):
    started = time.monotonic()
    limits = spec["limits"]
    deadline = min(started+limits["trial_seconds"], batch_deadline or float("inf"))
    base = Path(output)
    trace = base/"traces"/(spec["id"]+".jsonl")
    decisions = base/"decisions"/(spec["id"]+".jsonl")
    row = {k: spec[k] for k in ("id", "party", "roster", "seed", "mode", "reaction_ticks")}
    row.update(policy=spec["policy"]["id"], policy_config=spec["policy"], status="runner_failure", reason="not_started",
               artifact_errors=[], diagnostic_inspection_used=False, coverage=None,
               trace=str(trace.relative_to(base)), decisions=str(decisions.relative_to(base)), replay="not_checked")
    metrics = Metrics(); watch = ProgressWatch(limits); policy = Policy(spec["policy"])
    state = client = log = None
    counts = dict(decisions=0, actions=0, previews=0, rejected_previews=0, requests=0, max_decision_previews=0)
    timings = dict(startup_seconds=0.0, policy_seconds=0.0, play_loop_seconds=0.0)
    loop_started = None
    next_action = spec["reaction_ticks"]
    last_request = None

    def call(op, **fields):
        nonlocal state, last_request
        # Reserve one request for an orderly recording close, inside the total cap.
        if counts["requests"] >= limits["requests"]-1: raise StopTrial("timeout", "request_limit")
        if state and op in ("act", "advance") and state["mutations"] >= limits["mutations"]:
            raise StopTrial("timeout", "mutation_limit")
        last_request = dict(op=op, **fields)
        counts["requests"] += 1
        response = client.request(op, **fields)
        if not response["ok"]:
            code = "scenario_rejected" if op == "start" else "protocol_rejected"
            raise StopTrial("runner_failure" if op == "start" else "policy_failure", code+": "+str(response["error"]))
        data = response["data"]
        observed = data if op == "start" else data.get("observation")
        if observed is not None:
            validate_observation(observed)
            state = observed
            row["diagnostic_inspection_used"] |= bool(state.get("diagnostic_inspection_used"))
            metrics.observe(state)
        if data.get("stop") in ("tick_limit", "mutation_limit") or data.get("reason") == "mutation_limit":
            raise StopTrial("timeout", str(data.get("stop", data.get("reason"))))
        return data

    def advance(ticks, until):
        remaining = limits["ticks"]-state["ticks"]
        if remaining <= 0: raise StopTrial("timeout", "simulation_tick_limit")
        before = state["ticks"]
        data = call("advance", ticks=min(ticks, remaining, 1000), until_decision=until)
        log.write(dict(op="advance", tick=before, requested=min(ticks, remaining, 1000),
                       until_decision=until, actual=data["ticks"], stop=data["stop"]))
        if data["ticks"] == 0 and not state["decision"] and not state["terminal"]:
            raise StopTrial("stall", "advance_zero_progress")

    try:
        log = DecisionLog(decisions, limits["sidecar_bytes"])
        client = Client(runner, assets, spec["mode"], trace, metrics.event,
                        timeout=limits["request_seconds"], startup_timeout=limits["startup_seconds"],
                        deadline=deadline, cancelled=cancelled)
        call("start", scenario=spec["scenario"])
        descriptors = call("actions")
        row["coverage"] = coverage(state, descriptors, spec["policy"]["kind"])
        timings["startup_seconds"] = time.monotonic()-started
        loop_started = time.monotonic()
        if row["coverage"]["unsupported"]:
            raise StopTrial("unsupported_decision", "active_loadout: "+", ".join(row["coverage"]["unsupported"]))
        while True:
            status = terminal_status(state)
            if status: raise StopTrial(status, "terminal_"+state["phase"])
            if cancelled and cancelled(): raise StopTrial("cancelled", "user_interrupt")
            if time.monotonic() >= deadline: raise StopTrial("timeout", "trial_or_batch_wall_limit")
            if state["ticks"] >= limits["ticks"]: raise StopTrial("timeout", "simulation_tick_limit")
            if counts["decisions"] >= limits["decisions"]: raise StopTrial("timeout", "decision_limit")
            reason = watch.check(state, metrics.rounds, metrics.encounters)
            if reason: raise StopTrial("stall", reason)
            if state["phase"] == "victory":
                before = state["ticks"]
                result = call("act", action={"kind": "continue"})
                if not result["accepted"]: raise StopTrial("policy_failure", "continue_rejected")
                counts["actions"] += 1
                log.write(dict(op="act", tick=before, action={"kind": "continue"}, reason="partial_victory"))
                interval = spec["reaction_ticks"]
                next_action = (state["ticks"]//interval+1)*interval if interval else state["ticks"]
                continue
            real_time = state["phase"] in ("exploration", "approach")
            if state["moving"] or (not real_time and not state["awaiting_action"]):
                advance(1000, True)
                continue
            interval = spec["reaction_ticks"]
            if real_time and interval and state["ticks"] > next_action:
                next_action = ((state["ticks"]+interval-1)//interval)*interval
            if real_time and state["ticks"] < next_action:
                advance(next_action-state["ticks"], False)
                continue
            counts["decisions"] += 1
            reason = watch.check(state, metrics.rounds, metrics.encounters, decision=True)
            if reason: raise StopTrial("stall", reason)
            descriptors = call("actions")
            # Read-only discovery and previews are within one frozen observation revision.
            t = time.monotonic()
            view = policy_view(state)
            candidates = iter(policy.candidates(view, descriptors))
            timings["policy_seconds"] += time.monotonic()-t
            selected = None; rejections = Counter(); tried = 0
            while True:
                t = time.monotonic()
                candidate = next(candidates, None)
                timings["policy_seconds"] += time.monotonic()-t
                if candidate is None: break
                if tried >= limits["previews"]:
                    raise StopTrial("unsupported_decision", "candidate_search_limit")
                tried += 1; counts["previews"] += 1
                preview = call("preview", action=candidate.action)
                if preview["accepted"]:
                    selected = candidate; break
                counts["rejected_previews"] += 1
                rejections[preview["reason"]] += 1
            counts["max_decision_previews"] = max(counts["max_decision_previews"], tried)
            tick = state["ticks"]
            phase = state["phase"]
            if selected is not None:
                data = call("act", action=selected.action)
                if not data["accepted"]:
                    raise StopTrial("policy_failure", "accepted_preview_then_rejected_act: "+data["reason"])
                counts["actions"] += 1
                metrics.actions[selected.action.get("spell", selected.action.get("feature", selected.action.get("action", selected.action["kind"])))] += 1
                metrics.reasons[selected.reason] += 1
                policy.accepted(selected)
                log.write(dict(op="act", tick=tick, phase=phase, action=selected.action, reason=selected.reason,
                               previews=tried, rejected=dict(rejections)))
                if real_time: next_action = tick+spec["reaction_ticks"]
            else:
                log.write(dict(op="wait", tick=tick, reason=policy.wait_reason, previews=tried, rejected=dict(rejections)))
                if not real_time:
                    raise StopTrial("unsupported_decision", "awaiting_turn_without_supported_action")
                # Explicit ticks avoid the settled-exploration zero-decision loop.
                advance(max(1, spec["reaction_ticks"]), False)
                next_action = state["ticks"]
    except StopTrial as exc:
        row.update(status=exc.status, reason=exc.reason)
    except Cancelled as exc:
        row.update(status="cancelled", reason=str(exc))
    except DeadlineExceeded as exc:
        row.update(status="timeout", reason=str(exc))
    except (RunnerError, ProtocolError, ClientError) as exc:
        row.update(status="runner_failure", reason=type(exc).__name__+": "+str(exc))
    except OSError as exc:
        row.update(status="runner_failure", reason="io_failure: "+str(exc))
        row["artifact_errors"].append(str(exc))
    except Exception as exc:
        row.update(status="policy_failure", reason=type(exc).__name__+": "+str(exc))
    finally:
        if loop_started: timings["play_loop_seconds"] = time.monotonic()-loop_started
        if client is not None:
            # End responsive failure recordings cleanly. Expired deadlines/cancellation do
            # not get extra simulation time; close is the only allowed cleanup request.
            if not client.closed and not client.failed:
                old_deadline, old_cancelled = client.deadline, client.cancelled
                client.deadline, client.cancelled = time.monotonic()+0.5, None
                try: client.request("close", _timeout=0.5)
                except Exception as exc: row["artifact_errors"].append("close: "+str(exc))
                finally: client.deadline, client.cancelled = old_deadline, old_cancelled
            client.dispose()
            counts["requests"] = client.sequence
            row["runner_exit"] = client.process.returncode
            if client.process.returncode:
                row["artifact_errors"].append(f"runner_exit_{client.process.returncode}")
            row["runner_stderr_tail"] = client.error_tail()[-8192:]
            timings["protocol_seconds"] = client.request_seconds
            row["runner_peak_rss_kib"] = client.peak_rss_kib
            row["protocol_bytes"] = dict(read=client.bytes_read, written=client.bytes_written)
        if log:
            try: log.close()
            except OSError as exc: row["artifact_errors"].append("sidecar_close: "+str(exc))
            row["decision_hash"] = log.hash.hexdigest()
            row["decision_bytes"] = log.bytes
    header, complete, size = trace_metadata(trace)
    row["reason"] = row["reason"][:4096]
    row.update(engine_identity=header, trace_complete=complete, trace_bytes=size,
               counts=counts, metrics=metrics.result(), last_request=last_request,
               timings=dict(timings, total_seconds=time.monotonic()-started))
    if not complete: row["artifact_errors"].append("incomplete_engine_trace")
    if state:
        row.update(ticks=state["ticks"], simulation_ms=int(state["time_ms"]), gameplay_ms=int(state["gameplay_ms"]),
                   phase=state["phase"], terminal=state["terminal"], final_state_hash=digest(policy_view(state)),
                   last_progress=dict(tick=watch.last_tick, rounds=watch.last_round))
    else:
        row.update(ticks=0, simulation_ms=0, gameplay_ms=0, phase=None, terminal=False, final_state_hash=None)
    row["balance_eligible"] = (row["status"] in ("victory", "combat_loss") and not row["artifact_errors"]
                               and not (row["mode"] == "visual" and row["diagnostic_inspection_used"]))
    return row
