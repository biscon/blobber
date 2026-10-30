"""EXPLICIT integration playtest; deliberately not registered with CTest.

Uses generated fixtures, completes one win/loss, verifies engine replay and policy
rerun, and exercises bounded failure/cancellation. Not a balance assertion.
"""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from headless_cli_tests import fixture
from arena_experiments.config import resolve
from arena_experiments.batch import run_batch, rerun
from arena_experiments.report import rows
from arena_experiments.policies import Candidate, combat


def configuration(scenario, **limits):
    return resolve(dict(version=1, parties=[dict(id="fixture",scenario=scenario)],
                        rosters=[dict(id="enemy",monsters=scenario["monsters"])],
                        policies=[dict(id="focus",kind="focus",version=1)], seeds=[5],
                        limits=limits, retention="all"), ".", embedded=True)


def main(runner):
    runner = str(Path(runner).resolve())
    with tempfile.TemporaryDirectory(prefix="blobber-132-playtest-") as tmp:
        root=Path(tmp); assets=root/"assets"; assets.mkdir()
        scenario=fixture(assets)
        c=configuration(scenario)
        result=run_batch(c,runner,assets,root/"win",progress=lambda _:None)
        assert result["counts"]["victory"]==1,result
        row=next(rows(root/"win"))
        decisions=[json.loads(line) for line in (root/"win"/row["decisions"]).read_text().splitlines()]
        realtime=[d["tick"] for d in decisions if d["op"]=="act" and d.get("phase") in ("exploration","approach")]
        assert realtime and realtime[0]==25 and all(t%25==0 for t in realtime),realtime
        assert all(b-a>=25 for a,b in zip(realtime,realtime[1:])),realtime
        replay=subprocess.run([runner,"--assets",str(assets),"--replay",str(root/"win"/row["trace"])],capture_output=True,text=True,timeout=30)
        assert replay.returncode==0,(replay.stdout,replay.stderr)
        again=rerun(root/"win",row["id"],root/"again",progress=lambda _:None)
        assert again["rerun"]["matched"],again["rerun"]
        # The test owns its content. A dangerous enemy must produce an actual defeat,
        # not a timeout, without changing live state through a backdoor command.
        catalog=json.loads((assets/"rules/catalog.json").read_text())
        catalog["monsters"][0]["hit_points"]=10000
        catalog["attacks"][0]["attack_bonus"]=100
        catalog["attacks"][0]["damage"]["modifier"]=1000
        (assets/"rules/catalog.json").write_text(json.dumps(catalog))
        loss=run_batch(c,runner,assets,root/"loss",progress=lambda _:None)
        assert loss["counts"]["combat_loss"]==1,loss
        row=next(rows(root/"loss"))
        replay=subprocess.run([runner,"--assets",str(assets),"--replay",str(root/"loss"/row["trace"])],capture_output=True,text=True,timeout=30)
        assert replay.returncode==0,(replay.stdout,replay.stderr)
        # Strict reconstruction must reject content changed after the original win.
        try:
            rerun(root/"win",next(rows(root/"win"))["id"],root/"bad-replay",progress=lambda _:None)
        except ValueError as exc: assert "content changed" in str(exc)
        else: raise AssertionError("Content mismatch was accepted")
        short=configuration(scenario,ticks=1)
        bounded=run_batch(short,runner,assets,root/"bounded",progress=lambda _:None)
        assert bounded["counts"]["timeout"]==1 and bounded["counts"]["combat_loss"]==0,bounded
        tiny=configuration(scenario,sidecar_bytes=1)
        exhausted=run_batch(tiny,runner,assets,root/"sidecar",progress=lambda _:None)
        assert exhausted["artifact_failures"]==1 and exhausted["batch"]["status"]=="incomplete",exhausted
        # Cancellation after trial start; the callable does not sleep or inject input.
        def cancel():
            return any(p.stat().st_size for p in (root/"cancelled"/"decisions").glob("*.jsonl"))
        cancelled=run_batch(c,runner,assets,root/"cancelled",cancelled=cancel,progress=lambda _:None)
        assert cancelled["batch"]["status"]=="cancelled",cancelled
        assert cancelled["counts"]["combat_loss"]==0 and cancelled["counts"]["cancelled"]==1,cancelled
        with patch("arena_experiments.trial.Policy.candidates", side_effect=RuntimeError("fixture policy fault")):
            failed=run_batch(c,runner,assets,root/"policy-failure",progress=lambda _:None)
        assert failed["counts"]["policy_failure"]==1 and failed["counts"]["combat_loss"]==0,failed
        row=next(rows(root/"policy-failure"))
        assert row["trace_complete"],row
        replay=subprocess.run([runner,"--assets",str(assets),"--replay",str(root/"policy-failure"/row["trace"])],capture_output=True,text=True,timeout=30)
        assert replay.returncode==0,(replay.stdout,replay.stderr)
        def rejected_candidates(_observation,_descriptors):
            # A legal payload out of range at the safe spawn; two candidates with
            # a one-preview budget exercise unsupported search, not combat defeat.
            return iter([Candidate(combat(0,"melee",target=0),"fixture_search")]*2)
        with patch("arena_experiments.trial.Policy.candidates", side_effect=rejected_candidates):
            unsupported=run_batch(configuration(scenario,previews=1),runner,assets,root/"unsupported",progress=lambda _:None)
        assert unsupported["counts"]["unsupported_decision"]==1 and unsupported["counts"]["combat_loss"]==0,unsupported
        assert next(rows(root/"unsupported"))["reason"]=="candidate_search_limit"
        print("Explicit playtest passed: fixture win/loss, engine replay, policy repeatability, content mismatch, budgets, artifact exhaustion and cancellation.")


if __name__=="__main__": main(sys.argv[1])
