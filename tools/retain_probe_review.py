#!/usr/bin/env python3
"""Retain a compact R01 motion gallery from --capture output.

Uses an existing ffmpeg executable; does not install dependencies. Full original
PNG sequences stay in the capture directory. Output must be new or empty.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


def compress_png(source: Path, destination: Path):
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(source),
                    '-frames:v', '1', '-compression_level', '9', '-pred', 'mixed',
                    str(destination)], check=True)


def retain(source: Path, destination: Path):
    data = json.loads((source / 'manifest.json').read_text())
    destination.mkdir(parents=True, exist_ok=True)
    if any(destination.iterdir()):
        raise ValueError('Output directory must be empty')
    if not shutil.which('ffmpeg'):
        raise RuntimeError('ffmpeg is required for compact video retention; use the original PNG gallery instead')
    summaries = []
    for series in data['series']:
        name = series['name']
        if '/' in name or '\\' in name or '..' in name:
            raise ValueError('Invalid series name')
        frames = series['frames']
        with tempfile.TemporaryDirectory(prefix='blobber-r01-video-') as directory:
            root = Path(directory)
            # Uniform 10 ms samples, holding sparse settled frames until the next timestamp.
            j = 0
            for i in range(960):
                ms = i * 10
                while j + 1 < len(frames) and frames[j + 1]['ms'] <= ms:
                    j += 1
                image = (source / frames[j]['file']).resolve()
                if not image.is_relative_to(source.resolve()):
                    raise ValueError('Frame escapes capture directory')
                (root / f'{i:04}.png').symlink_to(image)
            subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-framerate', '100',
                            '-i', str(root / '%04d.png'), '-c:v', 'libvpx-vp9', '-crf', '18', '-b:v', '0',
                            '-pix_fmt', 'gbrp', '-cpu-used', '4', '-threads', '2',
                            str(destination / f'{name}.webm')], check=True)
        shots = [0, 4, 6] if name.startswith('flat') else [6, 10, 11]
        for shot in shots:
            compress_png(source / name / f'contact-{shot}.png', destination / f'{name}-contact-{shot}.png')
        shot = 6 if name.startswith('flat') else 10
        compress_png(source / name / f'native-{shot}.png', destination / f'{name}-native.png')
        summaries.append({'name': name, 'shots': shots})
    compact = {'environment': data['environment'], 'series': summaries,
               'sourceCapture': str(source.resolve()), 'durationMs': 9600,
               'video': 'VP9 GBR CRF 18, 100 fps; motion preview is compressed; PNG stills and original capture are exact'}
    (destination / 'manifest.json').write_text(json.dumps(compact, indent=2) + '\n')
    html = '''<!doctype html><meta charset="utf-8"><title>R01 renderer review</title>
<style>body{background:#101818;color:#ddd;font:16px system-ui;margin:24px}a{color:#e4be70}button,select,input{margin:8px}section{display:inline-block;vertical-align:top;margin:8px}video{width:750px;height:423px;image-rendering:pixelated}h2{font-size:18px}p{max-width:1100px}</style>
<h1>R01 · renderer motion review</h1><p>All panels share the same 9.6-second script. Flat views compare the unchanged legacy renderer with GPU quality profiles. Specimen views add a sky, door and painting. This is a standalone visual probe; gameplay remains unchanged. Videos are compressed motion previews; linked PNG captures preserve exact rendered pixels.</p>
<button id="play">Play / pause</button><button id="back">−10 ms</button><button id="next">+10 ms</button><select id="speed"><option value="1">Real time</option><option value="0.25">Quarter speed</option></select><select id="scene"><option>flat</option><option>specimen</option></select><input id="seek" type="range" min="0" max="9590" step="10" value="0"><span id="time"></span><div id="views"></div><pre id="info"></pre>
<script>const data=__DATA__;let running=false;const seek=document.querySelector('#seek');let videos=[];
function populate(){running=false;document.querySelector('#views').innerHTML=data.series.filter(s=>s.name.startsWith(document.querySelector('#scene').value)).map(s=>'<section><h2>'+s.name+'</h2><video muted preload="auto" src="'+s.name+'.webm"></video><p><a href="'+s.name+'-native.png">Native presentation</a> · '+s.shots.map(i=>'<a href="'+s.name+'-contact-'+i+'.png">Shot '+i+'</a>').join(' · ')+'</p></section>').join('');videos=[...document.querySelectorAll('video')];for(const v of videos)v.addEventListener('loadedmetadata',()=>{v.currentTime=+seek.value/1000});}
function position(ms){running=false;seek.value=Math.max(0,Math.min(9590,ms));for(const v of videos){v.pause();v.currentTime=+seek.value/1000}document.querySelector('#time').textContent=seek.value+' ms';}
document.querySelector('#play').onclick=()=>{running=!running;for(const v of videos){v.playbackRate=+document.querySelector('#speed').value;if(running)v.play();else v.pause()}};document.querySelector('#speed').onchange=()=>{for(const v of videos)v.playbackRate=+document.querySelector('#speed').value};document.querySelector('#scene').onchange=populate;seek.oninput=()=>position(+seek.value);document.querySelector('#back').onclick=()=>position(+seek.value-10);document.querySelector('#next').onclick=()=>position(+seek.value+10);
function tick(){if(running&&videos.length){const t=videos[0].currentTime;seek.value=Math.round(t*1000);document.querySelector('#time').textContent=Math.round(t*1000)+' ms';for(const v of videos.slice(1))if(Math.abs(v.currentTime-t)>.03)v.currentTime=t;if(videos[0].ended){position(0)}}requestAnimationFrame(tick)}document.querySelector('#info').textContent=JSON.stringify(data.environment,null,2);populate();tick();</script>'''
    (destination / 'index.html').write_text(html.replace('__DATA__', json.dumps(compact)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    retain(args.source, args.destination)
