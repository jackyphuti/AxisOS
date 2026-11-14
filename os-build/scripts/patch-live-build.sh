#!/usr/bin/env bash
set -e

PATH_LB="/usr/lib/live/build/lb_binary_syslinux"

if [ -f "$PATH_LB" ]; then
    # Safe bootlogo extraction
    sed -i 's/(cd "$tmpdir" && cpio -i) < ${_TARGET}\/bootlogo/if [ -e "${_TARGET}\/bootlogo" ]; then\n(cd "$tmpdir" \&\& cpio -i) < ${_TARGET}\/bootlogo/' "$PATH_LB" 2>/dev/null || true
    sed -i 's/(cd "$tmpdir" && ls -1 | cpio --quiet -o) > ${_TARGET}\/bootlogo\nrm -rf "$tmpdir"/(cd "$tmpdir" \&\& ls -1 | cpio --quiet -o) > ${_TARGET}\/bootlogo\nrm -rf "$tmpdir"\nfi/' "$PATH_LB" 2>/dev/null || true

    # Idempotent vmlinuz and initrd rename
    sed -i 's/mv binary\/live\/vmlinuz-\* binary\/live\/vmlinuz/mv binary\/live\/vmlinuz-* binary\/live\/vmlinuz 2>\/dev\/null || true/' "$PATH_LB" 2>/dev/null || true
    sed -i 's/mv binary\/live\/initrd\.img-\* binary\/live\/initrd\.img/mv binary\/live\/initrd.img-* binary\/live\/initrd.img 2>\/dev\/null || true/' "$PATH_LB" 2>/dev/null || true

    echo "lb_binary_syslinux patched successfully."
fi
