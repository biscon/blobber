#!/usr/bin/env python3
"""Bound only this workflow's prepared-publication caches; preserve all other caches."""
import argparse
import json
import os
import re
import subprocess

PATTERN = re.compile(r'wd01-prepared-v1-[0-9a-f]{64}\Z')
LIMIT = 1024**3


def removals(records, current):
    if not PATTERN.fullmatch(current):raise ValueError('Invalid prepared cache key')
    owned=[r for r in records if PATTERN.fullmatch(r['key'])]
    owned.sort(key=lambda r:r['created_at'],reverse=True)
    matching=[r for r in owned if r['key']==current and r['size_in_bytes']<=LIMIT]
    keep=matching[:1]
    previous=[r for r in owned if r['id'] not in {v['id'] for v in keep} and r['size_in_bytes']<=LIMIT]
    # Reserve one slot for the new publication when current is a miss.
    keep+=previous[:1]
    ids={r['id'] for r in keep}
    return [r['id'] for r in owned if r['id'] not in ids]


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('key');args=parser.parse_args()
    repo=os.environ['GITHUB_REPOSITORY']
    pages=json.loads(subprocess.check_output(['gh','api','--paginate','--slurp',
        'repos/'+repo+'/actions/caches?key=wd01-prepared-v1-&per_page=100'],text=True))
    records=[record for page in pages for record in page['actions_caches']]
    selected=removals(records,args.key)
    for cache_id in selected:
        subprocess.run(['gh','api','--method','DELETE','repos/'+repo+'/actions/caches/'+str(cache_id)],check=True)
    print('Removed '+str(len(selected))+' superseded/oversized owned prepared cache entries')


if __name__=='__main__':main()
