# lock.json and the driver must agree with this immutable amd64 base.
FROM ubuntu:22.04@sha256:8a2321a92b438238f93b4346fff61bc4259c99fdd4d9b52ee86f153fee0c344f
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C TZ=UTC
COPY bootstrap-ca.pem /etc/ssl/certs/ca-certificates.crt
# Frozen selection: 20261110T000000Z; exact accepted packages.tsv versions.
# Snapshot package downloads returned 503. Signed-index-derived hashes retain the
# same package bytes/versions through standard Ubuntu archive transport (LD04).
COPY build-ubuntu22.debs.tsv build-ubuntu22-lock.json /opt/blobber-baseline/
RUN mkdir -p /var/tmp/baseline-debs \
    && printf '%s\n' 'Acquire::http::Timeout "30";' 'Acquire::https::Timeout "30";' 'Acquire::Retries "0";' > /etc/apt/apt.conf.d/99blobber-transport \
    && while read -r checksum url filename; do \
       /usr/lib/apt/apt-helper download-file "$url" "/var/tmp/baseline-debs/$filename" "SHA256:$checksum" || exit 1; \
       done < /opt/blobber-baseline/build-ubuntu22.debs.tsv \
    && rm -f /etc/apt/sources.list /etc/apt/sources.list.d/* \
    && apt-get --yes --no-install-recommends install /var/tmp/baseline-debs/*.deb \
    && rm -rf /var/tmp/baseline-debs /var/lib/apt/lists/*
COPY lock.json provision.py /opt/blobber-baseline/
RUN python3 /opt/blobber-baseline/provision.py \
    && dpkg-query -W -f='${Package}\t${Version}\t${Architecture}\n' \
       > /opt/blobber-baseline/packages.tsv
ENV PATH=/opt/cmake/bin:/usr/bin:/bin CC=gcc-11 CXX=g++-11
