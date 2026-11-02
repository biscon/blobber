#!/usr/bin/env bash
set -euo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)
cache="$repo/.cache/audio-model"
image=blobber-stable-audio:small-sfx-20261102
mkdir -p "$cache/output"
chmod 700 "$cache"
case "${1:-help}" in
  build)
    exec nice -n 15 ionice -c 3 docker build --cpu-period 100000 --cpu-quota 200000 --memory 12g \
      -t "$image" "$repo/tools/audio/local" ;;
  login)
    # Interactive token entry goes straight to Hugging Face, never shell history.
    docker run --rm -it --cpus 2 --memory 2g --user "$(id -u):$(id -g)" \
      -v "$cache:/cache" --entrypoint /bin/sh "$image" -c 'umask 077; exec hf auth login --no-add-to-git-credential'
    chmod 700 "$cache/huggingface"
    chmod 600 "$cache/huggingface/token" "$cache/huggingface/stored_tokens"
    ;;
  status|download|generate|audition)
    command=$1; shift
    network=(--network none)
    device=()
    if [[ "$command" == download ]]; then network=(); fi
    if [[ "$command" == generate || "$command" == audition || "$command" == status ]]; then
      device=(--device nvidia.com/gpu=all)
    fi
    # No token environment, whole-home mount, Docker socket or desktop access.
    exec nice -n 15 ionice -c 3 docker run --rm --cpus 2 --memory 12g --pids-limit 256 \
      --user "$(id -u):$(id -g)" "${network[@]}" "${device[@]}" \
      -v "$cache:/cache" -v "$repo/tools/audio/local/audition.json:/recipes/audition.json:ro" \
      "$image" "$command" "$@" ;;
  *)
    echo 'Usage: tools/audio/local/run.sh build|login|download|status|audition'
    echo '       tools/audio/local/run.sh generate --prompt "one dry wooden door creak" --duration 4 --seed 42 --name door'
    ;;
esac
