"""Small deterministic level-one policies. Candidates are preferences, never rules authority."""
from collections import deque
from dataclasses import dataclass

CONSCIOUS, DYING, STABLE, DEAD = range(4)  # Accepted protocol v1 / rules::Life.
SUPPORTED_SPELLS = frozenset((
    "sacred_flame", "spare_the_dying", "bless", "bane", "shield_of_faith", "cure_wounds",
    "guiding_bolt", "inflict_wounds", "fire_bolt", "acid_splash", "mage_armor", "magic_missile",
    "burning_hands", "thunderwave", "ray_of_frost", "shocking_grasp", "guidance"))
DISABLED = ["rest", "loot", "inventory_management", "preparation_changes", "advancement"]
DIRECTIONS = ((0, -1), (1, 0), (0, 1), (-1, 0))


@dataclass(frozen=True)
class Candidate:
    action: dict
    reason: str


def combat(character, action, **fields):
    return dict(kind="combat", character=character, action=action, **fields)


def statuses(member):
    return {s["name"] for s in member.get("statuses", [])}


def coverage(observation, descriptors, kind):
    entries, unsupported = [], []
    for p, d in zip(observation["party"], descriptors["combat"]):
        if p["level"] != 1:
            unsupported.append(f"character {p['character']}: level {p['level']}")
        for s in d["spells"]:
            state = "implemented" if s["id"] in SUPPORTED_SPELLS else "unsupported"
            if state == "unsupported": unsupported.append(s["id"])
            elif kind == "focus" and s["cost"] > 0 and s["hostile"]:
                state = "intentionally_unused_offensive_mana"
            elif kind == "focus" and s["id"] in ("mage_armor", "bless", "bane", "shield_of_faith", "guidance"):
                state = "intentionally_unused_buff"
            entries.append(dict(character=p["character"], ability=s["id"], coverage=state))
        # Discovery lists all feature names, including ones this class cannot use.
        for f in d["features"]:
            name = f["id"]
            state = "unavailable_at_level_or_class"
            if name == "end_turn": state = "implemented"
            elif name == "dismiss_concentration": state = "implemented_when_concentrating"
            elif name == "second_wind" and p["class"] == "fighter": state = "implemented"
            elif name == "arcane_recovery" and p["class"] == "wizard": state = "implemented"
            elif f["eligibility"]["ready"]:
                state = "unsupported"
                unsupported.append(name)
            entries.append(dict(character=p["character"], ability=name, coverage=state))
    return dict(abilities=entries, unsupported=sorted(set(unsupported)), disabled=DISABLED)


def policy_view(observation):
    """Whitelist the policy input. Never pass trace state or diagnostic event payloads."""
    keys = ("phase", "terminal", "position", "heading", "moving", "ticks", "round", "gameplay_ms",
            "awaiting_action", "resolving_action", "can_end_turn", "active_character", "formation", "mode")
    result = {k: observation[k] for k in keys}
    party_keys = ("character", "class", "race", "level", "hp", "max_hp", "mana", "max_mana", "life",
                  "death_successes", "death_failures", "statuses", "concentration", "cooldown_ms", "spent",
                  "second_wind", "arcane_recovery", "inventory", "armor_class", "temporary_hp")
    result["party"] = [{k: p[k] for k in party_keys} for p in observation["party"]]
    enemy_keys = ["actor", "type", "position", "subcell", "health_fraction", "statuses", "targetable"]
    if observation["mode"] == "diagnostic": enemy_keys += ["hp", "max_hp"]
    result["enemies"] = [{k: e[k] for k in enemy_keys} for e in observation["enemies"]
                         if e["health_fraction"] > 0]
    return result


class Policy:
    def __init__(self, config):
        self.kind = config["kind"]
        self.params = config["parameters"]
        self.focus = None
        self.opened = set()
        self.visits = set()
        self.navigation_goal = None
        self.wait_reason = "no_legal_candidate"

    def accepted(self, candidate):
        action = candidate.action
        if "target" in action: self.focus = action["target"]
        elif action.get("enemies"): self.focus = action["enemies"][0]
        if candidate.reason.startswith("opening_"):
            self.opened.add((action["character"], action["spell"]))

    def enemies(self, o):
        x, y = o["position"]
        return sorted(o["enemies"], key=lambda e: (e["actor"] != self.focus,
                      e["health_fraction"], abs(e["position"][0]-x)+abs(e["position"][1]-y), e["actor"]))

    def candidates(self, o, descriptors):
        """Incremental search; the driver caps previews and records exhaustion explicitly."""
        self.wait_reason = "no_legal_candidate"
        party = o["party"]
        real_time = o["phase"] in ("exploration", "approach")
        actors = [p for p in party if p["life"] == CONSCIOUS and
                  (real_time or p["character"] == o["active_character"])]
        if real_time:
            actors = [p for p in actors if int(p["cooldown_ms"]) == 0]
        desc = {d["character"]: d for d in descriptors["combat"]}
        spells = {i: {s["id"]: s for s in d["spells"]} for i, d in desc.items()}
        enemies = self.enemies(o)
        # Broad distance shortlist, not a legality replacement: every cast is previewed.
        x, y = o["position"]
        visible = [e for e in enemies if e["targetable"] and
                   (e["position"][0]-x)**2+(e["position"][1]-y)**2 <= 36]
        threshold = self.params["heal_fraction"]
        allies = sorted((p for p in party if p["life"] != DEAD and
                         (p["life"] in (DYING, STABLE) or p["hp"] <= p["max_hp"]*threshold)),
                        key=lambda p: (p["life"] == CONSCIOUS, -p["death_failures"],
                                       p["hp"]/p["max_hp"], o["formation"].index(p["character"])))
        # Rescue first across the whole party, not per-actor attacks before another actor's heal.
        for ally in allies:
            for p in actors:
                i, a = p["character"], ally["character"]
                if i == a and p["second_wind"]:
                    yield Candidate(combat(i, "feature", feature="second_wind"), "heal_second_wind")
                if "cure_wounds" in spells[i] and p["mana"] >= spells[i]["cure_wounds"]["cost"]:
                    yield Candidate(combat(i, "spell", spell="cure_wounds", allies=[a]), "heal_ally")
                for item in p["inventory"]:
                    if item["item"] == "healing_potion" and item["cell"] < 48:
                        yield Candidate(combat(i, "item", item=item["handle"], ally=a), "heal_potion")
            if ally["life"] == DYING:
                for p in actors:
                    i = p["character"]
                    if "spare_the_dying" in spells[i]:
                        yield Candidate(combat(i, "spell", spell="spare_the_dying", allies=[ally["character"]]), "stabilize")
                    if i != ally["character"]:
                        yield Candidate(combat(i, "aid", ally=ally["character"]), "aid_stabilize")
        # Waking a sleeping ally is useful independently of hit point thresholds.
        for ally in party:
            if "Sleep" in statuses(ally):
                for p in actors:
                    if p["character"] != ally["character"]:
                        yield Candidate(combat(p["character"], "aid", ally=ally["character"]), "aid_wake")
        if self.kind != "focus":
            for p in actors:
                i = p["character"]
                known = spells[i]
                if "mage_armor" in known and (i, "mage_armor") not in self.opened:
                    for ally in party:
                        if ally["life"] == CONSCIOUS and "Mage Armor" not in statuses(ally) and not any(
                                item["cell"] == 48 for item in ally["inventory"]):
                            yield Candidate(combat(i, "spell", spell="mage_armor", allies=[ally["character"]]), "opening_armor")
                # Guidance is a single early initiative aid; don't keep refreshing it.
                if real_time and not visible and not p["concentration"] and "guidance" in known and (i, "guidance") not in self.opened:
                    yield Candidate(combat(i, "spell", spell="guidance", allies=[o["formation"][0]]), "opening_guidance")
                if self.kind == "conservative" and visible and not p["concentration"]:
                    if "bless" in known and p["mana"] >= known["bless"]["cost"] + self.params["reserve_mana"]:
                        targets = [a for a in o["formation"] if party[a]["life"] == CONSCIOUS and "Bless" not in statuses(party[a])][:3]
                        if targets:
                            yield Candidate(combat(i, "spell", spell="bless", allies=targets), "support_bless")
                    if "shield_of_faith" in known and p["mana"] >= known["shield_of_faith"]["cost"] + self.params["reserve_mana"]:
                        for a in o["formation"][:2]:
                            if party[a]["life"] == CONSCIOUS and "Shield of Faith" not in statuses(party[a]):
                                yield Candidate(combat(i, "spell", spell="shield_of_faith", allies=[a]), "support_armor")
                # End an expiring initiative-only concentration before a useful combat buff.
                if visible and self.kind == "conservative" and any(s["name"] == "Guidance" for s in p["concentration"]):
                    if "bless" in known and p["mana"] >= known["bless"]["cost"] + self.params["reserve_mana"]:
                        yield Candidate(combat(i, "feature", feature="dismiss_concentration"), "replace_guidance")
        # Recover mana only when it enables a useful rescue or offensive cast.
        for target in party:
            i = target["character"]
            need_heal = bool(allies) and "cure_wounds" in spells[i]
            need_attack = self.kind == "aggressive" and bool(visible) and any(s["hostile"] and s["cost"] for s in spells[i].values())
            preparing = (self.kind != "focus" and o["phase"] == "exploration" and not visible
                         and target["max_mana"]-target["mana"] >= 2
                         and any(s["hostile"] and s["cost"] for s in spells[i].values()))
            if target["life"] != CONSCIOUS or target["mana"] == target["max_mana"]:
                continue
            if target in actors and target["arcane_recovery"] and (preparing or ((need_heal or need_attack) and target["mana"] < 2)):
                yield Candidate(combat(i, "feature", feature="arcane_recovery"), "recover_arcane")
            if not (need_heal or need_attack) or target["mana"] >= 2:
                continue
            for p in actors:
                for item in p["inventory"]:
                    if item["item"] == "mana_potion" and item["cell"] < 48:
                        yield Candidate(combat(p["character"], "item", item=item["handle"], ally=i), "recover_potion")
        for p in actors:
            i = p["character"]
            known = spells[i]
            if self.kind == "aggressive":
                front = self.front_enemies(o, visible)
                if len(front) >= 2:
                    for spell in ("burning_hands", "thunderwave"):
                        if spell in known and p["mana"] >= known[spell]["cost"]:
                            yield Candidate(combat(i, "spell", spell=spell), "area_damage")
                if len(visible) >= 3 and not p["concentration"] and "bane" in known and p["mana"] >= known["bane"]["cost"]:
                    targets = [e["actor"] for e in visible if "Bane" not in statuses(e)][:3]
                    if len(targets) >= 2:
                        yield Candidate(combat(i, "spell", spell="bane", enemies=targets), "multi_control")
                for e in visible:
                    for spell in ("magic_missile", "guiding_bolt", "inflict_wounds"):
                        if spell not in known or p["mana"] < known[spell]["cost"]: continue
                        if spell == "magic_missile":
                            # Split only across very hurt targets; no hidden HP estimate in visual mode.
                            hurt = [x["actor"] for x in visible if x["health_fraction"] <= 0.25][:3]
                            if len(hurt) > 1:
                                yield Candidate(combat(i, "spell", spell=spell, enemies=hurt,
                                                       darts=[1]*(len(hurt)-1)+[4-len(hurt)]), "split_darts")
                            yield Candidate(combat(i, "spell", spell=spell, enemies=[e["actor"]], darts=[3]), "focus_darts")
                        else:
                            yield Candidate(combat(i, "spell", spell=spell, enemies=[e["actor"]]), "paid_damage")
            for e in visible:
                builders = (self.cantrips, self.weapons) if p["class"] in ("cleric", "wizard") else (self.weapons, self.cantrips)
                for builder in builders:
                    yield from builder(p, e, visible, known, desc[i])
        if real_time:
            # Preparation spends normal cooldowns. Let them settle before walking
            # into threat range; otherwise safe Arcane Recovery can never be used.
            prepared = {i for i, _spell in self.opened}
            if o["phase"] == "exploration" and not visible and any(
                    p["character"] in prepared and int(p["cooldown_ms"]) > 0 for p in party):
                self.wait_reason = "preparation_cooldown"
                return
            yield from self.navigation(o, enemies)
        elif o["awaiting_action"]:
            i = o["active_character"]
            # A normal action blocked by targets may defensively wait; optional allowances end explicitly.
            if not o["can_end_turn"]:
                yield Candidate(combat(i, "defend"), "defend_no_attack")
            yield Candidate(combat(i, "feature", feature="end_turn"), "end_turn")

    @staticmethod
    def weapons(p, enemy, _visible, _known, described):
        # Discovery filters target eligibility; the driver still previews each payload.
        for action in ("melee", "back_melee", "ranged"):
            d = next((a for a in described["actions"] if a["action"] == action), {})
            if any(t["actor"] == enemy["actor"] and t["ready"] for t in d.get("targets", [])):
                yield Candidate(combat(p["character"], action, target=enemy["actor"]), "focus_weapon")

    @staticmethod
    def cantrips(p, enemy, visible, known, _described):
        i = p["character"]
        if "acid_splash" in known:
            pair = [x["actor"] for x in visible if x["position"] == enemy["position"]][:2]
            if len(pair) == 2:
                yield Candidate(combat(i, "spell", spell="acid_splash", enemies=pair), "paired_cantrip")
        for spell in ("fire_bolt", "sacred_flame", "ray_of_frost", "shocking_grasp", "acid_splash"):
            if spell in known and p["mana"] >= known[spell]["cost"]:
                yield Candidate(combat(i, "spell", spell=spell, enemies=[enemy["actor"]]), "focus_cantrip")

    @staticmethod
    def front_enemies(o, enemies):
        x, y = o["position"]
        dx, dy = DIRECTIONS[o["heading"]]
        return [e for e in enemies if (e["position"][0]-x)*dx+(e["position"][1]-y)*dy == 1
                and abs((e["position"][0]-x)*(-dy)+(e["position"][1]-y)*dx) <= 1]

    def navigation(self, o, enemies):
        start = tuple(o["position"])
        self.visits.add(start)
        blocked = {tuple(e["position"]) for e in enemies}
        if enemies:
            nearest = min(enemies, key=lambda e: (abs(e["position"][0]-start[0])+abs(e["position"][1]-start[1]), e["actor"]))
            tx, ty = nearest["position"]
            dx, dy = tx-start[0], ty-start[1]
            if max(abs(dx), abs(dy)) <= 1:
                heading = (2 if dy > 0 else 0) if dy else (1 if dx > 0 else 3)
                yield from self.turn(o["heading"], heading)
                return
            goals = {(tx+dx, ty+dy) for dx, dy in DIRECTIONS}
        else:
            # Scan the known room, not the hidden placement table. FIFO ties prefer north/east.
            goals = {(x, y) for y in range(1, 24) for x in range(1, 20)} - self.visits
            if not goals:
                # One full scan is enough to request a diagnostic stall, not claim victory.
                return
        queue = deque([start]); previous = {start: None}; goal = None
        while queue:  # At most 437 traversable cells in layout v1.
            cell = queue.popleft()
            if cell in goals and cell not in blocked:
                goal = cell; break
            for dx, dy in DIRECTIONS:
                nxt = (cell[0]+dx, cell[1]+dy)
                if 1 <= nxt[0] < 20 and 1 <= nxt[1] < 24 and nxt not in previous and nxt not in blocked:
                    previous[nxt] = cell; queue.append(nxt)
        if goal is None or goal == start: return
        step = goal
        while previous[step] != start: step = previous[step]
        heading = DIRECTIONS.index((step[0]-start[0], step[1]-start[1]))
        if o["heading"] != heading:
            yield from self.turn(o["heading"], heading)
        else:
            yield Candidate({"kind": "move", "direction": "forward"}, "approach" if enemies else "search_room")

    @staticmethod
    def turn(current, target):
        delta = (target-current) % 4
        if delta:
            yield Candidate({"kind": "move", "direction": "turn_left" if delta == 3 else "turn_right"}, "face_target")
