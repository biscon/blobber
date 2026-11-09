"""Owned one-HP, harmless-enemy protocol fixture; no production content or balance claims."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from campaign_route_review import Route
from arena_experiments.trial import StopTrial

exe = Path(sys.argv[1])
city_entry_only = "--city-entry" in sys.argv
full_clear = "--full-clear" in sys.argv
scenario = json.loads(subprocess.check_output([str(exe), "--scenario"], text=True))
with tempfile.TemporaryDirectory(prefix="blobber-route-") as name:
    directory = Path(name)
    route = Route(exe, directory, directory, scenario, full_clear, time.monotonic()+(60 if full_clear else 30), record=False)
    try:
        if city_entry_only:
            route.call("start", scenario=scenario)
            assert route.state["position"] == [11, 8]
            route.stairs(3)
            assert route.state["campaign"]["location"] == 1
            assert route.state["campaign"]["visited_entries"] == [1]
        else:
            route.run()
    except StopTrial as exc:
        if exc.status != "victory":
            print(json.dumps(dict(position=route.state["position"], phase=route.state["phase"], ticks=route.state["ticks"],
                                  rests=route.rests, recent=route.journal[-15:]), indent=2), file=sys.stderr)
            raise RuntimeError((exc.status, exc.reason))
    finally:
        route.close()
    if not city_entry_only:
        assert len(route.checkpoints) == 3
        assert all(p[0] >= 300 for p in route.checkpoints[-1]["xp"])
        assert route.rests <= 3
        assert route.state["campaign"]["status"] == "active"
        if full_clear:
            assert route.state["party"][0]["xp"] == 793
            assert route.state["party"][0]["xp_fraction"] == 9
print("Owned city entry route passed" if city_entry_only else "Owned scripted campaign route passed")
