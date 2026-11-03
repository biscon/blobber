#!/usr/bin/env python3
"""Measure supplied MP3s without normalizing them; preserve manual trims and audio bytes."""
import json
import math
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2] / 'assets/audio/music'
manifest = root / 'playlist.json'
old = {t['path']: t for t in json.loads(manifest.read_text())['tracks']} if manifest.exists() else {}
tracks = []
for path in sorted((root / 'combat').glob('*.mp3')):
    proc = subprocess.run(['ffmpeg', '-hide_banner', '-nostdin', '-threads', '1', '-filter_threads', '1',
                           '-i', str(path), '-af', 'loudnorm=I=-18:TP=-3:LRA=11:print_format=json',
                           '-f', 'null', '-'], capture_output=True, text=True, check=True)
    result = json.JSONDecoder().raw_decode(proc.stderr[proc.stderr.rfind('{'):])[0]
    loudness, peak = float(result['input_i']), float(result['input_tp'])
    if not math.isfinite(loudness) or not math.isfinite(peak):
        raise ValueError(f'Invalid measurement: {path}')
    relative = path.relative_to(root).as_posix()
    stem = re.sub(r'-\d{6}$', '', path.stem.removeprefix('nickpanek-'))
    track = old.get(relative, {'id': path.stem, 'path': relative,
                              'title': stem.replace('-', ' ').title(), 'artist': 'Nick Panek'})
    track.setdefault('gain_db', 0)
    track.update(measured_lufs=loudness, measured_true_peak_db=peak)
    tracks.append(track)
    print(f'{path.name}: {loudness:.2f} LUFS, trim {track["gain_db"]:.2f} dB', flush=True)
manifest.write_text(json.dumps({'version': 1, 'tracks': tracks}, indent=2)+'\n')
