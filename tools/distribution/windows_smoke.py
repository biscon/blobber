#!/usr/bin/env python3
"""Windows application-owned smoke with isolated paths/ACLs and external software GL."""
import ctypes
import json
import os
from pathlib import Path
import shutil
import subprocess
import urllib.request
from archive import digest, encoded, require
from windows_archive import inspect_zip

MESA_URL='https://github.com/pal1000/mesa-dist-win/releases/download/26.2.6/mesa3d-26.2.6-release-msvc.7z'
MESA_SHA='b2e00615feaba45942343c265cc8c5f576142ca682c1ac6661922d4b72492877'


def saved_acl(operation, path, sddl=None):
    args=['powershell','-NoProfile','-NonInteractive','-File',str(Path(__file__).with_name('windows_acl.ps1')),
          '-Operation',operation,'-Target',str(path)]
    if sddl is not None:args+=['-Sddl',sddl]
    return subprocess.check_output(args,text=True).strip()


def smoke(bundle, source, build, run):
    archive=next(bundle.glob('*.zip'))
    require((bundle/(archive.name+'.sha256')).read_text()==digest(archive)+'  '+archive.name+'\n','ZIP checksum differs')
    extracted=run/'Windows extracted æ 日本'
    summary=inspect_zip(archive,extracted)
    install=extracted/summary['name']
    user=run/'user æ 日本';user.mkdir()
    (user/'.blobber-smoke-owner.json').write_bytes(encoded({'version':1}))
    (user/'local').mkdir();(user/'tmp').mkdir();cwd=user/'unrelated';cwd.mkdir()
    mesa=run/'mesa';mesa.mkdir()
    download=run/'mesa.7z'
    try:
        urllib.request.urlretrieve(MESA_URL,download)
        require(digest(download)==MESA_SHA,'Mesa archive hash mismatch')
        subprocess.run(['7z','x','-y',str(download),'-o'+str(mesa),'x64/*'],check=True,stdout=subprocess.DEVNULL)
        provider=mesa/'x64'
        require((provider/'opengl32.dll').is_file(),'Mesa WGL provider missing')
    except Exception as e:
        return {'status':'unverified','phase':'graphics-provider','reason':str(e),'archive_sha256':digest(archive)}
    env={**os.environ,'LOCALAPPDATA':str(user/'local'),'TEMP':str(user/'tmp'),'TMP':str(user/'tmp'),
         'GALLIUM_DRIVER':'llvmpipe','LIBGL_ALWAYS_SOFTWARE':'1','LP_NUM_THREADS':'2',
         'OMP_NUM_THREADS':'1','OPENBLAS_NUM_THREADS':'1'}
    # SetDllDirectory is inherited by normal unpackaged child processes. The game
    # and installation stay byte-identical; no Mesa DLL enters the player payload.
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.SetDllDirectoryW.argtypes=[ctypes.c_wchar_p]
    require(kernel.SetDllDirectoryW(str(provider.resolve())), 'Cannot set verification DLL directory')
    saved=[]
    sid=subprocess.check_output(['powershell','-NoProfile','-Command','[System.Security.Principal.WindowsIdentity]::GetCurrent().User.Value'],text=True).strip()
    def acl(path, mode):
        sddl=saved_acl('get',path);saved.append((path,sddl))
        if mode=='install':
            subprocess.run(['icacls',str(path),'/inheritance:r','/grant:r',sid+':(OI)(CI)(RX)'],check=True,stdout=subprocess.DEVNULL)
        else:
            subprocess.run(['icacls',str(path),'/deny',sid+':(OI)(CI)(RX)'],check=True,stdout=subprocess.DEVNULL)
    try:
        acl(install,'install')
        denied=[source/'assets',build/'assets']
        for path in denied:acl(path,'deny')
        for path in denied:
            try:(path/'rules/catalog.json').read_bytes()
            except PermissionError:pass
            else:raise RuntimeError('Source/build assets still readable after isolation')
        try:
            (install/'write-probe').write_bytes(b'forbidden')
        except PermissionError:pass
        else:raise RuntimeError('Install write ACL is ineffective')
        results={}
        for phase in ['create','continue']:
            log=run/('windows-smoke-'+phase+'.log')
            with log.open('w',encoding='utf-8') as stream:
                result=subprocess.run([str(install/'blobber.exe'),'--distribution-smoke',phase,
                    '--smoke-output',str(user/'smoke')],cwd=cwd,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=600)
            if result.returncode:
                message=log.read_text(encoding='utf-8',errors='replace')
                if phase=='create' and any(n in message for n in ['Failed to initialize GLFW','Failed to create window','WGL:','GLFW: Failed to initialize']):
                    return {'status':'unverified','phase':'graphics-context','reason':message[-8000:], 'archive_sha256':digest(archive)}
                raise RuntimeError('Extracted Windows smoke failed: '+message[-8000:])
            results[phase]=json.loads((user/'smoke'/(phase+'.json')).read_text(encoding='utf-8'))
        require(results['create']['graphics']==results['continue']['graphics'],'Graphics contexts differ')
        graphics=results['create']['graphics']
        require('llvmpipe' in graphics['renderer'].lower(), 'Expected software llvmpipe renderer')
        require(Path(graphics['module']).resolve().parent==provider.resolve(), 'Unexpected loaded OpenGL provider')
        for name,record in summary['manifest']['files'].items():
            path=install/name
            require(path.stat().st_size==record['bytes'] and digest(path)==record['sha256'],'Package changed during smoke')
        require({p.relative_to(install).as_posix() for p in install.rglob('*') if p.is_file()} ==
                set(summary['manifest']['files'])|{'MANIFEST.json'},'Unexpected installation file')
        evidence=run/'selected-captures';evidence.mkdir()
        for p in sorted((user/'smoke').glob('*.png'))[:6]:shutil.copy2(p,evidence/p.name)
        return {'status':'passed','archive_sha256':digest(archive),'software_gl':True,
                'provider_sha256':MESA_SHA,'provider_version':'mesa-dist-win-26.2.6',
                'isolation':'read-only ACL install; source/build assets denied; isolated LocalAppData; unrelated CWD',
                'phases':results,'audio':'references only; playback/listening unverified'}
    finally:
        for path,sddl in reversed(saved):
            saved_acl('restore',path,sddl)
        kernel.SetDllDirectoryW(None)
