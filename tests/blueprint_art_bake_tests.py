#!/usr/bin/env python3
"""Focused optional baker integration; owns all pixels, meshes and blueprints.
One configuration, one tiny actor, one kit. No production artwork dependency.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib


def png(path, size=8):
    path.parent.mkdir(parents=True, exist_ok=True)
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=(b'\0'+bytes((150,110,70,255))*size)*size
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,6,0,0,0))+
                     chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baker',type=Path)
    parser.add_argument('--cmake',type=Path,required=True)
    parser.add_argument('--ninja',type=Path,required=True)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='blueprint-bake-') as tmp:
        root=Path(tmp); source=root/'source'; output=root/'build/out'
        def write(path,data):
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text(json.dumps(data)+'\n')
        def run(command,okay=True):
            r=subprocess.run(command,text=True,capture_output=True)
            assert (r.returncode==0)==okay, r.stdout+r.stderr
            return r
        for name in ('wall','floor','ceiling','door'):
            png(source/f'textures/{name}.png')
        for name in ('grease','web','spiritual_weapon'):
            png(source/f'effects/{name}.png')
        for folder,names in [('se01',['body','emission']),('se02',['arcane-body','arcane-emission','contact-body','contact-emission']),
                             ('se03',['support-body','support-emission','ground-body','ground-emission'])]:
            for name in names: png(source/f'effects/{folder}/{name}.png',1024)
        for name in ('key-bronze','key-silver','upstairs','downstairs','endpoint','threshold'):
            png(source/f'fixtures/{name}.png')
        png(source/'ui/items/loot-bag.png'); png(source/'npcs/body.png')
        write(source/'npcs/level.json',dict(definitions=['actor.json'],instances=[]))
        write(source/'npcs/actor.json',dict(id='fixture',width=.45,height=.65,alpha_cutoff=15,
              animations={clip:dict(durations_ms=[100],views=dict.fromkeys(('front','back','left','right'),['body.png']))
                          for clip in ('idle','prone')}))
        for name in ('crest','vine','sky','atlas'): png(source/f'packs/fixture/{name}.png')
        manifest=dict(id='fixture',worldScale=1,horizon=[.6,.65,.7],wall='textures/wall.png',floor='textures/floor.png',
                      ceiling='textures/ceiling.png',stairWall='textures/wall.png',stairFloor='textures/floor.png',
                      door='textures/door.png',lockedDoor='textures/door.png',doorEdge='textures/door.png',
                      crest='packs/fixture/crest.png',vine='packs/fixture/vine.png',sky='packs/fixture/sky.png',
                      blueprintCatalog='blueprints/fixture.json',buildings=[dict(id='fixture',parts=['packs/fixture/kit.mesh.json'])])
        write(source/'packs/fixture/pack.json',manifest)
        model=dict(name='fixture-kit',texture='packs/fixture/atlas.png',
                   vertices=[[-.4,0,0,0,0,1,0,0],[.4,0,0,0,0,1,1,0],[0,.3,0,0,0,1,.5,1]])
        write(source/'packs/fixture/kit.mesh.json',model)
        catalog=dict(schema_version=1,kits=[dict(id='fixture',size=[1,1],height=.4)],rooms=[dict(
            id='room',purpose='kitchen',size=[4,4],ceiling=6,rotations=[0,90,180,270],solid=[],
            placements=[dict(kit='fixture',anchor=[0,0],facing='north')],
            sockets=[dict(id='south',cell=[1,4],outward='south')],lanes=[dict(origin=[1,0],size=[1,4])],
            encounters=[dict(origin=[1,0],size=[2,2])],rewards=[dict(cell=[3,3],access=[2,3])])])
        write(source/'blueprints/fixture.json',catalog)
        project=root/'project'; project.mkdir()
        rule=Path(__file__).resolve().parents[1]/'cmake/WorldAssets.cmake'
        (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.24)
project(BlueprintBake NONE)
include("{rule}")
blobber_world_publication(publication "{args.baker.resolve()}" "{source}" "${{CMAKE_BINARY_DIR}}/out")
''')
        run([str(args.cmake),'-S',str(project),'-B',str(root/'build'),'-G','Ninja',f'-DCMAKE_MAKE_PROGRAM={args.ninja}'])
        command=[str(args.cmake),'--build',str(root/'build'),'--target','publication','--parallel','2']
        run(command)
        stamp=(output/'metadata.json').stat().st_mtime_ns
        run(command)
        assert (output/'metadata.json').stat().st_mtime_ns==stamp, 'No-op build rewrote publication'
        published=(output/'textures.bin').read_bytes()
        provenance=json.loads((output/'sources.json').read_text())
        assert 'blueprints/fixture.json' in provenance
        # Catalog dependencies must trigger rebaking even though mesh and textures did not change.
        catalog['kits'][0]['height']=.2; write(source/'blueprints/fixture.json',catalog)
        failed=run(command,False)
        assert 'footprint or height' in failed.stderr+failed.stdout
        assert (output/'textures.bin').read_bytes()==published, 'Failed contract replaced installed payload'
        catalog['kits'][0]['height']=.4; write(source/'blueprints/fixture.json',catalog)
        run(command)
        # Geometry itself must trigger rejection, and failed publication preserves metadata too.
        prior=(output/'metadata.json').read_bytes()
        model['vertices'][1][0]=.6; write(source/'packs/fixture/kit.mesh.json',model)
        run(command,False)
        assert (output/'metadata.json').read_bytes()==prior
        assert (output/'textures.bin').read_bytes()==published
        print('Owned blueprint bake: source dependency, no-op, bounds failures and rollback passed')


if __name__=='__main__': main()
