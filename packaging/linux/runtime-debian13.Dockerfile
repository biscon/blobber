# amd64 image manifest resolved from the official debian:13-slim tag, 2026-11-11.
FROM debian:13-slim@sha256:7378260c0a098d27ac206225db5e7d92b8c7c84ea7afccc86eeb513756bbd392
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C TZ=UTC
COPY bootstrap-ca.pem /etc/ssl/certs/ca-certificates.crt
RUN rm -f /etc/apt/sources.list.d/* && printf '%s\n' \
    'deb http://snapshot.debian.org/archive/debian/20261110T000000Z trixie main' \
    'deb http://snapshot.debian.org/archive/debian-security/20261110T000000Z trixie-security main' \
    > /etc/apt/sources.list \
    && apt-get -o Acquire::Check-Valid-Until=false -o APT::Update::Error-Mode=any update \
    && apt-get -o Acquire::Check-Valid-Until=false install -y --no-install-recommends \
       ca-certificates python3 xvfb libgl1-mesa-dri libglx-mesa0 libx11-6 libxrandr2 \
       libxi6 libxcursor1 libxinerama1 libxxf86vm1 libasound2t64 libpulse0 util-linux \
    && mkdir -p /opt/blobber-runtime /work \
    && dpkg-query -W -f='${Package}\t${Version}\t${Architecture}\n' > /opt/blobber-runtime/packages.tsv \
    && rm -rf /var/lib/apt/lists/*
