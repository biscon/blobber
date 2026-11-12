#!/usr/bin/env python3
"""Bound only the shared manual workflow's prepared-publication caches."""
import argparse
import hashlib
import json
import os
import re
import subprocess

PATTERN = re.compile(r'wd01-prepared-v1-[0-9a-f]{64}\Z')
LIMIT = 1024**3
# actions/cache v4's pinned toolkit: relative path, cross-OS zstd, salt 1.0.
# Check this contract when updating that action; fail if the saved version differs.
CACHE_VERSION = hashlib.sha256(b'output/wd01-prepared|zstd-without-long|1.0').hexdigest()


def usable(record, current, ref, default_ref, version):
    return (record['key'] == current and record['size_in_bytes'] <= LIMIT and
            record.get('ref') in (ref, default_ref) and record.get('version') == version)


def removals(records, current, ref=None, default_ref=None, version=None):
    if not PATTERN.fullmatch(current):
        raise ValueError('Invalid prepared cache key')
    owned = sorted((r for r in records if PATTERN.fullmatch(r['key'])),
                   key=lambda r: r['created_at'], reverse=True)
    matching = [r for r in owned if (usable(r, current, ref, default_ref, version) if ref else
                                    r['key'] == current and r['size_in_bytes'] <= LIMIT)]
    # Prefer branch-local over default-branch entries, like cache restore does.
    matching.sort(key=lambda r: r.get('ref') != ref)
    keep = matching[:1]
    previous = [r for r in owned if r['id'] not in {v['id'] for v in keep} and r['size_in_bytes'] <= LIMIT]
    keep += previous[:1]  # On miss reserve the other slot for a new cache.
    ids = {r['id'] for r in keep}
    return [r['id'] for r in owned if r['id'] not in ids]


def records(repo):
    pages = json.loads(subprocess.check_output(['gh', 'api', '--paginate', '--slurp',
        'repos/' + repo + '/actions/caches?key=wd01-prepared-v1-&per_page=100'], text=True))
    return [r for page in pages for r in page['actions_caches']]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('key')
    parser.add_argument('--verify', action='store_true', help='Require a usable saved/restored entry and enforce actual bytes')
    args = parser.parse_args()
    if not PATTERN.fullmatch(args.key):
        raise ValueError('Invalid prepared cache key')
    repo = os.environ['GITHUB_REPOSITORY']
    ref = os.environ['GITHUB_REF']
    default_ref = 'refs/heads/' + os.environ.get('DEFAULT_BRANCH', 'main')
    entries = records(repo)
    def delete(cache_id):
        subprocess.run(['gh', 'api', '--method', 'DELETE', 'repos/' + repo + '/actions/caches/' + str(cache_id)], check=True)
    if args.verify:
        oversized = [r for r in entries if r['key'] == args.key and r.get('ref') == ref and
                     r.get('version') == CACHE_VERSION and r['size_in_bytes'] > LIMIT]
        for r in oversized:
            delete(r['id'])
        if oversized:
            raise RuntimeError('New owned cache exceeded 1 GiB and was removed')
        matches = [r for r in entries if usable(r, args.key, ref, default_ref, CACHE_VERSION)]
        if not matches:
            raise RuntimeError('Exact cross-OS prepared cache is unavailable; refusing Windows transfer')
        print(json.dumps({'cache_entries': [{k:r[k] for k in ('id','key','ref','version','size_in_bytes')} for r in matches]}))
    selected = removals(entries, args.key, ref, default_ref, CACHE_VERSION)
    for cache_id in selected:
        delete(cache_id)
    print('Removed ' + str(len(selected)) + ' superseded/oversized owned prepared cache entries')


if __name__ == '__main__':
    main()
