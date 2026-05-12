#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
INTEGRATION_DIR="$PROJECT_ROOT/test/integration"
BINARY="$INTEGRATION_DIR/integrationtest.app/Contents/MacOS/integrationtest"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script currently targets macOS GUI runtime (Qt cocoa platform)." >&2
  exit 1
fi

CPU_COUNT="$(sysctl -n hw.ncpu 2>/dev/null || echo 8)"

export QT_QPA_PLATFORM="cocoa"
export CUTEMARKED_TEST_SUITES="MainWindowLfsIntegrationTest"

echo "[1/2] Building integration test binary..."
make -C "$INTEGRATION_DIR" -j"$CPU_COUNT"

echo "[2/2] Running MainWindow LFS GUI suite on cocoa..."
set +e
TEST_OUTPUT="$($BINARY -txt 2>&1)"
TEST_STATUS=$?
set -e

printf '%s\n' "$TEST_OUTPUT"

if printf '%s\n' "$TEST_OUTPUT" | rg -q "SKIP\s+: MainWindowLfsIntegrationTest::initTestCase"; then
  echo "MainWindow LFS suite was skipped. Run this script inside a logged-in desktop session with an active display." >&2
  exit 2
fi

if [[ "$TEST_STATUS" -ne 0 ]]; then
  echo "MainWindow LFS suite process exited with status $TEST_STATUS before completing assertions." >&2
  exit "$TEST_STATUS"
fi

if printf '%s\n' "$TEST_OUTPUT" | rg -q "FAIL!\s+: MainWindowLfsIntegrationTest::"; then
  echo "MainWindow LFS GUI integration suite failed." >&2
  exit 3
fi

if ! printf '%s\n' "$TEST_OUTPUT" | rg -q "PASS\s+: MainWindowLfsIntegrationTest::lfsActionsExistInGitMenu"; then
  echo "MainWindow LFS suite did not execute expected test cases." >&2
  exit 4
fi

exit "$TEST_STATUS"
