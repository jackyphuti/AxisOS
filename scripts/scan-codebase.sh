#!/usr/bin/env bash

set -u

readonly SCRIPT_NAME="$(basename "$0")"
readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

failures=0
checks=0
skipped=0

tmp_dir="$(mktemp -d)"
trap 'rm -rf -- "$tmp_dir"' EXIT

print_usage() {
    cat <<EOF
Usage: $SCRIPT_NAME

Scan tracked AxisOS source files for common syntax and configuration errors.
The scan is read-only; temporary compiler output is stored outside the repository.
EOF
}

record_failure() {
    failures=$((failures + 1))
}

run_check() {
    local description="$1"
    shift

    checks=$((checks + 1))
    printf '  %-58s' "$description"
    if "$@" >"$tmp_dir/check-output" 2>&1; then
        printf 'OK\n'
        return 0
    fi

    printf 'FAILED\n'
    sed 's/^/    /' "$tmp_dir/check-output"
    record_failure
    return 1
}

skip_check() {
    local description="$1"

    skipped=$((skipped + 1))
    printf '  %-58sSKIPPED\n' "$description"
}

tracked_files() {
    git -C "$ROOT_DIR" ls-files --cached --others --exclude-standard -z
}

check_shell() {
    local file
    local -a files=()

    while IFS= read -r -d '' file; do
        [[ "$file" == *.sh ]] && files+=("$ROOT_DIR/$file")
    done < <(tracked_files)

    if ((${#files[@]} == 0)); then
        skip_check "Shell syntax"
        return
    fi

    run_check "Shell syntax (${#files[@]} files)" bash -n "${files[@]}"
}

check_node() {
    local file
    local -a files=()

    while IFS= read -r -d '' file; do
        case "$file" in
            *.js|*.cjs|*.mjs) files+=("$ROOT_DIR/$file") ;;
        esac
    done < <(tracked_files)

    if ((${#files[@]} == 0)); then
        skip_check "Node.js syntax"
        return
    fi

    if ! command -v node >/dev/null 2>&1; then
        skip_check "Node.js syntax (node unavailable)"
        return
    fi

    local result=0
    for file in "${files[@]}"; do
        run_check "Node.js syntax ($(realpath --relative-to="$ROOT_DIR" "$file"))" \
            node --check "$file" || result=1
    done
    return "$result"
}

check_pure_c() {
    local file
    local -a forbidden=()
    while IFS= read -r -d '' file; do
        if [[ "$file" == *.cpp || "$file" == *.hpp || "$file" == *.qml || "$file" == *.py ]]; then
            forbidden+=("$file")
        fi
    done < <(tracked_files)

    if ((${#forbidden[@]} > 0)); then
        echo "[-] Violation: Found forbidden non-C files in repository:"
        printf '  %s\n' "${forbidden[@]}"
        return 1
    fi
    echo "[PASS] 100% Pure C codebase verified (0 .cpp, 0 .hpp, 0 .qml, 0 .py files)"
    return 0
}

check_json() {
    local file
    local -a files=()

    while IFS= read -r -d '' file; do
        [[ "$file" == *.json ]] && files+=("$ROOT_DIR/$file")
    done < <(tracked_files)

    if ((${#files[@]} == 0)); then
        skip_check "JSON syntax"
        return
    fi

    local result=0
    for file in "${files[@]}"; do
        if command -v jq >/dev/null 2>&1; then
            run_check "JSON syntax ($(realpath --relative-to="$ROOT_DIR" "$file"))" \
                jq . "$file" >/dev/null || result=1
        elif command -v node >/dev/null 2>&1; then
            run_check "JSON syntax ($(realpath --relative-to="$ROOT_DIR" "$file"))" \
                node -e 'JSON.parse(require("fs").readFileSync(process.argv[1]))' "$file" || result=1
        else
            skip_check "JSON syntax (jq/node unavailable)"
            return 0
        fi
    done
    return "$result"
}

check_optional_tools() {
    local file
    local -a shell_files=()

    while IFS= read -r -d '' file; do
        [[ "$file" == *.sh ]] && shell_files+=("$ROOT_DIR/$file")
    done < <(tracked_files)

    if ((${#shell_files[@]} > 0)); then
        if command -v shellcheck >/dev/null 2>&1; then
            run_check "ShellCheck (${#shell_files[@]} files)" shellcheck "${shell_files[@]}"
        else
            skip_check "ShellCheck (not installed)"
        fi
    fi
}

main() {
    if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
        print_usage
        return 0
    fi

    if (($# > 0)); then
        printf 'Error: unexpected argument: %s\n' "$1" >&2
        print_usage >&2
        return 2
    fi

    if ! git -C "$ROOT_DIR" rev-parse --show-toplevel >/dev/null 2>&1; then
        printf 'Error: %s must be run inside a Git repository.\n' "$SCRIPT_NAME" >&2
        return 2
    fi

    printf 'Scanning AxisOS for syntax and configuration errors...\n\n'
    check_pure_c
    check_shell
    check_node
    check_json
    check_optional_tools

    printf '\nChecks: %d | Skipped: %d | Failures: %d\n' \
        "$checks" "$skipped" "$failures"

    if ((failures > 0)); then
        printf 'Scan completed with errors.\n'
        return 1
    fi

    printf 'Scan completed successfully.\n'
}

main "$@"
