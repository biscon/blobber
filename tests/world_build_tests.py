#!/usr/bin/env python3
"""Opt-in offline publication/build contract; generated fixtures only."""
import argparse, json, pathlib, struct, subprocess, tempfile, zlib

def png(path, value=170):
    path.parent.mkdir(parents=True, exist_ok=True)
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + bytes((value, 80, 40, 255)) * 8 for _ in range(8))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 8, 8, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))

def run(*args, okay=True):
    result = subprocess.run(args, capture_output=True, text=True)
    if (result.returncode == 0) != okay:
        raise AssertionError(result.stdout + result.stderr)
    return result.stdout

def main():
    parser=argparse.ArgumentParser(); parser.add_argument('baker', type=pathlib.Path); parser.add_argument('rule', type=pathlib.Path)
    args=parser.parse_args(); baker=args.baker.resolve(); rule=args.rule.resolve()
    with tempfile.TemporaryDirectory(prefix='blobber world build ') as temp:
        root=pathlib.Path(temp); source=root/'source assets'; source.mkdir()
        for name in ['wall','floor','ceiling','door','door-locked']: png(source/'textures'/f'{name}.png')
        for name in ['grease','web','spiritual_weapon']: png(source/'effects'/f'{name}.png')
        for name in ['key-bronze','key-silver','upstairs','downstairs','endpoint','threshold']: png(source/'fixtures'/f'{name}.png')
        png(source/'ui/items/loot-bag.png'); png(source/'npcs/body.png')
        (source/'npcs/level.json').write_text(json.dumps({'definitions':['actor.json'],'instances':[]}))
        (source/'npcs/actor.json').write_text(json.dumps({'id':'fixture','width':.45,'height':.65,'alpha_cutoff':15,'views':dict.fromkeys(['front','back','left','right'],'body.png')}))
        project=root/'project'; project.mkdir()
        (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.24)
project(WorldAssetFixture NONE)
include("{rule.as_posix()}")
set(destination "${{CMAKE_BINARY_DIR}}/out")
if(CMAKE_CONFIGURATION_TYPES)
  string(APPEND destination "/$<CONFIG>")
endif()
blobber_world_publication(publication "{baker.as_posix()}" "{source.as_posix()}" "${{destination}}")
''')
        effect_revision=40
        for generator in ['Ninja','Ninja Multi-Config']:
            build=root/generator; run('cmake','-S',str(project),'-B',str(build),'-G',generator)
            configs=['Debug','Release'] if 'Multi' in generator else ['Debug']
            for config in configs:
                output=build/'out'/config if 'Multi' in generator else build/'out'
                command=['cmake','--build',str(build),'--target','publication','--config',config]
                run(*command)
                names=['metadata.json','textures.bin','sources.json']
                stamp=lambda: [(output/n).stat().st_mtime_ns for n in names]
                first=stamp(); run(*command); assert stamp()==first, 'No-op build rewrites assets'
                old=(output/'textures.bin').read_bytes(); png(source/'textures/wall.png',(old[0]+17)%200+20)
                run(*command); assert (output/'textures.bin').read_bytes()!=old, 'Changed source did not rebuild payload'
                old=(output/'textures.bin').read_bytes(); effect_revision+=1; png(source/'effects/web.png',effect_revision)
                run(*command); assert (output/'textures.bin').read_bytes()!=old, 'Changed effect source did not rebuild payload'
                (output/'textures.bin').unlink(); run(*command); assert (output/'textures.bin').exists()
                png(source/'fixtures/new-unused.png'); run(*command)
                assert 'fixtures/new-unused.png' in json.loads((output/'sources.json').read_text()), 'Added source not detected'
                (source/'fixtures/new-unused.png').unlink(); run(*command)
                assert 'fixtures/new-unused.png' not in json.loads((output/'sources.json').read_text()), 'Removed source not detected'
                prior=stamp(); (source/'npcs/body.png').touch(); run(*command); assert stamp()!=prior, 'Touched source not rebuilt'
                good=(source/'textures/wall.png').read_bytes(); prior_payload=(output/'textures.bin').read_bytes()
                (source/'textures/wall.png').write_bytes(b'broken'); run(*command,okay=False)
                assert (output/'textures.bin').read_bytes()==prior_payload, 'Failed bake replaced prior payload'
                (source/'textures/wall.png').write_bytes(good); run(*command)
                manifest=json.loads((output/'metadata.json').read_text())
                assert len(manifest['appearances'])==1 and len(manifest['textures'])==25 and manifest['appearance_revision']==4
                print(generator,config,'clean/no-op/change/touch/missing/add/remove/failure: pass')
    print('World build fixtures passed')
if __name__=='__main__': main()
