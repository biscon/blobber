# Independent runtime: exact signed-archive package resolution is locked in the TSV.
FROM ubuntu:22.04@sha256:8a2321a92b438238f93b4346fff61bc4259c99fdd4d9b52ee86f153fee0c344f
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C TZ=UTC
COPY bootstrap-ca.pem /etc/ssl/certs/ca-certificates.crt
COPY runtime-ubuntu22.debs.tsv runtime-ubuntu22-lock.json /opt/blobber-runtime/
RUN mkdir -p /var/tmp/runtime-debs /work \
    && while read -r checksum url filename; do \
       /usr/lib/apt/apt-helper download-file "$url" "/var/tmp/runtime-debs/$filename" "SHA256:$checksum" || exit 1; \
       done < /opt/blobber-runtime/runtime-ubuntu22.debs.tsv \
    && rm -f /etc/apt/sources.list /etc/apt/sources.list.d/* \
    && apt-get --yes --no-install-recommends install /var/tmp/runtime-debs/*.deb \
    && dpkg-query -W -f='${Package}\t${Version}\t${Architecture}\n' > /opt/blobber-runtime/packages.tsv \
    && rm -rf /var/tmp/runtime-debs /var/lib/apt/lists/*
