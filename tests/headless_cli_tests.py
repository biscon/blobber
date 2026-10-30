"""Fixture-owned protocol/process checks. No game content or display required."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def fixture(root):
    (root / "rules").mkdir()
    (root / "npcs").mkdir()
    abilities = dict(zip(("strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"), (15,14,13,12,10,8)))
    zeros = {k: 0 for k in abilities}
    catalog = {
        "schema_version": 9,
        "attacks": [{"id":"fixture.hit","kind":"melee","attack_bonus":-100,
            "damage":{"count":1,"sides":4,"modifier":0,"type":"slashing"}}],
        "monsters":[{"id":"fixture.enemy","abilities":{k:10 for k in abilities},"armor_class":10,"hit_points":5,
            "attacks":["fixture.hit"],"traits":[],"loot":[],"metal_armor":False,"sleep_immune":False,"experience":1200,"humanoid":True}],
        "races":[{"id":"human","name":"Fixture human","ability_increases":zeros,"hit_points_per_level":0,
            "weapons":[],"sleep_immune":False,"wizard_cantrip":False}],
        "classes":[{"id":"fixture.fighter","name":"Fixture fighter","hit_die":10,"saves":["strength","constitution"],
            "weapons":["longsword"],"armor":[],"allows_defense":False,"casting":"none","features":[],"spells":[],"progression":"fighter"}],
        "items":[{"id":"fixture.sword","name":"Fixture sword","description":"Dedicated test fixture","weight":100,
            "kind":"weapon","weapon":"longsword","damage":{"count":1,"sides":8,"type":"slashing"},
            "finesse":False,"ranged":False,"two_handed":False,"versatile_sides":10,"stack_limit":1,"ammunition":"none"}],
        "spells":[],"preset_party":None,
        "creation":[{"class":"fixture.fighter","recommended":abilities,
            "groups":[[{"id":"sword","name":"Sword","items":[{"item":"fixture.sword","quantity":1,"slot":"main_hand"}]}]],"supplies":[]}],
    }
    catalog["items"].append({"id":"camp_kit","name":"Fixture kit","description":"Dedicated test supply","kind":"supply","weight":1,"stack_limit":5})
    (root/"rules/catalog.json").write_text(json.dumps(catalog))
    (root/"npcs/level.json").write_text(json.dumps({"definitions":["enemy.json"],"instances":[]}))
    definition={"id":"fixture.enemy","width":0.45,"height":0.65,
                "views":{side:"intentionally-absent.png" for side in ("front","back","left","right")}}
    (root/"npcs/enemy.json").write_text(json.dumps(definition))
    party=[]
    for i in range(4):
        party.append({"id":f"fixture.{i}","name":f"Fixture {i}","race":"human","class":"fixture.fighter",
            "portrait":"alden","abilities":list(abilities.values()),"level":1,"defense":False,"life_domain":False,
            "spells":{key:[] for key in ("cantrips","learned","prepared","domain","racial")},"choices":["sword"],
            "gear":{"items":[{"item":"fixture.sword","quantity":1}],"equipment":{"body":None,"main_hand":0,"off_hand":None,"ranged":None}}})
    return {"schema_version":1,"layout_version":1,"name":"CLI fixture","seed":5,"party":party,
            "monsters":[{"monster":"fixture.enemy","quantity":1}]}


def main(executable):
    env=os.environ.copy()
    env.pop("DISPLAY",None); env.pop("WAYLAND_DISPLAY",None)
    with tempfile.TemporaryDirectory(prefix="blobber-cli-") as tmp:
        root=Path(tmp); scenario=fixture(root); trace=root/"run.jsonl"
        command=[executable,"--assets",str(root),"--mode","diagnostic"]
        with subprocess.Popen(command+["--record",str(trace)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,text=True,env=env) as process:
            state=None; sequence=0; event_count=0
            def call(op,**kwargs):
                nonlocal state,sequence,event_count
                request={"version":1,"id":str(sequence),"op":op,**kwargs}; sequence+=1
                if state is not None and op in ("act","preview","advance","reset","start"):
                    request.update(session=state["session"],revision=state["revision"])
                process.stdin.write(json.dumps(request)+"\n"); process.stdin.flush()
                while True:
                    line=process.stdout.readline()
                    assert line, process.stderr.read()
                    response=json.loads(line)
                    assert response["type"]!="fatal",response
                    if response["type"]=="events": event_count+=len(response["events"]); continue
                    assert response["id"]==request["id"],response
                    assert response["ok"],response
                    data=response["data"]
                    if "observation" in data: state=data["observation"]
                    elif "party" in data: state=data
                    return data
            # Raw malformed input stays a recoverable transport error.
            process.stdin.write('{bad\n');process.stdin.flush()
            assert json.loads(process.stdout.readline())["error"]["code"]=="invalid_json"
            call("start",scenario=scenario)
            assert state["party"][0]["level"]==1 and state["time_ms"]=="0"
            for _ in range(1000):
                if state["terminal"]: break
                if state["awaiting_action"]:
                    actor=state["active_character"]
                    attack={"kind":"combat","character":actor,"action":"melee","target":0}
                    if call("preview",action=attack)["accepted"]:
                        assert call("act",action=attack)["accepted"]
                    else:
                        assert call("act",action={"kind":"combat","character":actor,"action":"defend"})["accepted"]
                elif state["phase"] in ("exploration","approach") and not state["moving"]:
                    assert call("act",action={"kind":"move","direction":"forward"})["accepted"]
                else: call("advance",ticks=1000,until_decision=True)
            assert state["terminal"] and state["phase"]=="victory",state
            assert event_count>0
            old=state["session"]; call("reset"); assert state["session"]!=old
            call("close"); process.stdin.close()
            assert process.wait(timeout=15)==0,process.stderr.read()
        result=subprocess.run(command+["--replay",str(trace)],capture_output=True,text=True,env=env,timeout=30)
        assert result.returncode==0,(result.stdout,result.stderr)
        assert json.loads(result.stdout)["replay"]=="matched"
        refusal=subprocess.run(command+["--record",str(trace)],input="",capture_output=True,text=True,env=env,timeout=15)
        assert refusal.returncode!=0 and "exclusively" in refusal.stderr
        original=trace.read_text(); truncated=root/"incomplete.jsonl"
        truncated.write_text("\n".join(original.splitlines()[:-1])+"\n")
        result=subprocess.run(command+["--replay",str(truncated)],capture_output=True,text=True,env=env,timeout=30)
        assert result.returncode!=0 and "Incomplete" in result.stderr
        scenario_path=root/"scenario.json"
        scenario_path.write_text(json.dumps(scenario))
        client=Path(__file__).resolve().parents[1]/"tools/headless_client.py"
        result=subprocess.run([sys.executable,str(client),"--runner",executable,"--assets",str(root),
                               "--scenario",str(scenario_path)],
                              input='{"op":"act","action":{"kind":"move","direction":"forward"}}\n'
                                    '{"op":"advance","ticks":1000,"until_decision":true}\n'
                                    '{"op":"reset"}\n',capture_output=True,text=True,env=env,timeout=15)
        assert result.returncode==0,(result.stdout,result.stderr)
        responses=[json.loads(line) for line in result.stdout.splitlines()]
        assert len(responses)==4 and all(r["ok"] for r in responses),responses
        assert responses[-1]["data"]["session"]!=responses[0]["data"]["session"]
        # Content changes reject before a replay starts, even when the change is only whitespace.
        with (root/"rules/catalog.json").open("a") as stream: stream.write("\n")
        result=subprocess.run(command+["--replay",str(trace)],capture_output=True,text=True,env=env,timeout=15)
        assert result.returncode!=0 and "header" in result.stderr
        print("Headless CLI: fixture-only complete encounter, recording, fresh-process replay and failure paths passed")

if __name__=="__main__": main(sys.argv[1])
