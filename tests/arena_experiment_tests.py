"""Fast fixture-owned tooling contracts. No game, assets, autonomous fight or matrix."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from headless_client import Client, DeadlineExceeded, Cancelled, ProtocolError, RunnerError
from arena_experiments.config import resolve, read_json, trials, DEFAULT_LIMITS, digest
from arena_experiments.policies import Policy, combat, coverage, policy_view
from arena_experiments.trial import ProgressWatch, terminal_status, Metrics, DecisionLog, ArtifactError
from arena_experiments.report import aggregate, keep_traces, wilson


def observation():
    party = []
    for i, cls in enumerate(("fighter", "cleric", "rogue", "wizard")):
        party.append(dict(character=i, name=f"Fixture {i}", **{"class":cls}, race="human", level=1,
            hp=10, max_hp=10, mana=4 if i in (1,3) else 0, max_mana=4 if i in (1,3) else 0,
            life=0, death_successes=0, death_failures=0, statuses=[], concentration=[], cooldown_ms="0", spent=False,
            second_wind=i==0, arcane_recovery=i==3, inventory=[], armor_class=14, temporary_hp=0,
            hit_dice=1, abilities=[10]*6, base_abilities=[10]*6, cantrips=[], prepared=[], domain=[], racial=[]))
    enemies = [dict(actor=i, type="fixture", position=[10,9], subcell=[i%2,i//2], health_fraction=1.,
                    statuses=[], targetable=True, hp=8, max_hp=8) for i in range(3)]
    return dict(party=party, enemies=enemies, phase="turns", terminal=False, position=[10,10], heading=0,
                moving=False, ticks=0, round="1", gameplay_ms="0", time_ms="0", awaiting_action=True,
                resolving_action=False, can_end_turn=False, active_character=0, formation=[0,1,2,3], mode="diagnostic")


def descriptors(spells=()):
    result = {"combat":[]}
    for i in range(4):
        result["combat"].append(dict(character=i, spells=[dict(id=s, cost=cost, hostile=hostile) for s,cost,hostile in spells] if i in (1,3) else [],
            features=[dict(id=f, eligibility={"ready":False}) for f in ("second_wind","arcane_recovery","dismiss_concentration","action_surge","end_turn")],
            actions=[dict(action=a, targets=[{"actor":j,"ready":True} for j in range(3)]) for a in ("melee","back_melee","ranged")]))
    return result


def policy(kind="focus"):
    from arena_experiments.config import POLICY_DEFAULTS
    return Policy(dict(kind=kind, parameters=POLICY_DEFAULTS[kind]))


def config():
    scenario=dict(schema_version=1,layout_version=1,name="Fixture",seed=1,
                  party=[dict(id=f"p{i}",level=1) for i in range(4)],monsters=[dict(monster="fixture",quantity=1)])
    return resolve(dict(version=1,parties=[dict(id="a",scenario=scenario),dict(id="b",scenario=scenario)],
                        rosters=[dict(id="r",monsters=[dict(monster="fixture",quantity=1)])],
                        policies=[dict(id="focus",kind="focus",version=1)],seeds=[1,2,3,4], comparisons=[["a","b"]]), ".", embedded=True)


class PolicyContracts(unittest.TestCase):
    def test_heal_unconscious_before_attack(self):
        o=observation(); o["active_character"]=1; o["party"][2].update(hp=0,life=1)
        choices=list(policy().candidates(o,descriptors([("cure_wounds",2,False),("spare_the_dying",0,False)])))
        self.assertEqual(choices[0].action, combat(1,"spell",spell="cure_wounds",allies=[2]))
        self.assertIn("stabilize",[c.reason for c in choices])

    def test_no_dead_or_full_health_heals(self):
        o=observation(); o["active_character"]=1; o["party"][2].update(hp=0,life=3)
        choices=list(policy().candidates(o,descriptors([("cure_wounds",2,False)])))
        self.assertFalse(any(c.reason.startswith("heal") for c in choices))

    def test_second_wind_before_owned_potion(self):
        o=observation(); o["party"][0]["hp"]=1
        o["party"][0]["inventory"]=[dict(item="healing_potion",cell=0,quantity=1,handle=dict(slot=7,generation="3"))]
        choices=list(policy().candidates(o,descriptors()))
        self.assertEqual(choices[0].action["feature"],"second_wind")
        potion=next(c for c in choices if c.reason=="heal_potion")
        self.assertEqual(potion.action["item"]["generation"],"3")

    def test_compound_payloads(self):
        o=observation(); o["active_character"]=3
        for e in o["enemies"]: e["health_fraction"]=.2
        choices=list(policy("aggressive").candidates(o,descriptors([("magic_missile",2,True),("acid_splash",0,True)])))
        darts=next(c.action for c in choices if c.reason=="split_darts")
        self.assertEqual(sum(darts["darts"]),3); self.assertEqual(len(darts["enemies"]),3)
        self.assertTrue(all(d>0 for d in darts["darts"]))
        pair=next(c.action for c in choices if c.reason=="paired_cantrip")
        self.assertEqual(len(pair["enemies"]),2)

    def test_front_area_requires_multiple_front_enemies(self):
        o=observation(); o["active_character"]=3
        d=descriptors([("burning_hands",2,True)])
        self.assertEqual(next(policy("aggressive").candidates(o,d)).reason,"area_damage")
        for e in o["enemies"]: e["position"]=[10,12]
        self.assertFalse(any(c.reason=="area_damage" for c in policy("aggressive").candidates(o,d)))

    def test_concentration_preserved(self):
        o=observation(); o["active_character"]=1; o["party"][1]["concentration"]=[{"name":"Shield of Faith"}]
        d=descriptors([("bless",2,False),("shield_of_faith",2,False)])
        self.assertFalse(any(c.action.get("spell") in ("bless","shield_of_faith") for c in policy("conservative").candidates(o,d)))

    def test_optional_turn_ends(self):
        o=observation(); o["can_end_turn"]=True; o["enemies"]=[]
        choices=list(policy().candidates(o,descriptors()))
        self.assertEqual(choices[-1].action["feature"],"end_turn")
        self.assertFalse(any(c.action.get("action")=="defend" for c in choices))

    def test_cooldown_uses_navigation_not_attack(self):
        o=observation(); o.update(phase="approach",awaiting_action=False,active_character=None)
        for p in o["party"]: p["cooldown_ms"]="1000"
        for e in o["enemies"]: e["position"]=[10,7]
        choices=list(policy().candidates(o,descriptors()))
        self.assertTrue(choices); self.assertTrue(all(c.action["kind"]=="move" for c in choices))

    def test_arcane_recovery_after_preparation(self):
        o=observation(); o.update(phase="exploration",awaiting_action=False,active_character=None)
        o["enemies"]=[]; o["party"][3]["mana"]=2
        choices=list(policy("aggressive").candidates(o,descriptors([("magic_missile",2,True)])))
        recovery=next(c for c in choices if c.reason=="recover_arcane")
        self.assertEqual(recovery.action["character"],3)
        o["party"][3]["mana"]=4
        self.assertFalse(any(c.reason=="recover_arcane" for c in policy("aggressive").candidates(o,descriptors([("magic_missile",2,True)]))))

    def test_preparation_waits_for_normal_cooldown(self):
        o=observation(); o.update(phase="exploration",awaiting_action=False,active_character=None)
        o["enemies"]=[]
        o["party"][3].update(mana=2,cooldown_ms="6000")
        p=policy("aggressive"); p.opened.add((3,"mage_armor"))
        self.assertEqual(list(p.candidates(o,descriptors([("magic_missile",2,True)]))),[])
        self.assertEqual(p.wait_reason,"preparation_cooldown")
        o["party"][3]["cooldown_ms"]="0"
        self.assertEqual(next(p.candidates(o,descriptors([("magic_missile",2,True)]))).reason,"recover_arcane")

    def test_coverage_active_grants_only(self):
        o=observation(); o["party"][3]["learned"]=["sleep"]
        covered=coverage(o,descriptors([("shocking_grasp",0,True)]),"focus")
        self.assertEqual(covered["unsupported"],[])
        covered=coverage(o,descriptors([("sleep",2,True)]),"focus")
        self.assertEqual(covered["unsupported"],["sleep"])

    def test_policy_view_strips_privileged_fields(self):
        o=observation(); o["attacks"]=[{"roll":20}]; o["enemies"][0]["awareness"]=2
        view=policy_view(o)
        self.assertNotIn("attacks",view); self.assertNotIn("awareness",view["enemies"][0])
        o["mode"]="visual"; view=policy_view(o)
        self.assertNotIn("hp",view["enemies"][0])

    def test_casters_prefer_cantrips_to_a_backup_melee_weapon(self):
        o=observation(); o["active_character"]=3
        first=next(policy().candidates(o,descriptors([("fire_bolt",0,True)])))
        self.assertEqual(first.action["spell"],"fire_bolt")

    def test_navigation_faces_nearby_enemy_and_is_repeatable(self):
        o=observation(); o.update(phase="approach",awaiting_action=False,heading=2)
        p=policy(); q=policy()
        self.assertEqual(list(p.navigation(o,o["enemies"])),list(q.navigation(o,o["enemies"])))
        self.assertEqual(next(p.navigation(o,o["enemies"])).action["direction"],"turn_right")


class DataContracts(unittest.TestCase):
    def test_matrix_identity_and_roster_order(self):
        c=config(); items=list(trials(c,"code"))
        self.assertEqual(len(items),8)
        self.assertEqual(len({s["id"] for s in items}),8)
        other=copy.deepcopy(c); other["rosters"][0]["monsters"].append(dict(monster="other",quantity=1))
        a=list(trials(other,"code"))[0]["id"]
        other["rosters"][0]["monsters"].reverse()
        self.assertNotEqual(a,list(trials(other,"code"))[0]["id"])

    def test_strict_limits_and_duplicates(self):
        for key,value in (("seeds",[1,1]),("reaction_ticks",True),("limits",{"ticks":360001}),("unknown",1)):
            c=config(); c[key]=value
            with self.assertRaises(ValueError): resolve(c,".",embedded=True)
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"bad.json"; p.write_text('{"version":1,"version":1}')
            with self.assertRaises(ValueError): read_json(p)
            p.write_text('{"x": NaN}')
            with self.assertRaises(ValueError): read_json(p)

    def test_partial_victory_and_stall_not_defeat(self):
        o=observation(); o.update(phase="victory",terminal=False)
        self.assertIsNone(terminal_status(o))
        o["terminal"]=True; self.assertEqual(terminal_status(o),"victory")
        o["phase"]="defeat"; self.assertEqual(terminal_status(o),"combat_loss")
        o=observation(); watch=ProgressWatch(dict(DEFAULT_LIMITS,stall_decisions=3))
        self.assertIsNone(watch.check(o,0,1,True))
        o["ticks"]=100; o["round"]="2"
        self.assertIsNone(watch.check(o,1,1,True))
        self.assertEqual(watch.check(o,2,1,True),"repeated_decisions_without_progress")
        o["party"][0]["hp"]-=1
        self.assertIsNone(watch.check(o,2,1,True))

    def test_resource_gross_spending_and_recovery(self):
        o=observation(); metrics=Metrics(); metrics.observe(o)
        o["party"][1]["mana"]=2; metrics.observe(o)
        o["party"][1]["mana"]=4; metrics.observe(o)
        self.assertEqual(metrics.result()["mana_spent"][1],2)
        self.assertEqual(metrics.result()["mana_recovered"][1],2)

    def test_sidecar_exclusive_and_bound(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"decisions.jsonl"; log=DecisionLog(p,100)
            with self.assertRaises(FileExistsError): DecisionLog(p,100)
            log.write({"action":"wait"})
            with self.assertRaises(ArtifactError): log.write({"action":"x"*100})
            log.close(); self.assertEqual(len(p.read_text().splitlines()),1)

    @staticmethod
    def row(i,status,eligible=True,party="a"):
        o=observation(); metrics=Metrics(); metrics.observe(o)
        return dict(id=str(i),party=party,roster="r",policy="focus",seed=i,mode="diagnostic",reaction_ticks=25,
                    status=status,reason=status,balance_eligible=eligible,ticks=i*10,trace=f"traces/{i}.jsonl",
                    decisions=f"decisions/{i}.jsonl",trace_complete=True,engine_identity={"binary":"same"},
                    artifact_errors=[],timings={"total_seconds":1},simulation_ms=1000,metrics=metrics.result())

    def test_denominators_and_missing_pairs(self):
        data=[self.row(1,"victory"),self.row(2,"combat_loss"),self.row(3,"stall",False),self.row(4,"policy_failure",False),self.row(1,"victory",party="b")]
        data[-1]["id"]="b1"
        s=aggregate(data,config()); g=s["groups"][0]
        self.assertEqual(g["victory_rate"],.5); self.assertEqual(g["wins_per_started"],.25)
        self.assertEqual(g["valid_completions"],2); self.assertEqual(g["counts"]["stall"],1)
        self.assertEqual(s["comparisons"][0]["paired"],1); self.assertIsNone(s["comparisons"][0]["bootstrap_95"])
        empty=aggregate([self.row(1,"timeout",False)],config())["groups"][0]
        self.assertIsNone(empty["victory_rate"]); self.assertIsNone(empty["simulation_seconds"])
        self.assertIsNone(wilson(0,0)); self.assertLess(wilson(8,8)[0],1)

    def test_retention_keeps_failures_and_deterministic_extremes(self):
        rows=[self.row(i,"victory") for i in (2,1,3,4)]+[self.row(5,"combat_loss"),self.row(6,"timeout",False)]
        self.assertEqual(keep_traces(rows),{"1","2","4","5","6"})
        rows[0]["engine_identity"]={"binary":"other"}
        with self.assertRaises(ValueError): aggregate(rows,config())

    def test_paired_uncertainty_is_repeatable_and_uses_valid_pairs(self):
        c=config(); c["seeds"]=list(range(1,25))
        data=[]
        for seed in c["seeds"]:
            for party in ("a","b"):
                row=self.row(seed,"victory" if (seed%3 or party=="b") else "combat_loss",party=party)
                row["id"]=party+str(seed); data.append(row)
        first=aggregate(data,c)["comparisons"]
        self.assertEqual(first,aggregate(data,c)["comparisons"])
        self.assertEqual(first[0]["paired"],24)
        self.assertIsNotNone(first[0]["bootstrap_95"])
        data[-1]["code_identity"]="changed"
        data[0]["code_identity"]="original"
        with self.assertRaises(ValueError): aggregate(data,c)


class TransportContracts(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)

    def fake(self, body):
        p=Path(self.tmp.name)/"runner"
        p.write_text("#!/usr/bin/env python3\nimport sys,json,time,os\n"+body)
        p.chmod(0o700); return p

    def test_identity_events_stderr_and_cleanup(self):
        script='''for line in sys.stdin:
 r=json.loads(line)
 os.write(2,b"x"*65536)
 if r['op']=='act':
  assert r['session']=='8' and r['revision']=='2'
  print(json.dumps(dict(version=1,type='events',id=r['id'],events=[{'event':'fixture'}])))
 data={'session':'8','revision':'2'} if r['op']=='start' else {'closed':True}
 print(json.dumps(dict(version=1,type='response',id=r['id'],ok=True,data=data)),flush=True)
 if r['op']=='close': break
'''
        events=[]
        with Client(self.fake(script),".",on_event=events.append,timeout=2) as c:
            c.request("start"); c.request("act",action={})
        self.assertEqual(len(events),1); self.assertEqual(c.process.returncode,0)
        self.assertTrue(c.stderr)

    def test_timeout_partial_line_and_reap(self):
        p=self.fake("sys.stdin.readline()\nos.write(1,b'{')\ntime.sleep(30)\n")
        c=Client(p,".",startup_timeout=.05)
        with self.assertRaises(DeadlineExceeded):
            with c: c.request("start")
        self.assertIsNotNone(c.process.poll())

    def test_blocked_write_deadline(self):
        c=Client(self.fake("time.sleep(30)\n"),".",startup_timeout=.05)
        with self.assertRaises(DeadlineExceeded):
            with c: c.request("start",scenario="x"*1000000)
        self.assertIsNotNone(c.process.poll())

    def test_cancel_and_oversized_response(self):
        c=Client(self.fake("time.sleep(30)\n"),".",cancelled=lambda:True)
        with self.assertRaises(Cancelled):
            with c: c.request("start")
        self.assertIsNotNone(c.process.poll())
        c=Client(self.fake("sys.stdin.readline()\nos.write(1,b'x'*1024)\ntime.sleep(30)\n"),".")
        c.MAX_LINE=100
        with self.assertRaises(ProtocolError):
            with c: c.request("start")

    def test_wrong_event_id_rejects(self):
        c=Client(self.fake("sys.stdin.readline()\nprint(json.dumps(dict(version=1,type='events',id='wrong',events=[])),flush=True)\n"),".")
        with self.assertRaises(ProtocolError):
            with c: c.request("start")


if __name__ == "__main__": unittest.main()
