#!/usr/bin/env python3
"""
AxisOS Repository Index Builder (`repo-builder.py`)
==================================================
Scans a directory of `.axis` package archives, extracts metadata.json from each,
computes file size and SHA256 checksum, and outputs `repo-index.json`.
"""

import sys
import os
import json
import hashlib
import tarfile
from datetime import datetime
from pathlib import Path


def compute_sha256(filepath: str) -> str:
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def build_repo_index(packages_dir: str, base_url: str = "", output_file: str = "repo-index.json"):
    packages_dir = os.path.abspath(packages_dir)
    print(f"[+] Scanning for .axis packages in {packages_dir} ...")

    packages_map = {}

    for root, _, files in os.walk(packages_dir):
        for file in files:
            if file.endswith(".axis"):
                pkg_path = os.path.join(root, file)
                rel_url = os.path.relpath(pkg_path, packages_dir).replace("\\", "/")
                full_url = f"{base_url.rstrip('/')}/{rel_url.lstrip('/')}" if base_url else rel_url

                try:
                    with tarfile.open(pkg_path, "r:gz") as tar:
                        meta_file = tar.extractfile("metadata.json")
                        if not meta_file:
                            print(f"[!] Warning: {file} is missing metadata.json. Skipping.")
                            continue
                        meta = json.loads(meta_file.read().decode("utf-8"))

                    name = meta.get("name")
                    if not name:
                        print(f"[!] Warning: {file} metadata.json has no 'name'. Skipping.")
                        continue

                    sha256_hash = compute_sha256(pkg_path)
                    file_size = os.path.getsize(pkg_path)

                    packages_map[name] = {
                        "name": name,
                        "version": meta.get("version", "1.0.0"),
                        "description": meta.get("description", ""),
                        "dependencies": meta.get("dependencies", []),
                        "sha256": sha256_hash,
                        "size": file_size,
                        "url": full_url,
                        "maintainer": meta.get("maintainer", "AxisOS Community"),
                    }
                    print(f"  [+] Indexed: {name} v{meta.get('version')} ({file_size} bytes)")
                except Exception as e:
                    print(f"[!] Error processing {file}: {e}")

    index_data = {
        "version": "1.0",
        "updated_at": datetime.utcnow().isoformat() + "Z",
        "packages": packages_map,
    }

    out_path = os.path.join(packages_dir, output_file)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(index_data, f, indent=2)

    print(f"\n[OK] Successfully generated {out_path} with {len(packages_map)} packages.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python repo-builder.py <packages_dir> [base_url] [output_file]")
        sys.exit(1)

    pkg_dir = sys.argv[1]
    url = sys.argv[2] if len(sys.argv) > 2 else ""
    out = sys.argv[3] if len(sys.argv) > 3 else "repo-index.json"
    build_repo_index(pkg_dir, base_url=url, output_file=out)
