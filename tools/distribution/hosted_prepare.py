#!/usr/bin/env python3
"""One native Linux preparation, using the accepted pinned baseline environment."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from archive import encoded

ROOT=Path(__file__).resolve().parents[2]


def _prepare(run):
    recipe=ROOT/'packaging/linux'
    hashes={n:hashlib.sha256((recipe/n).read_bytes()).hexdigest() for n in ['Dockerfile','lock.json','provision.py','bootstrap-ca.pem']}
    recipe_id=hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest()
    image='blobber-baseline:'+recipe_id[:20]
    def command(args,name):
        print('Preparation phase: '+name,flush=True)
        with (run/name).open('w') as log:
            result=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise RuntimeError(name+' failed:\n'+(run/name).read_text(errors='replace')[-16000:])
    if subprocess.run(['docker','image','inspect',image],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
        command(['nice','-n','15','ionice','-c','3','docker','build','--platform','linux/amd64','--force-rm','-t',image,str(recipe)],'prepare-image.log')
    build=ROOT/'build-wd01-prepare';build.mkdir(exist_ok=True)
    base=['docker','run','--rm','--init','--network=none','--cpus=2','--memory=10g','--pids-limit=256',
          '--user',str(os.getuid())+':'+str(os.getgid()),
          '--mount','type=bind,src='+str(ROOT)+',dst=/src,readonly',
          '--mount','type=bind,src='+str(build)+',dst=/build',
          '--mount','type=bind,src='+str(run)+',dst=/run-output',
          '--mount','type=bind,src='+str(ROOT/'output')+',dst=/transfer',
          '--env','HOME=/run-output/home','--env','TMPDIR=/run-output/tmp',
          '--env','OMP_NUM_THREADS=1','--env','OPENBLAS_NUM_THREADS=1',
          image,'nice','-n','15','ionice','-c','3']
    (run/'home').mkdir(exist_ok=True);(run/'tmp').mkdir(exist_ok=True)
    command(base+['/opt/cmake/bin/cmake','-S','/src','-B','/build','-G','Ninja',
                 '-DCMAKE_MAKE_PROGRAM=/usr/bin/ninja','-DCMAKE_BUILD_TYPE=Release','-DBUILD_TESTING=ON','-DBLOBBER_LINUX_BASELINE=ON'],'prepare-configure.log')
    command(base+['/opt/cmake/bin/cmake','--build','/build','--target','blobber_bake_world','blobber_bake_portraits','blobber_bake_creator',
                 'blobber_validate_prepared','blobber_distribution_compatibility','--parallel','2'],'prepare-tools.log')
    command(base+['/opt/cmake/bin/cmake','--build','/build','--target','blobber_prepared_assets','--parallel','1'],'prepare-assets.log')
    command(base+['/build/blobber_validate_prepared','/src/packaging/runtime-content.json','/src/assets','/build/assets/prepared'],'prepare-validation.log')
    image_id=subprocess.check_output(['docker','image','inspect','--format','{{.Id}}',image],text=True).strip()
    command(base+['python3','-c',
        'import json,pathlib,sys; d=json.loads(pathlib.Path("/build/baseline-build.json").read_text()); d["image_id"]=sys.argv[1]; print(json.dumps(d))',
        image_id],'producer-environment.json')
    command(base+['python3','/src/tools/distribution/prepared.py','export','--source','/src','--prepared','/build/assets/prepared',
                 '--output','/transfer/wd01-prepared','--environment','/run-output/producer-environment.json',
                 '--validation','/run-output/prepare-validation.log'],'prepare-export.log')
    command(base+['/build/blobber_distribution_compatibility','export','/transfer/wd01-prepared/compatibility'],'prepare-compatibility.log')
    # Bound compressed transfer conservatively before requesting remote cache storage.
    import zlib
    total=0
    for path in (ROOT/'output/wd01-prepared').rglob('*'):
        if path.is_file():
            compressor=zlib.compressobj(3)
            with path.open('rb') as f:
                for block in iter(lambda:f.read(1024*1024),b''):total+=len(compressor.compress(block))
            total+=len(compressor.flush())
    if total>1024**3:raise RuntimeError('Prepared compressed transfer exceeds 1 GiB')
    print(json.dumps({'image':image,'recipe_sha256':recipe_id,'deflate_transfer_estimate':total}))


def prepare(run):
    import fcntl
    build=ROOT/'build-wd01-prepare'
    build.mkdir(exist_ok=True)
    owner=build/'.wd01-prepare-owner.json'
    identity={'version':1,'source':str(ROOT),'policy':'pinned-linux-preparation'}
    if owner.exists():
        if json.loads(owner.read_text())!=identity:raise RuntimeError('Preparation build ownership differs')
    else:
        if any(build.iterdir()):raise RuntimeError('Refusing unowned preparation build')
        owner.write_bytes(encoded(identity))
    with (build/'.prepare.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        _prepare(run)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--run-dir',type=Path,required=True)
    a=p.parse_args();prepare(a.run_dir.resolve())
