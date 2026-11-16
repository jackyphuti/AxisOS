# AxisOS Verification & Codebase Utilities

This directory contains standalone tools for inspecting code compliance and binary linkages in AxisOS.

---

## 1. Codebase Compliance Scanner

`scan-codebase.c` scans git-tracked files to ensure strict 100% Pure C language compliance:

```bash
gcc -std=gnu11 -O2 scripts/scan-codebase.c -o scripts/scan-codebase
./scripts/scan-codebase
```

---

## 2. Strict Pure C Linkage Verification

`verify-pure-c.sh` (backed by `verify-pure-c.c`) inspects dynamic ELF symbols across all 22 native binaries to ensure zero linkage to `libstdc++`, `Qt6`, or `Python`:

```bash
bash scripts/verify-pure-c.sh
```
