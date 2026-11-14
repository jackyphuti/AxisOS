<div style="font-family: 'IBM Plex Sans', 'IBM Plex Mono', 'Liberation Mono', Menlo, Consolas, monospace; line-height: 1.6;">

# AxisOS Pure C Codebase Scanner

`scripts/scan-codebase.c` is a read-only helper for finding common syntax and
architecture errors in the AxisOS repository. It scans Git-tracked files and
verifies 100% Pure C language compliance.

## Running the scanner

From the repository root:

```bash
gcc -std=gnu11 -O2 scripts/scan-codebase.c -o scripts/scan-codebase
./scripts/scan-codebase
```

## Running Linkage Verification

```bash
gcc -std=gnu11 -O2 scripts/verify-pure-c.c -o scripts/verify-pure-c
./scripts/verify-pure-c
```

</div>
