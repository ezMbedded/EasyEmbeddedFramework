#!/usr/bin/env bash
# Build the Doxygen documentation and report warnings.
#
# Usage: run_doxygen.sh [header]
#   header  Optional path of a header (e.g. inc/service/event_bus/ez_event_bus.h).
#           Only warnings that mention this file are reported.
#
# Output: HTML in doxygen/html, XML in doxygen/xml (entry point index.xml).
#
# Exit codes: 0 no warnings, 1 warnings found, 2 doxygen missing or build failed.

set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
filter="${1:-}"
log="$(mktemp)"
trap 'rm -f "$log"' EXIT

if ! command -v doxygen >/dev/null 2>&1; then
    echo "ERROR: doxygen is not installed (e.g. sudo apt install doxygen graphviz)" >&2
    exit 2
fi

cd "$repo_root" || exit 2

# Warnings are written to stderr, progress to stdout.
if ! doxygen Doxyfile >/dev/null 2>"$log"; then
    echo "ERROR: doxygen failed" >&2
    cat "$log" >&2
    exit 2
fi

if [ -n "$filter" ]; then
    warnings="$(grep -F -- "$filter" "$log" || true)"
else
    warnings="$(cat "$log")"
fi

if [ -n "$warnings" ]; then
    count="$(printf '%s\n' "$warnings" | wc -l)"
    echo "Doxygen finished with $count warning line(s):"
    printf '%s\n' "$warnings"
    exit 1
fi

echo "Doxygen finished with no warnings."
echo "HTML: $repo_root/doxygen/html/index.html"
echo "XML:  $repo_root/doxygen/xml/index.xml"
exit 0
