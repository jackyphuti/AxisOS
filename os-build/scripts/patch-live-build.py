#!/usr/bin/env python3
import sys

path = "/usr/lib/live/build/lb_binary_syslinux"
with open(path, "r") as f:
    content = f.read()

# 1. Safe bootlogo extraction
target = '(cd "$tmpdir" && cpio -i) < ${_TARGET}/bootlogo'
safe_target = 'if [ -e "${_TARGET}/bootlogo" ]; then\n(cd "$tmpdir" && cpio -i) < ${_TARGET}/bootlogo'
end_target = '(cd "$tmpdir" && ls -1 | cpio --quiet -o) > ${_TARGET}/bootlogo\nrm -rf "$tmpdir"'
safe_end_target = '(cd "$tmpdir" && ls -1 | cpio --quiet -o) > ${_TARGET}/bootlogo\nrm -rf "$tmpdir"\nfi'

if 'if [ -e "${_TARGET}/bootlogo" ]; then' not in content:
    if target in content and end_target in content:
        content = content.replace(target, safe_target, 1)
        content = content.replace(end_target, safe_end_target, 1)

# 2. Idempotent vmlinuz and initrd rename
content = content.replace("mv binary/live/vmlinuz-* binary/live/vmlinuz", "mv binary/live/vmlinuz-* binary/live/vmlinuz 2>/dev/null || true")
content = content.replace("mv binary/live/initrd.img-* binary/live/initrd.img", "mv binary/live/initrd.img-* binary/live/initrd.img 2>/dev/null || true")

with open(path, "w") as f:
    f.write(content)

print("lb_binary_syslinux patched successfully.")
