#!/usr/bin/env bash
# Build the unit tests, run one test, and check the line coverage of the
# source file(s) it is meant to cover.
#
# Usage: run_tests.sh [options] <test_name> <source_file> [<source_file> ...]
#   test_name    ctest name of the test, e.g. ez_spi_test
#   source_file  source under test, relative to the repo root, e.g. src/hal/spi/ez_spi.c
#
# Options:
#   -t, --threshold N   Required coverage per source file in percent (default 90).
#                       Applies to line coverage AND branch coverage. A file
#                       without branches only needs the line coverage.
#   -b, --build-dir D   Build directory (default build_linux_test_debug)
#       --coverage-only Skip configure, build and test. Only evaluate the coverage
#                       data (.gcda) already in the build directory.
#
# Output: build/test log on failure, a coverage table, the uncovered lines and
# the lines with branches not taken, and an HTML report in coverage/<test_name>/.
#
# Exit codes:
#   0  build ok, tests pass, line and branch coverage >= threshold for every file
#   1  configure/build failed, or a test failed
#   2  tests pass but coverage is below the threshold: STOP and report it to
#      the human, do not weaken tests or change the code to reach it
#   3  tooling missing, bad arguments, or no coverage data for a source file

set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
threshold=90
build_dir="build_linux_test_debug"
coverage_only=0
preset="linux_test_debug"
args=()

while [ $# -gt 0 ]; do
    case "$1" in
        -t|--threshold)  threshold="${2:-}"; shift 2 ;;
        -b|--build-dir)  build_dir="${2:-}"; shift 2 ;;
        --coverage-only) coverage_only=1; shift ;;
        -h|--help)       sed -n '2,28p' "${BASH_SOURCE[0]}"; exit 0 ;;
        -*)              echo "ERROR: unknown option $1" >&2; exit 3 ;;
        *)               args+=("$1"); shift ;;
    esac
done

if [ "${#args[@]}" -lt 2 ]; then
    echo "ERROR: need <test_name> and at least one <source_file>. Use --help." >&2
    exit 3
fi
test_name="${args[0]}"
sources=("${args[@]:1}")

case "$threshold" in
    ''|*[!0-9]*) echo "ERROR: threshold must be an integer, got '$threshold'" >&2; exit 3 ;;
esac

for tool in python3 gcovr; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool is not installed (gcovr: pip install gcovr)" >&2
        exit 3
    fi
done
if [ "$coverage_only" -eq 0 ]; then
    for tool in cmake ctest; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            echo "ERROR: $tool is not installed" >&2
            exit 3
        fi
    done
fi

cd "$repo_root" || exit 3

for s in "${sources[@]}"; do
    if [ ! -f "$s" ]; then
        echo "ERROR: source file not found: $s" >&2
        exit 3
    fi
done

log="$(mktemp)"
json="$(mktemp)"
trap 'rm -f "$log" "$json"' EXIT

if [ "$coverage_only" -eq 0 ]; then
    echo "== configure ($preset)"
    if ! cmake --preset "$preset" >"$log" 2>&1; then
        echo "ERROR: configure failed. Last lines:" >&2
        tail -n 30 "$log" >&2
        exit 1
    fi

    echo "== build"
    if ! cmake --build --preset "$preset" >"$log" 2>&1; then
        echo "ERROR: build failed. Errors:" >&2
        grep -E "error|Error|undefined reference" "$log" | head -n 30 >&2
        echo "(full log is deleted on exit; re-run: cmake --build --preset $preset)" >&2
        echo "If the failure is in another module, do not fix it. Build and run" >&2
        echo "your test by hand (see SKILL.md) and report that the full build failed." >&2
        exit 1
    fi

    # Only the counts of this run must be evaluated.
    find "$build_dir" -name '*.gcda' -delete 2>/dev/null

    echo "== test ($test_name)"
    if ! ctest --preset "$preset" -R "^${test_name}\$" --output-on-failure >"$log" 2>&1; then
        echo "ERROR: test failed or not found:" >&2
        tail -n 60 "$log" >&2
        exit 1
    fi
    grep -E "tests passed|Test +#|No tests were found" "$log"
fi

echo "== coverage (threshold ${threshold}% lines and branches)"
filters=()
for s in "${sources[@]}"; do
    esc="$(printf '%s' "$s" | sed 's/[][\.*^$+?(){}|]/\\&/g')"
    filters+=(--filter "^${esc}\$")
done

out_dir="coverage/${test_name}"
mkdir -p "$out_dir"
if ! gcovr -r . "${filters[@]}" --exclude-directories "$build_dir/_deps" \
        --json "$json" --html-details "$out_dir/index.html" "$build_dir" >"$log" 2>&1; then
    echo "ERROR: gcovr failed:" >&2
    tail -n 20 "$log" >&2
    exit 3
fi

python3 - "$json" "$threshold" "${sources[@]}" <<'PY'
import json, sys

data = json.load(open(sys.argv[1]))
threshold = float(sys.argv[2])
wanted = sys.argv[3:]
files = {f["file"]: f for f in data.get("files", [])}

def ranges(nums):
    out, start, prev = [], None, None
    for n in nums:
        if start is None:
            start = prev = n
        elif n == prev + 1:
            prev = n
        else:
            out.append((start, prev)); start = prev = n
    if start is not None:
        out.append((start, prev))
    return ", ".join(str(a) if a == b else f"{a}-{b}" for a, b in out)

missing, below = [], []
print(f"{'file':44} {'lines':>14} {'branches':>16}")
for w in wanted:
    f = files.get(w)
    if f is None:
        missing.append(w)
        continue
    lines = [l for l in f["lines"] if not l.get("gcovr/noncode")]
    total = len(lines)
    uncovered = sorted(l["line_number"] for l in lines if l["count"] == 0)
    covered = total - len(uncovered)
    pct = 100.0 * covered / total if total else 100.0
    br = [b for l in lines for b in l.get("branches", [])]
    br_cov = sum(1 for b in br if b["count"] > 0)
    br_pct = 100.0 * br_cov / len(br) if br else None
    br_txt = f"{br_cov}/{len(br)} {br_pct:5.1f}%" if br else "n/a"
    partial = sorted(l["line_number"] for l in lines
                     if l.get("branches") and any(b["count"] == 0 for b in l["branches"]))
    print(f"{w:44} {covered:>4}/{total:<4} {pct:5.1f}% {br_txt:>16}")
    if uncovered:
        print(f"  uncovered lines: {ranges(uncovered)}")
    if partial:
        print(f"  lines with branches not taken: {ranges(partial)}")
    if pct < threshold:
        below.append((w, "lines", pct))
    if br_pct is not None and br_pct < threshold:
        below.append((w, "branches", br_pct))

if missing:
    print("\nERROR: no coverage data for: " + ", ".join(missing), file=sys.stderr)
    print("The test did not run this file, or it was built without coverage.", file=sys.stderr)
    sys.exit(3)
if below:
    print("")
    for w, kind, p in below:
        print(f"RESULT: {kind.upper()} COVERAGE BELOW {threshold:.0f}%: {w} is {p:.1f}%")
    print("STOP and report this to the human with the uncovered lines and branches. Do not weaken tests or change the code under test to reach the number.")
    sys.exit(2)
print(f"\nRESULT: PASS (every file >= {threshold:.0f}% lines and branches)")
PY
exit $?
