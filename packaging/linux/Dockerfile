# lock.json and the driver must agree with this immutable amd64 base.
FROM ubuntu:22.04@sha256:8a2321a92b438238f93b4346fff61bc4259c99fdd4d9b52ee86f153fee0c344f
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C TZ=UTC
COPY bootstrap-ca.pem /etc/ssl/certs/ca-certificates.crt
# Use signed frozen archives, including security updates. No rolling apt sources.
RUN rm -f /etc/apt/sources.list.d/* && printf '%s\n' \
    'deb http://snapshot.ubuntu.com/ubuntu/20261110T000000Z jammy main universe' \
    'deb http://snapshot.ubuntu.com/ubuntu/20261110T000000Z jammy-updates main universe' \
    'deb http://snapshot.ubuntu.com/ubuntu/20261110T000000Z jammy-security main universe' \
    > /etc/apt/sources.list \
    && apt-get -o Acquire::Check-Valid-Until=false -o APT::Update::Error-Mode=any update \
    && apt-get -o Acquire::Check-Valid-Until=false install -y --no-install-recommends \
       ca-certificates curl python3 gcc-11 g++-11 ninja-build binutils make xz-utils \
       libgl1-mesa-dev libglu1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
       libxcursor-dev libxinerama-dev libxxf86vm-dev libpulse0 libasound2 \
       util-linux \
    && rm -rf /var/lib/apt/lists/*
COPY lock.json provision.py /opt/blobber-baseline/
RUN python3 /opt/blobber-baseline/provision.py \
    && dpkg-query -W -f='${Package}\t${Version}\t${Architecture}\n' \
       > /opt/blobber-baseline/packages.tsv
ENV PATH=/opt/cmake/bin:/usr/bin:/bin CC=gcc-11 CXX=g++-11
