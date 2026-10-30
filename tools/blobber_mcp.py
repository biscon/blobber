#!/usr/bin/env python3
"""Optional local stdio MCP adapter; install tools/mcp-requirements.lock in a venv."""
import argparse
import os
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--runner', type=Path, default=Path('cmake-build-headless-release/blobber_headless'))
    parser.add_argument('--assets', type=Path, default=Path('assets'))
    parser.add_argument('--output-root', type=Path, default=Path('output/mcp'))
    args = parser.parse_args()
    workspace = args.workspace.resolve(strict=True)
    os.chdir(workspace)
    runner, assets = args.runner.resolve(strict=True), args.assets.resolve(strict=True)
    if not runner.is_file() or not os.access(runner, os.X_OK) or not assets.is_dir():
        parser.error('Runner must be executable and assets must be a directory')
    try:
        from mcp_adapter.server import create_server
    except ImportError as exc:
        parser.error(f'Optional MCP environment missing: {exc}; install tools/mcp-requirements.lock')
    create_server(runner, assets, args.output_root.resolve()).run(transport='stdio')


if __name__ == '__main__':
    main()
