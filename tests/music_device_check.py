#!/usr/bin/env python3
"""Optional bounded device integration with owned MP3 fixtures; never game content."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory(prefix='blobber-music-device-') as directory:
    root = Path(directory)
    music = root / 'audio/music'
    music.mkdir(parents=True)
    for name, duration, frequency in [('short', 2, 330), ('long', 7, 440)]:
        subprocess.run(['ffmpeg', '-v', 'error', '-nostdin', '-threads', '1', '-filter_threads', '1',
                        '-f', 'lavfi', '-i', f'sine=frequency={frequency}:duration={duration}:sample_rate=48000',
                        '-ac', '2', '-c:a', 'libmp3lame', '-b:a', '128k', str(music / f'{name}.mp3')], check=True)
    subprocess.run(['ffmpeg', '-v', 'error', '-nostdin', '-threads', '1', '-filter_threads', '1', '-f', 'lavfi',
                    '-i', 'sine=frequency=220:duration=0.3:sample_rate=48000', '-af', 'volume=6',
                    str(root / 'audio/hot.wav')], check=True)
    (music / 'playlist.json').write_text(json.dumps({'tracks': [
        {'id': name, 'path': f'{name}.mp3', 'title': name.title(), 'artist': 'Owned fixture', 'gain_db': 0}
        for name in ('short', 'long')]}))
    (root / 'music-state.json').write_text(json.dumps({'version': 1, 'order': ['short', 'long'], 'cursor': 0, 'random': 42, 'last': ''}))
    (root / 'audio/bank.json').write_text(json.dumps({'mix_gain_db': {'effects': 12}, 'cues': {'hot': {
        'gain': 1, 'cooldown': 0, 'aliases': 8, 'concurrency': 8, 'pitch_min': 1, 'pitch_max': 1, 'volume_min': 1,
        'variants': [{'id': '01', 'path': 'hot.wav'}]}}}))
    result = subprocess.run([sys.argv[1], str(root)])
    sys.exit(result.returncode)
