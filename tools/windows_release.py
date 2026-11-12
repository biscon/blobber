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
from windows_smoke import smoke, saved_acl, dacl_policy
from pe_audit import inspect as inspect_pe

ROOT=Path(__file__).resolve().parents[1]
TESTS=['rules','ai','campaign_save','agent_artifacts_tools','rng_portability','platform_io','platform_io_process','path_decode','path_stream_decode','library','arena','campaign_save_smoke','options','packaged_options',
       'runtime_inventory','dist_staging','npc_metadata','dialogue','audio','music','menu','gameplay_startup',
       'distribution_smoke','buttons','windows_distribution_tools','creator_art','portraits']
TARGETS=['blobber','blobber_rules_tests','blobber_ai_tests','blobber_bake_world','blobber_bake_creator','blobber_bake_portraits','blobber_rng_portability_tests','blobber_platform_io_tests','blobber_path_decode_tests','blobber_library_tests','blobber_arena_tests','blobber_campaign_save_tests',
         'blobber_options_tests','blobber_packaged_options_tests','blobber_distribution_tests','blobber_npc_metadata_tests',
         'blobber_dialogue_tests','blobber_audio_tests','blobber_music_tests','blobber_menu_tests','blobber_gameplay_startup_tests',
         'blobber_distribution_smoke_tests','blobber_buttons_tests','blobber_distribution_compatibility','blobber_validate_prepared',
         'blobber_creator_art_tests','blobber_portraits_tests','blobber_gpu_smoke']


def capture(args):return subprocess.check_output(list(map(str,args)),text=True,stderr=subprocess.STDOUT).strip()


def source_identity(source=ROOT):
    ledger=prepared.source_files(source)
    status=capture(['git','-C',source,'status','--porcelain','--untracked-files=normal'])
    return {'revision':capture(['git','-C',source,'rev-parse','HEAD']),'dirty':bool(status),'source_status':status.splitlines(),
            'source_fingerprint':hashlib.sha256(encoded(ledger)).hexdigest(),
            'source_epoch':int(capture(['git','-C',source,'show','-s','--format=%ct','HEAD']))}


@contextmanager
def writer(build):
    import msvcrt
    with (build/'.windows-release.lock').open('a+b') as f:
        f.seek(0);f.write(b'0');f.flush();f.seek(0)
        msvcrt.locking(f.fileno(),msvcrt.LK_NBLCK,1)
        try:yield
        finally:f.seek(0);msvcrt.locking(f.fileno(),msvcrt.LK_UNLCK,1)


def build_release(run,bundle,build,allow_dirty,code_check=False):
    require(sys.platform=='win32','Native Windows environment required')
    require(run.is_dir() and (run/'.agent-run.json').is_file(),'Owned run directory required')
    require(shutil.disk_usage(run).free>=8*1024**3,'Insufficient scratch space')
    class MemoryStatus(ctypes.Structure):
        _fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong),
                  *[(name,ctypes.c_ulonglong) for name in
                    ['total','available','total_page','available_page','total_virtual','available_virtual','extended']]]
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    memory=MemoryStatus();memory.length=ctypes.sizeof(memory)
    kernel.GlobalMemoryStatusEx.argtypes=[ctypes.POINTER(MemoryStatus)]
    require(kernel.GlobalMemoryStatusEx(ctypes.byref(memory)),'Cannot inspect available memory')
    require(memory.available>=3*1024**3,'Insufficient physical memory')
    kernel.GetCurrentProcess.restype=ctypes.c_void_p
    kernel.SetPriorityClass.argtypes=[ctypes.c_void_p,ctypes.c_ulong]
    require(kernel.SetPriorityClass(kernel.GetCurrentProcess(),0x4000),'Cannot lower build priority')
    build.mkdir(exist_ok=True)
    owner=build/'.windows-build-owner.json'
    ownership={'version':1,'source':str(ROOT),'platform':'native-msvc-x64'}
    if owner.exists():require(json.loads(owner.read_text())==ownership,'Windows build ownership mismatch')
    else:
        require(not any(build.iterdir()),'Refusing unowned nonempty build')
        owner.write_bytes(encoded(ownership))
    cache_path=build/'CMakeCache.txt'
    existing=dict(line.split('=',1) for line in cache_path.read_text().splitlines()
                  if '=' in line and not line.startswith(('#','//'))) if cache_path.is_file() else {}
    configured_cmake=regular(existing.get('CMAKE_COMMAND:INTERNAL',shutil.which('cmake') or ''))
    configured_ninja=regular(existing.get('CMAKE_MAKE_PROGRAM:FILEPATH',
                            existing.get('CMAKE_MAKE_PROGRAM:UNINITIALIZED',shutil.which('ninja') or '')))
    configured_ctest=regular(configured_cmake.with_name('ctest.exe'))
    environment=source_identity()
    require(not environment['dirty'] or allow_dirty,'Dirty source requires --allow-dirty')
    tools={'cmake':capture([configured_cmake,'--version']).splitlines()[0],
           'ninja':capture([configured_ninja,'--version']).splitlines()[0],
           'python':capture([sys.executable,'--version']).splitlines()[0],
           'paths':{'cmake':str(configured_cmake),'ninja':str(configured_ninja),
                    'ctest':str(configured_ctest),'python':sys.executable}}
    # cl reports its version to stderr; preserve both streams, regardless of no-input exit code.
    compiler=subprocess.run(['cl','/Bv'],capture_output=True,text=True)
    tools['compiler']=compiler.stdout+compiler.stderr
    tools['sdk']=os.environ.get('WindowsSDKVersion','').rstrip('\\')
    tools['msvc']=os.environ.get('VCToolsVersion','')
    require(tools['msvc'].startswith('14.44.') and tools['sdk']=='10.0.22621.0',
            'Unexpected MSVC/SDK selection: '+tools['msvc']+' / '+tools['sdk'])
    require(tools['cmake'].startswith('cmake version 3.31.') and tools['ninja'].startswith('1.13.') and
            tools['python'].startswith('Python 3.12.'),'Unexpected tool family: '+
            ' / '.join(tools[k] for k in ['cmake','ninja','python']))
    environment.update({'tools':tools,'runner_image':os.environ.get('ImageVersion'),
        'available_memory_bytes':memory.available,'free_scratch_bytes':shutil.disk_usage(run).free,
        'runner_os':capture(['cmd','/c','ver']),'dependency_lock_sha256':digest(ROOT/'packaging/linux/lock.json')})
    print('Windows phase: DACL preflight',flush=True)
    acl_probe=run/'DACL preflight æ 日本';acl_probe.mkdir()
    original_acl=saved_acl('get',acl_probe)
    saved_acl('restore',acl_probe,original_acl)
    restored_acl=saved_acl('get',acl_probe)
    require(dacl_policy(restored_acl)==dacl_policy(original_acl),'DACL preflight changed policy: '+
            json.dumps({'before':original_acl,'after':restored_acl}))
    environment['acl_preflight']='Unicode directory DACL policy snapshot/restore passed'
    def command(args,name):
        print('Windows phase: '+name,flush=True)
        with (run/name).open('w',encoding='utf-8') as log:
            result=subprocess.run(list(map(str,args)),stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:
            output=(run/name).read_text(encoding='utf-8',errors='replace')
            errors=[line for line in output.splitlines() if line.startswith('FAILED:') or ': error ' in line or 'fatal error' in line]
            raise RuntimeError(name+' failed:\n'+'\n'.join(errors[:80])+'\n'+output[-8000:])
    with writer(build):
        if code_check:
            bundle=run/'unused-code-check-publication'
            bundle.mkdir()
        else:
            manifest=prepared.validate(bundle,ROOT)
            environment['prepared']={k:manifest[k] for k in ['producer_revision','input_digest','producer_environment','identities']}
            require(manifest['producer_environment'] and manifest['identities'],'Missing prepared producer evidence')
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
        command([configured_cmake,'-S',ROOT,'-B',build,'-G','Ninja','-DCMAKE_MAKE_PROGRAM='+str(configured_ninja),'-DCMAKE_BUILD_TYPE=Release','-DBUILD_TESTING=ON',
                 '-DPython3_EXECUTABLE='+sys.executable,
                 '-DBLOBBER_WINDOWS_DISTRIBUTION=ON','-DBLOBBER_PREPARED_INPUT='+str(bundle),*overrides],'windows-configure.log')
        cache=dict(line.split('=',1) for line in (build/'CMakeCache.txt').read_text().splitlines() if '=' in line and not line.startswith(('#','//')))
        cmake=regular(cache['CMAKE_COMMAND:INTERNAL'])
        regular(cache.get('CMAKE_MAKE_PROGRAM:FILEPATH',cache.get('CMAKE_MAKE_PROGRAM:UNINITIALIZED','')))
        # Compile all required game/test/tool code in one graph. Keep independent
        # compilation going after errors so one port failure cannot hide others.
        targets=[('blobber_code_check' if target=='blobber' and code_check else target) for target in TARGETS]
        command([cmake,'--build',build,'--target',*targets,'--parallel','2','--','-k','0'],'windows-focused-build.log')
        command([configured_ctest,'--test-dir',build,'-R','^('+'|'.join(TESTS)+')$','--parallel','1','--output-on-failure','--no-tests=error'],'windows-focused-tests.log')
        if code_check:
            environment['code_check_pe']=inspect_pe(build/'blobber_code_check.exe')
            command([build/'blobber_code_check.exe','--help'],'windows-entry-check.log')
            observed=source_identity()
            require(observed=={k:environment[k] for k in observed},'Source changed during code check')
            print((run/'windows-focused-tests.log').read_text(encoding='utf-8',errors='replace')[-6000:],flush=True)
            print(json.dumps({'status':'code-check-passed','source':environment}),flush=True)
            return
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
        def unchanged_source():
            current=source_identity()
            require(current=={k:environment[k] for k in current},'Source changed while packaging')
        final=package(build,ROOT,environment,version,allow_dirty,unchanged_source)
        result=smoke(final,ROOT,build,run)
        (final/'verification.json').write_bytes(encoded(result))
        shutil.copytree(compatibility,final/'compatibility-windows')
        (final/'focused-tests.txt').write_bytes((run/'windows-focused-tests.log').read_bytes())
        if (run/'selected-captures').is_dir():shutil.copytree(run/'selected-captures',final/'captures')
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
    p.add_argument('--prepared-input',type=Path);p.add_argument('--build-dir',type=Path,default=ROOT/'build-windows-release')
    p.add_argument('--code-check',action='store_true',help='Native compile/link and owned tests; no review package')
    p.add_argument('--allow-dirty',action='store_true');a=p.parse_args()
    if not a.code_check and a.prepared_input is None:p.error('--prepared-input is required for a full review build')
    build_release(a.run_dir.resolve(),a.prepared_input.resolve() if a.prepared_input else None,
                  a.build_dir.resolve(),a.allow_dirty,a.code_check)
