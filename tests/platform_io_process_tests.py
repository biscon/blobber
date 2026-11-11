import pathlib
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory(prefix='blobber-lock-') as root:
    path = str(pathlib.Path(root)/'lock')
    with subprocess.Popen([sys.argv[1],'hold',path],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True) as holder:
        try:
            assert holder.stdout.readline().strip() == 'locked'
            assert subprocess.run([sys.argv[1],'probe',path],timeout=10).returncode == 3
        finally:
            holder.communicate('\n',timeout=10)
    assert subprocess.run([sys.argv[1],'probe',path],timeout=10).returncode == 0
