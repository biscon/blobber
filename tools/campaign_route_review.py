#!/usr/bin/env python3
"""Explicit PC07.2 diagnostic routes, not an autonomous explorer or routine test."""
import argparse
from collections import Counter, deque
import copy
import hashlib
import json
from pathlib import Path
import shutil
import time

from headless_client import Client, ClientError
from arena_experiments.policies import Policy, coverage, policy_view
from arena_experiments.trial import Metrics, StopTrial, trace_metadata, resources

MIB = 1024 * 1024
DIRECTIONS = ((0, -1), (1, 0), (0, 1), (-1, 0))
SUPPLIES = {"healing_potion", "mana_potion", "arrow", "camp_kit"}
POLICY = dict(kind="aggressive", version=2, parameters=dict(heal_fraction=.25, reserve_mana=0))


def path_step(state, goal, adjacent=False):
    """Diagnostic final tiles; previews remain the movement/door authority."""
    start = tuple(state["position"])
    if start == tuple(goal) and not adjacent:
        return None
    rows = state["generation"]["map"].splitlines()
    blocked = {tuple(f["position"]) for f in state["fixtures"] if f["kind"] in (2, 3, 5)}
    blocked.update(tuple(e["position"]) for e in state["enemies"] if e["health_fraction"] > 0)
    # Completed guardian gates are ordinary traversable cells.
    blocked.difference_update(tuple(f["position"]) for f in state["fixtures"] if f["kind"] == 5 and f["open"])
    goals = {tuple(goal)} if not adjacent else {(goal[0]+dx, goal[1]+dy) for dx, dy in DIRECTIONS}
    if start in goals:
        return None
    queue, parents = deque([start]), {start: None}
    found = None
    while queue:
        here = queue.popleft()
        if here in goals:
            found = here
            break
        for dx, dy in DIRECTIONS:
            to = here[0]+dx, here[1]+dy
            if (0 <= to[1] < len(rows) and 0 <= to[0] < len(rows[0]) and rows[to[1]][to[0]] != "#"
                    and to not in blocked and to not in parents):
                parents[to] = here
                queue.append(to)
    if found is None:
        raise StopTrial("policy_failure", "no_diagnostic_path")
    while parents[found] != start:
        found = parents[found]
    return found


class Route:
    def __init__(self, runner, assets, directory, scenario, reserve, deadline):
        self.directory, self.reserve = directory, reserve
        self.scenario = scenario
        self.deadline = min(time.monotonic()+120, deadline)
        self.metrics, self.policy = Metrics(), Policy(POLICY)
        self.state = None
        self.next_action = 25
        self.decisions = 0
        self.journal = []
        self.checkpoints = []
        self.consumed, self.collected = Counter(), Counter()
        self.steps = self.retraced = self.rests = 0
        self.visited = set()
        self.expected = set()
        self.merges = []
        self.coverage = {}
        self.client = Client(runner, assets, "diagnostic", directory/"trace.jsonl", self.events,
                             deadline=self.deadline)

    def events(self, chunk):
        self.metrics.event(chunk)
        for event in chunk["events"]:
            if event["event"] == "campaign_transition":
                self.metrics.windows = [deque() for _ in range(4)]
                self.metrics.window_damage = [0]*4
                self.metrics.round_damage = [0]*4
                self.metrics.previous_time = None
            if event["event"] == "started" and self.state:
                self.journal.append(dict(event="combat_started", location=self.state["campaign"]["location"],
                                         expected=sorted(self.expected)))

    def call(self, op, **fields):
        if time.monotonic() >= self.deadline:
            raise StopTrial("timeout", "route_wall_limit")
        if self.client.sequence >= 99999:
            raise StopTrial("timeout", "request_limit")
        response = self.client.request(op, **fields)
        if not response["ok"]:
            raise StopTrial("runner_failure", str(response["error"]))
        data = response["data"]
        state = data if op == "start" else data.get("observation")
        if state is not None:
            self.state = state
            self.metrics.observe(state)
            if state["ticks"] >= 60000:
                raise StopTrial("timeout", "simulation_tick_limit")
            if self.expected and state["phase"] != "exploration":
                unexpected = sorted(e["actor"] for e in state["enemies"] if e["health_fraction"] > 0
                                    and e.get("awareness") in (1, 2, 3) and e["actor"] not in self.expected)
                if unexpected and (not self.merges or self.merges[-1]["actors"] != unexpected):
                    self.merges.append(dict(location=state["campaign"]["location"], actors=unexpected,
                                            tick=state["ticks"]))
            if state["mutations"] >= 20000:
                raise StopTrial("timeout", "mutation_limit")
            if state["campaign"]["status"] == "defeat":
                raise StopTrial("combat_loss", "campaign_defeat")
        return data

    def advance(self, ticks=1000, decision=True):
        self.call("advance", ticks=min(ticks, 60000-self.state["ticks"]), until_decision=decision)

    @staticmethod
    def quantities(state):
        result = Counter()
        for p in state["party"]:
            for item in p["inventory"]:
                result[item["item"]] += item["quantity"]
        return result

    def act(self, action, required=True):
        realtime = self.state["phase"] in ("exploration", "approach")
        if realtime and self.state["ticks"] < self.next_action:
            self.advance(self.next_action-self.state["ticks"], False)
            if self.state["phase"] not in ("exploration", "approach"):
                return False
        if action["kind"] in ("interact", "pickup", "rest", "advancement"):
            # Movement can be legal during presentation tails; management must wait.
            for _ in range(12):
                cooldown = max(int(p["cooldown_ms"]) for p in self.state["party"])
                if not cooldown or self.state["phase"] != "exploration":
                    break
                self.advance(min(100, (cooldown+9)//10), False)
        self.decisions += 1
        if self.decisions > 20000:
            raise StopTrial("timeout", "decision_limit")
        preview = self.call("preview", action=action)
        if not preview["accepted"]:
            if required:
                raise StopTrial("policy_failure", "rejected_action: "+preview["reason"])
            return False
        before = self.quantities(self.state)
        at = dict(tick=self.state["ticks"], location=self.state["campaign"]["location"],
                  phase=self.state["phase"], action=action, operation=self.client.sequence+1)
        result = self.call("act", action=action)
        if not result["accepted"]:
            raise StopTrial("policy_failure", "preview_then_rejected: "+result["reason"])
        after = self.quantities(self.state)
        if action["kind"] == "pickup":
            self.collected.update(after-before)
        elif action["kind"] in ("combat", "rest", "recall"):
            self.consumed.update(before-after)
        self.journal.append(at)
        self.metrics.actions[action.get("spell", action.get("feature", action.get("action", action["kind"])))] += 1
        self.next_action = self.state["ticks"]+25
        return True

    def settle(self):
        for _ in range(200):
            if self.state["campaign"]["status"] == "victory":
                raise StopTrial("victory", "campaign_victory")
            if self.state["phase"] == "victory":
                self.act(dict(kind="continue"))
            elif (self.state["moving"] or self.state["door_opening"] or
                  (self.state["phase"] not in ("exploration", "approach") and not self.state["awaiting_action"])):
                self.advance()
            else:
                return
        raise StopTrial("stall", "settle_limit")

    def tactics(self):
        self.settle()
        if self.state["phase"] in ("exploration", "approach") and self.state["ticks"] < self.next_action:
            self.advance(self.next_action-self.state["ticks"], False)
            self.settle()
        described = self.call("actions")
        checked = coverage(self.state, described, "aggressive", 2)
        self.coverage[",".join(str(p["level"]) for p in self.state["party"])] = checked
        if checked["unsupported"]:
            raise StopTrial("unsupported_decision", str(checked["unsupported"]))
        for n, candidate in enumerate(self.policy.candidates(policy_view(self.state), described)):
            if n >= 256:
                raise StopTrial("unsupported_decision", "candidate_limit")
            if candidate.action["kind"] == "move":
                continue  # Arena navigation never drives a generated map.
            if self.act(candidate.action, False):
                self.policy.accepted(candidate)
                return True
        if self.policy.wait_reason == "preparation_cooldown":
            self.advance(25, False)
            return True
        if self.state["awaiting_action"]:
            raise StopTrial("unsupported_decision", "contact_without_supported_action")
        return False

    def face(self, heading):
        for _ in range(12):
            self.settle()
            if self.state["heading"] == heading:
                return
            if self.state["phase"] not in ("exploration", "approach"):
                self.tactics()
            else:
                self.act(dict(kind="move", direction="turn_right"))
        raise StopTrial("stall", "cannot_face_waypoint")

    def goto(self, goal, adjacent=False, actor=None):
        for _ in range(2000):
            self.settle()
            if self.tactics():
                continue
            if actor is not None:
                current = next(e for e in self.state["enemies"] if e["actor"] == actor)
                if current["health_fraction"] <= 0:
                    return
                goal = current["position"]
            step = path_step(self.state, goal, adjacent)
            if step is None:
                return
            here = self.state["position"]
            heading = DIRECTIONS.index((step[0]-here[0], step[1]-here[1]))
            self.face(heading)
            door = next((f for f in self.state["fixtures"] if tuple(f["position"]) == step and f["kind"] == 0), None)
            if door and not door["open"]:
                self.act(dict(kind="interact", fixture=door["fixture"]))
                continue
            if self.act(dict(kind="move", direction="forward"), False):
                location = self.state["campaign"]["location"]
                marker = (location, *step)
                self.retraced += marker in self.visited
                self.visited.add(marker)
                self.steps += 1
            else:
                self.advance(25, False)
        raise StopTrial("stall", "waypoint_limit")

    def living(self, encounter):
        members = range(encounter["first"], encounter["first"]+encounter["count"])
        return [e for e in self.state["enemies"] if e["actor"] in members and e["health_fraction"] > 0]

    def fight(self, encounter):
        self.expected = set(range(encounter["first"], encounter["first"]+encounter["count"]))
        for _ in range(2500):
            self.settle()
            living = self.living(encounter)
            if not living and self.state["phase"] == "exploration":
                self.expected.clear()
                return
            if self.tactics():
                continue
            if not living:
                self.advance(25, False)
                continue
            enemy = min(living, key=lambda e: abs(e["position"][0]-self.state["position"][0])+
                                             abs(e["position"][1]-self.state["position"][1]))
            target = enemy["position"]
            self.goto(target, True, enemy["actor"])
            delta = target[0]-self.state["position"][0], target[1]-self.state["position"][1]
            if delta in DIRECTIONS:
                self.face(DIRECTIONS.index(delta))
            self.advance(25, False)
        raise StopTrial("stall", "encounter_limit")

    def loot(self):
        for _ in range(80):
            self.settle()
            picked = False
            for loot in self.state["loot"]:
                if loot.get("item") not in SUPPLIES:
                    continue
                for character in range(4):
                    if self.act(dict(kind="pickup", loot=loot["handle"], character=character), False):
                        picked = True
                        break
                if picked:
                    break
            if not picked:
                return
        raise StopTrial("stall", "pickup_limit")

    def advance_party(self):
        for character in range(4):
            p = self.state["party"][character]
            if p["level"] >= 2:
                continue
            action = dict(kind="advancement", character=character, prepared=[],
                          learned=["false_life", "color_spray"] if p["class"] == "wizard" else [])
            self.act(action, False)

    def stairs(self, kind):
        fixture = next(f for f in self.state["fixtures"] if f["kind"] == kind)
        dx, dy = DIRECTIONS[fixture["heading"]] if "heading" in fixture else (0, 0)
        # Observation fixtures do not expose facing; generation does. Hub is authored east.
        if self.state["campaign"]["location"] == 0:
            heading = 1
        else:
            heading = next(f["heading"] for f in self.state["generation"]["fixtures"] if f["id"] == fixture["fixture"])
        dx, dy = DIRECTIONS[heading]
        landing = fixture["position"][0]-dx, fixture["position"][1]-dy
        if self.state["campaign"]["location"] != 0:
            self.goto(landing)
        self.face(heading)
        self.act(dict(kind="interact", fixture=fixture["fixture"]))
        self.metrics.previous_time = None
        self.settle()

    def run(self):
        self.call("start", scenario=self.scenario)
        self.stairs(3)
        for depth in (1, 2, 3):
            layout = copy.deepcopy(self.state["generation"])
            rooms = {r["id"]: r for r in layout["rooms"]}
            groups = {e["room"]: e for e in layout["encounters"]}
            for room in layout["route"][1:-1]:
                encounter = groups.get(room)
                if encounter:
                    self.fight(encounter)
                self.goto(rooms[room]["anchor"])
                for fixture in self.state["fixtures"]:
                    if fixture["kind"] == 1 and fixture["position"] == rooms[room]["anchor"] and not fixture["completed"]:
                        self.act(dict(kind="interact", fixture=fixture["fixture"]))
                self.loot()
                self.advance_party()
            if self.reserve:
                encounter = next(e for e in layout["encounters"] if e["role"] == "optional")
                self.fight(encounter)
                self.goto(layout["supplies"]["optional"])
                self.loot()
                self.advance_party()
            self.goto(layout["supplies"]["direct"])
            self.loot()
            self.advance_party()
            pre_recovery = resources(self.state)
            rest = dict(kind="rest", type="long")
            preview = self.call("preview", action=rest)
            if preview["accepted"]:
                self.act(rest)
                self.rests += 1
                self.policy.opened.clear()
            elif not preview["reason"].startswith("No recovery needed."):
                raise StopTrial("recovery_required", preview["reason"])
            self.advance_party()
            self.checkpoints.append(dict(depth=depth, pre_recovery=pre_recovery, resources=resources(self.state),
                xp=[(p["xp"], p.get("xp_fraction")) for p in self.state["party"]],
                campaign_ms=self.state["campaign"]["gameplay_ms"], operation=self.client.sequence, consumed=dict(self.consumed), collected=dict(self.collected)))
            if depth == 3 and any(p["life"] == 0 and p["level"] < 2 for p in self.state["party"]):
                raise StopTrial("progression_shortfall", "conscious member below level two before final guardian")
            guardian = next(e for e in layout["encounters"] if e["role"] == "guardian")
            approach = next(f for f in layout["fixtures"] if f["id"] == layout["objective"]["approach_door"])
            self.face(approach["heading"])
            self.act(dict(kind="interact", fixture=approach["id"]))
            self.fight(guardian)
            self.goto(layout["objective"]["reward"])
            self.loot()
            self.advance_party()
            self.stairs(3)
        raise StopTrial("policy_failure", "missing_campaign_victory")

    def close(self):
        if not self.client.closed and not self.client.failed:
            old = self.client.deadline
            self.client.deadline = time.monotonic()+.5
            try:
                self.client.request("close", _timeout=.5)
            finally:
                self.client.deadline = old
        self.client.dispose()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case", type=int, choices=range(1, 9), required=True,
                        help="Run one declared case, in order; inspect its result before the next")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=args.case != 1)
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root/"docs/pc071_calibration/bugbear_support/manifest.json").read_text())
    previous = json.loads((args.output/"summary.json").read_text()) if args.case > 1 else []
    if len(previous) != args.case-1 or any(r["status"] not in ("victory", "combat_loss") or not r["trace_complete"] for r in previous):
        raise ValueError("Previous cases must complete in order; do not silently rerun or expand the study")
    provenance = dict(binary_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(),
                      script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                      runner=str(args.runner.resolve()), assets=str(args.assets.resolve()), policy=POLICY,
                      limit_seconds=120, limit_ticks=60000, study_seconds=1800, output_bytes=3*1024*MIB,
                      code={str(p.relative_to(root/"tools")): hashlib.sha256(p.read_bytes()).hexdigest()
                            for p in [root/"tools/headless_client.py", *sorted((root/"tools/arena_experiments").glob("*.py"))]},
                      content={str(p.relative_to(args.assets)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in [args.assets/"rules/catalog.json", args.assets/"npcs/level.json",
                                         *[args.assets/"npcs"/n for n in json.loads((args.assets/"npcs/level.json").read_text())["definitions"]]]})
    if args.case == 1:
        provenance["deadline_wall"] = time.time()+1800
        (args.output/"provenance.json").write_text(json.dumps(provenance, indent=2)+"\n")
    else:
        saved = json.loads((args.output/"provenance.json").read_text())
        provenance["deadline_wall"] = saved["deadline_wall"]
        if saved != provenance:
            raise ValueError("Study binary/content/tool identity changed; preserve evidence and stop")
    deadline = time.monotonic()+max(0, provenance["deadline_wall"]-time.time())
    rows = previous
    index = 0
    for package in manifest["parties"]:
        for seed in (5, 8):
            for reserve in (False, True):
                index += 1
                if index != args.case:
                    continue
                used = sum(p.stat().st_size for p in args.output.rglob("*") if p.is_file())
                if used+300*MIB > 3*1024*MIB or shutil.disk_usage(args.output).free < 300*MIB or time.monotonic() >= deadline:
                    return 2
                name = f"{package['id']}-{seed}-{'reserve' if reserve else 'direct'}"
                directory = args.output/name
                directory.mkdir()
                scenario = dict(kind="campaign", schema_version=1, generator_version=7, template_version=5,
                                seed=seed, party=package["scenario"]["party"])
                (directory/"scenario.json").write_text(json.dumps(scenario, indent=2)+"\n")
                route = Route(args.runner, args.assets, directory, scenario, reserve, deadline)
                started = time.monotonic()
                status, reason = "runner_failure", "not_started"
                try:
                    route.run()
                except StopTrial as exc:
                    status, reason = exc.status, exc.reason
                except KeyboardInterrupt:
                    status, reason = "cancelled", "user_interrupt"
                except (ClientError, OSError, ValueError, KeyError) as exc:
                    status, reason = "runner_failure", repr(exc)
                finally:
                    try:
                        route.close()
                    except Exception as exc:
                        status, reason = "runner_failure", "close: "+repr(exc)
                _, complete, size = trace_metadata(directory/"trace.jsonl")
                metrics = route.metrics.result()
                metrics["consumed_items"] = dict(route.consumed)
                result = dict(id=name, status=status, reason=reason, seconds=time.monotonic()-started,
                              trace_complete=complete, trace_bytes=size, checkpoints=route.checkpoints,
                              metrics=metrics, coverage=route.coverage, rests=route.rests, steps=route.steps, retraced_steps=route.retraced, merges=route.merges,
                              final=route.state)
                (directory/"result.json").write_text(json.dumps(result, indent=2)+"\n")
                (directory/"decisions.jsonl").write_text("".join(json.dumps(x)+"\n" for x in route.journal))
                rows.append({k: v for k, v in result.items() if k != "final"})
                (args.output/"summary.json").write_text(json.dumps(rows, indent=2)+"\n")
                print(name, status, reason, f"{result['seconds']:.2f}s", flush=True)
                if status not in ("victory", "combat_loss") or not complete:
                    return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
