#!/usr/bin/env python3
"""Explicit Release transport/recording benchmark; not a test-suite or balance run.

Replay a retained trial's requests through the existing client, or exercise a
scenario with bounded movement/stepping. Compare record on/off with identical
requests and visible-output hashes. Does not claim pure C++ simulation timing.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import sys
import time

from headless_client import Client
from arena_experiments.config import canonical, file_digest, read_json, MIB
from arena_experiments.report import atomic_json


def trace_commands(path):
    if Path(path).stat().st_size > 256*MIB: raise ValueError("Trace exceeds 256 MiB")
    with Path(path).open("rb") as stream:
        while True:
            line=stream.readline(4*MIB+1)
            if not line: break
            if len(line)>4*MIB: raise ValueError("Oversized trace record")
            record=json.loads(line)
            if record["type"]=="request":
                yield {k:v for k,v in record["data"].items() if k not in ("id","version","session","revision")}


def capacity_commands(scenario, ticks):
    yield dict(op="start",scenario=scenario)
    # One fixed schedule for both recording modes, including rejected commands.
    for _ in range(12):
        yield dict(op="act",action={"kind":"move","direction":"forward"})
        yield dict(op="advance",ticks=100,until_decision=False)
    while ticks:
        step=min(ticks,1000); ticks-=step
        yield dict(op="advance",ticks=step,until_decision=False)
    yield dict(op="observe")
    yield dict(op="close")


def measure(runner,assets,mode,commands,record,deadline):
    result=dict(recording=record is not None,requests=0)
    output_hash=hashlib.sha256(); state=None; tick_count=0
    def event(chunk): output_hash.update((canonical(chunk)+"\n").encode())
    started=time.monotonic()
    with Client(runner,assets,mode,record,event,deadline=deadline) as client:
        for command in commands():
            request=dict(command); op=request.pop("op")
            response=client.request(op,**request)
            output_hash.update((canonical(response)+"\n").encode())
            result["requests"]+=1
            if not response["ok"]: raise RuntimeError(response)
            data=response["data"]
            if op=="start": result["startup_seconds"]=time.monotonic()-started; state=data
            elif "observation" in data: state=data["observation"]
            if op=="advance": tick_count+=data["ticks"]
        result.update(protocol_seconds=client.request_seconds,runner_peak_rss_kib=client.peak_rss_kib,
                      transport_read_bytes=client.bytes_read,transport_write_bytes=client.bytes_written)
    wall=time.monotonic()-started
    result.update(total_seconds=wall,simulation_ticks=tick_count,simulated_seconds_per_wall_second=tick_count/100/wall,
                  output_hash=output_hash.hexdigest(),phase=state["phase"] if state else None,
                  trace_bytes=Path(record).stat().st_size if record else 0)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner",required=True,type=Path); parser.add_argument("--assets",required=True,type=Path)
    parser.add_argument("--mode",choices=("diagnostic","visual"),default="diagnostic")
    source=parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--trace",type=Path); source.add_argument("--scenario",type=Path)
    parser.add_argument("--ticks",type=int,default=2000)
    parser.add_argument("--repetitions",type=int,default=3)
    parser.add_argument("--output",required=True,type=Path)
    args=parser.parse_args()
    if not 1<=args.repetitions<=5 or not 1<=args.ticks<=60000: parser.error("Bounds: 1..5 repetitions, 1..60000 ticks")
    args.output.mkdir(parents=True,exist_ok=False)
    scenario=read_json(args.scenario) if args.scenario else None
    commands=(lambda:trace_commands(args.trace)) if args.trace else (lambda:capacity_commands(scenario,args.ticks))
    measurements=[]
    for i in range(args.repetitions):
        # Alternate ordering to make filesystem/cache bias visible, not pretend to control it.
        for record in ((False,True) if i%2==0 else (True,False)):
            path=args.output/f"record-{i}.jsonl" if record else None
            row=measure(args.runner,args.assets,args.mode,commands,path,time.monotonic()+120)
            row["repetition"]=i; measurements.append(row)
    if len({r["output_hash"] for r in measurements})!=1: raise RuntimeError("Recording changed visible output / nonrepeatable request schedule")
    report=dict(version=1,workload="retained_trial_requests" if args.trace else "bounded_capacity_not_a_complete_fight",
                mode=args.mode,runner=str(args.runner.resolve()),binary_sha256=file_digest(args.runner),
                input_sha256=file_digest(args.trace or args.scenario),platform=platform.platform(),python=sys.version,
                measurements=measurements,output_equivalence="matched",build_requirement="Use Release; caller records build configuration.",
                limitation="Includes startup/fingerprinting, JSON, IPC and recording; OS cache uncontrolled. No frame-rate or pure simulation claim.")
    atomic_json(args.output/"benchmark.json",report)
    print(json.dumps(report,indent=2))


if __name__=="__main__": main()
