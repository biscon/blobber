#!/usr/bin/env python3
"""Manual native Windows build/test/package. Uses no cross compiler or desktop input."""
import argparse
from contextlib import contextmanager
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request
sys.path.insert(0,str(Path(__file__).resolve().parent/'distribution'))
from archive import digest,encoded,regular,require
import prepared
from windows_archive import package
from windows_smoke import smoke

ROOT=Path(__file__).resolve().parents[1]
TESTS=['platform_io','platform_io_process','library','arena','campaign_save_smoke','options','packaged_options',
       'runtime_inventory','dist_staging','npc_metadata','dialogue','audio','music','menu','gameplay_startup',
       'distribution_smoke','buttons','windows_distribution_tools','creator_art','portraits']
TARGETS=['blobber_platform_io_tests','blobber_library_tests','blobber_arena_tests','blobber_campaign_save_tests',
         'blobber_options_tests','blobber_packaged_options_tests','blobber_distribution_tests','blobber_npc_metadata_tests',
         'blobber_dialogue_tests','blobber_audio_tests','blobber_music_tests','blobber_menu_tests','blobber_gameplay_startup_tests',
         'blobber_distribution_smoke_tests','blobber_buttons_tests','blobber_distribution_compatibility','blobber_validate_prepared',
         'blobber_creator_art_tests','blobber_portraits_tests','blobber_gpu_smoke']


def capture(args):return subprocess.check_output(list(map(str,args)),text=True,stderr=subprocess.STDOUT).strip()


def source_identity():
    names=capture(['git','ls-files','-z']).split('\0')
    ledger={n:digest(regular(ROOT/n)) for n in sorted(names) if n}
    status=capture(['git','status','--porcelain','--untracked-files=normal'])
    return {'revision':capture(['git','rev-parse','HEAD']),'dirty':bool(status),'source_status':status.splitlines(),
            'source_fingerprint':hashlib.sha256(encoded(ledger)).hexdigest(),
            'source_epoch':int(capture(['git','show','-s','--format=%ct','HEAD']))}


@contextmanager
def writer(build):
    import msvcrt
    with (build/'.windows-release.lock').open('a+b') as f:
        f.seek(0);f.write(b'0');f.flush();f.seek(0)
        msvcrt.locking(f.fileno(),msvcrt.LK_NBLCK,1)
        try:yield
        finally:f.seek(0);msvcrt.locking(f.fileno(),msvcrt.LK_UNLCK,1)


def build_release(run,bundle,build,allow_dirty):
    require(sys.platform=='win32','Native Windows environment required')
    require(run.is_dir() and (run/'.agent-run.json').is_file(),'Owned run directory required')
    require(shutil.disk_usage(run).free>=8*1024**3,'Insufficient scratch space')
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(),0x4000)
    environment=source_identity()
    require(not environment['dirty'] or allow_dirty,'Dirty source requires --allow-dirty')
    tools={name:capture([name,'--version']).splitlines()[0] for name in ['cmake','ninja','python']}
    # cl reports its version to stderr; preserve both streams, regardless of no-input exit code.
    compiler=subprocess.run(['cl','/Bv'],capture_output=True,text=True)
    tools['compiler']=compiler.stdout+compiler.stderr
    tools['sdk']=os.environ.get('WindowsSDKVersion','').rstrip('\\')
    tools['msvc']=os.environ.get('VCToolsVersion','')
    require(tools['msvc'].startswith('14.44.') and tools['sdk']=='10.0.22621.0','Unexpected MSVC/SDK selection')
    require(tools['cmake'].startswith('cmake version 3.31.') and tools['ninja'].startswith('1.13.') and
            tools['python'].startswith('Python 3.12.'),'Unexpected CMake/Ninja/Python family')
    environment.update({'tools':tools,'runner_image':os.environ.get('ImageVersion'),
        'runner_os':capture(['cmd','/c','ver']),'dependency_lock_sha256':digest(ROOT/'packaging/linux/lock.json')})
    def command(args,name):
        with (run/name).open('w',encoding='utf-8') as log:
            result=subprocess.run(list(map(str,args)),stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise RuntimeError(name+' failed:\n'+(run/name).read_text(encoding='utf-8',errors='replace')[-14000:])
    build.mkdir(exist_ok=True)
    owner=build/'.windows-build-owner.json'
    ownership={'version':1,'source':str(ROOT),'platform':'native-msvc-x64'}
    if owner.exists():require(json.loads(owner.read_text())==ownership,'Windows build ownership mismatch')
    else:
        require(not any(build.iterdir()),'Refusing unowned nonempty build')
        owner.write_bytes(encoded(ownership))
    with writer(build):
        manifest=prepared.validate(bundle,ROOT)
        environment['prepared']={k:manifest[k] for k in ['producer_revision','input_digest']}
        lock=json.loads((ROOT/'packaging/linux/lock.json').read_text())
        dependencies=run/'dependencies';dependencies.mkdir()
        overrides=[]
        for name,entry in lock['sources'].items():
            archive=run/(name+'.tar');urllib.request.urlretrieve(entry['url'],archive)
            require(digest(archive)==entry['sha256'],'Dependency checksum mismatch: '+name)
            destination=dependencies/name;destination.mkdir()
            with tarfile.open(archive) as tar:tar.extractall(destination,filter='data')
            roots=list(destination.iterdir());require(len(roots)==1 and roots[0].is_dir(),'Dependency root mismatch')
            overrides.append('-DFETCHCONTENT_SOURCE_DIR_'+name.upper()+'='+str(roots[0]))
        command(['cmake','-S',ROOT,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DBUILD_TESTING=ON',
                 '-DBLOBBER_WINDOWS_DISTRIBUTION=ON','-DBLOBBER_PREPARED_INPUT='+str(bundle),*overrides],'windows-configure.log')
        cache=dict(line.split('=',1) for line in (build/'CMakeCache.txt').read_text().splitlines() if '=' in line and not line.startswith(('#','//')))
        cmake=regular(cache['CMAKE_COMMAND:INTERNAL'])
        regular(cache.get('CMAKE_MAKE_PROGRAM:FILEPATH',cache.get('CMAKE_MAKE_PROGRAM:UNINITIALIZED','')))
        command([cmake,'--build',build,'--target',*TARGETS,'--parallel','2'],'windows-focused-build.log')
        command(['ctest','--test-dir',build,'-R','^('+'|'.join(TESTS)+')$','--parallel','1','--output-on-failure','--no-tests=error'],'windows-focused-tests.log')
        command([build/'blobber_validate_prepared.exe',ROOT/'packaging/runtime-content.json',ROOT/'assets',bundle/'prepared'],'windows-prepared-validation.log')
        compatibility=run/'compatibility-windows'
        command([build/'blobber_distribution_compatibility.exe','import',bundle/'compatibility',compatibility],'windows-compatibility.log')
        command([cmake,'--build',build,'--target','dist-stage','--parallel','2'],'windows-stage.log')
        dry=capture([cmake,'--build',build,'--target','blobber','--','-n'])
        require(not re.search(r'\bblobber_bake_(world|portraits|creator)(?:\.exe)?\s',dry),'Imported game still schedules production baker')
        environment['import_graph']='no production bake scheduled'
        final_source=source_identity()
        require(final_source=={k:environment[k] for k in final_source},'Source changed during build')
        version=re.search(r'project\(blobber VERSION ([0-9.]+)',(ROOT/'CMakeLists.txt').read_text()).group(1)
        final=package(build,ROOT,environment,version,allow_dirty)
        result=smoke(final,ROOT,build,run)
        (final/'verification.json').write_bytes(encoded(result))
        shutil.copytree(compatibility,final/'compatibility-windows')
        (final/'focused-tests.txt').write_bytes((run/'windows-focused-tests.log').read_bytes())
        if result['status']=='passed':
            # Same provider, hidden context; no OS input or desktop focus changes.
            import windows_smoke
            ctypes.windll.kernel32.SetDllDirectoryW(ctypes.c_wchar_p(str((run/'mesa/x64').resolve())))
            try:command([build/'blobber_gpu_smoke.exe','--depth-copy'],'windows-depth-copy.log')
            finally:ctypes.windll.kernel32.SetDllDirectoryW(None)
            (final/'depth-copy.txt').write_bytes((run/'windows-depth-copy.log').read_bytes())
        print('Windows review bundle: '+str(final),flush=True)
        print('Extracted graphics smoke: '+result['status'],flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--run-dir',type=Path,required=True)
    p.add_argument('--prepared-input',type=Path,required=True);p.add_argument('--build-dir',type=Path,default=ROOT/'build-windows-release')
    p.add_argument('--allow-dirty',action='store_true');a=p.parse_args()
    build_release(a.run_dir.resolve(),a.prepared_input.resolve(),a.build_dir.resolve(),a.allow_dirty)
