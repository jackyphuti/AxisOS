# AxisOS Codebase Scanner

`scan-codebase.sh` is a read-only helper for finding common syntax and
configuration errors in the AxisOS repository. It scans Git-tracked files and
non-ignored working-tree files; it does not build the project, execute
application code, or modify repository files.

## Requirements

The script requires:

- Bash
- Git

These checks run when the corresponding tool is available:

- `python3` for Python syntax and JSON validation
- `node` for JavaScript, CommonJS, and ECMAScript module syntax
- `shellcheck` for optional shell-script linting

If `python3`, `node`, or `shellcheck` is unavailable, the related check is
reported as `SKIPPED`. A skipped optional check does not cause the scan to
fail.

## Running the scanner

From the repository root:

```bash
./scripts/scan-codebase.sh
```

The script can also be started from another directory using its path:

```bash
/path/to/AxisOS/scripts/scan-codebase.sh
```

To display the command usage:

```bash
./scripts/scan-codebase.sh --help
```

The scanner creates temporary compiler output outside the repository and
removes it when the scan finishes.

## Checks performed

The scanner checks:

- Python files (`.py`) with `python3 -m py_compile`
- Shell files (`.sh`) with `bash -n`
- JavaScript files (`.js`, `.cjs`, `.mjs`) with `node --check`
- JSON files (`.json`) with Python's JSON parser
- Shell files with `shellcheck` when ShellCheck is installed

The scan includes files known to Git as well as non-ignored files in the
working tree, so newly created files can be checked before they are staged.

## Exit codes

| Code | Meaning |
| ---: | --- |
| `0` | All performed checks passed. |
| `1` | At least one performed check found an error. |
| `2` | The command was used incorrectly or was run outside a Git repository. |

Example of using the exit code in automation:

```bash
if ./scripts/scan-codebase.sh; then
    echo "Codebase scan passed"
else
    echo "Codebase scan found errors"
    exit 1
fi
```

## Reading the output

Each check is shown as `OK`, `FAILED`, or `SKIPPED`. Failed checks include the
diagnostic output from the relevant tool. The summary at the end reports the
number of performed checks, skipped checks, and failures.
