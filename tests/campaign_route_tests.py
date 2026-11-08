"""Fast owned route-plumbing checks; never launch a campaign experiment."""
import copy
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from types import SimpleNamespace
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from campaign_route_review import Route, path_step
from arena_experiments.trial import StopTrial


def state():
    return dict(position=[1, 1], generation=dict(map="#######\n#.....#\n#.....#\n#######\n"),
                fixtures=[], enemies=[])


class RouteTests(unittest.TestCase):
    def test_actual_tiles_and_occupancy(self):
        s = state()
        s["enemies"] = [dict(position=[2, 1], health_fraction=1)]
        self.assertEqual(path_step(s, [4, 1]), (1, 2))
        self.assertIsNone(path_step(s, [2, 1], True))
        s["fixtures"] = [dict(kind=5, position=[3, 1], open=False),
                         dict(kind=5, position=[3, 2], open=False)]
        with self.assertRaises(StopTrial):
            path_step(s, [4, 1])
        s["fixtures"][1]["open"] = True
        self.assertEqual(path_step(s, [4, 1]), (1, 2))

    def test_stairs_blocked_and_door_is_interactable(self):
        s = state()
        s["fixtures"] = [dict(kind=3, position=[2, 1]), dict(kind=0, position=[1, 2], open=False)]
        self.assertEqual(path_step(s, [4, 1]), (1, 2))

    def test_blocking_dressing_is_not_a_walkable_corridor(self):
        s = state()
        s["generation"]["dressing"] = [dict(cell=[2, 1], blocking=True)]
        self.assertEqual(path_step(s, [4, 1]), (1, 2))

    def test_transfer_not_consumption(self):
        s = dict(party=[dict(inventory=[dict(item="mana_potion", quantity=2)]), dict(inventory=[])])
        transferred = copy.deepcopy(s)
        transferred["party"].reverse()
        self.assertEqual(Route.quantities(s), Route.quantities(transferred))

    def test_epoch_resets_burst_window_without_resetting_costs(self):
        r = Route.__new__(Route)
        from arena_experiments.trial import Metrics
        r.metrics, r.journal, r.state = Metrics(), [], None
        r.metrics.incoming[0] = 9
        r.metrics.windows[0].append((100, 9))
        r.metrics.window_damage[0] = 9
        r.events(dict(events=[dict(event="campaign_transition")]))
        self.assertEqual(r.metrics.incoming[0], 9)
        self.assertEqual(r.metrics.window_damage[0], 0)
        self.assertFalse(r.metrics.windows[0])

    def test_tactics_rebuilds_after_reaction_cadence_enters_contact(self):
        r = Route.__new__(Route)
        r.state = dict(phase="exploration", ticks=0, awaiting_action=False, party=[dict(level=1)]*4)
        r.next_action, r.coverage = 25, {}
        r.settle = lambda: None
        r.advance = lambda *_: r.state.update(phase="contact", ticks=25)
        r.call = lambda _: dict(phase=r.state["phase"])
        observed = []
        def candidates(view, descriptors):
            observed.append((view["phase"], descriptors["phase"]))
            return iter(())
        r.policy = SimpleNamespace(candidates=candidates, wait_reason="none")
        with patch("campaign_route_review.coverage", return_value=dict(unsupported=[])), patch("campaign_route_review.policy_view", side_effect=lambda s:s):
            self.assertFalse(r.tactics())
        self.assertEqual(observed, [("contact", "contact")])

    def test_empty_encounter_drains_presentation_tail_with_real_time(self):
        r = Route.__new__(Route)
        r.state = dict(phase="approach")
        r.settle = lambda: None
        r.living = lambda _: []
        r.tactics = lambda: False
        calls = []
        def advance(ticks, decision):
            calls.append((ticks, decision))
            r.state["phase"] = "exploration"
        r.advance = advance
        r.fight(dict(first=0, count=2))
        self.assertEqual(calls, [(25, False)])
        self.assertFalse(r.expected)

    def test_realtime_action_cannot_execute_on_new_contact_turn(self):
        r = Route.__new__(Route)
        r.state = dict(phase="exploration", ticks=0)
        r.next_action = 25
        r.advance = lambda *_: r.state.update(phase="contact", ticks=10)
        self.assertFalse(r.act(dict(kind="move", direction="forward")))


if __name__ == "__main__":
    unittest.main()
