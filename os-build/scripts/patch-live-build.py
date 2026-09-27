#!/usr/bin/env python3
import sys

def patch_file(path, search_str, replace_str):
    with open(path, "r") as f:
        content = f.read()
    if search_str in content:
        content = content.replace(search_str, replace_str, 1)
        with open(path, "w") as f:
            f.write(content)
        print(f"Patched: {path}")
        return True
    print(f"Already patched or pattern not found: {path}")
    return False

# 1. Patch lb_binary_syslinux for bootlogo
search_bootlogo = """tmpdir="$(mktemp -d)"
(cd "$tmpdir" && cpio -i) < ${_TARGET}/bootlogo"""

replace_bootlogo = """if [ -e "${_TARGET}/bootlogo" ]; then
tmpdir="$(mktemp -d)"
(cd "$tmpdir" && cpio -i) < ${_TARGET}/bootlogo"""

search_rm = """rm -rf "$tmpdir"

case "${LB_BUILD_WITH_CHROOT}" in"""

replace_rm = """rm -rf "$tmpdir"
fi

case "${LB_BUILD_WITH_CHROOT}" in"""

patch_file("/usr/lib/live/build/lb_binary_syslinux", search_bootlogo, replace_bootlogo)
patch_file("/usr/lib/live/build/lb_binary_syslinux", search_rm, replace_rm)
