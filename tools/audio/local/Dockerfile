FROM python:3.11-slim-bookworm@sha256:ee710afcfb733f4a750d9be683cf054b5cd247b6c5f5237a6849ea568b90ab15
ENV UV_LINK_MODE=copy UV_COMPILE_BYTECODE=0 UV_CONCURRENT_DOWNLOADS=2 UV_CONCURRENT_BUILDS=1 UV_CONCURRENT_INSTALLS=2 \
    OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=2 HF_HOME=/cache/huggingface \
    PATH=/opt/stable-audio/.venv/bin:$PATH HOME=/cache
RUN pip install --no-cache-dir uv==0.13.6
ADD --checksum=sha256:d37a94c98140e913d2f1f0eb489046ffa56528a1c62914f24bee6af5dfeddb9f https://codeload.github.com/Stability-AI/stable-audio-3/tar.gz/5f5e6ac21ae9ff78a6e8847eb7a1dcbe969ffb84 /tmp/source.tar.gz
RUN mkdir /opt/stable-audio && tar xzf /tmp/source.tar.gz -C /opt/stable-audio --strip-components=1 && rm /tmp/source.tar.gz
WORKDIR /opt/stable-audio
RUN UV_PYTHON_DOWNLOADS=never uv sync --python /usr/local/bin/python --frozen --no-dev && uv cache clean
COPY driver.py /opt/blobber-audio/driver.py
ENTRYPOINT ["python", "/opt/blobber-audio/driver.py"]
