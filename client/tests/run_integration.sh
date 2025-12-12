#!/bin/bash
# BACnet Client Integration Tests
# This script runs integration tests using a bacnet-stack server

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_DIR="${SCRIPT_DIR}/.."
REPO_DIR="${SCRIPT_DIR}/../.."

# Binaries
CLIENT="${CLIENT_DIR}/bin/bacnet-client"
SERVER="${REPO_DIR}/bin/bacserver"

# Test configuration
TEST_DEVICE_INSTANCE=12345
SERVER_PORT=47808
CLIENT_PORT=47809  # Different port to avoid conflict

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Log functions
log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++)) || true
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++)) || true
}

# Run a test
run_test() {
    local name="$1"
    local cmd="$2"
    local expected_exit="$3"
    
    ((TESTS_RUN++)) || true
    
    log_info "Running: $name"
    
    set +e
    output=$($cmd 2>&1)
    exit_code=$?
    set -e
    
    if [ "$exit_code" -eq "$expected_exit" ]; then
        log_pass "$name"
        return 0
    else
        log_fail "$name (exit code: $exit_code, expected: $expected_exit)"
        echo "  Output: $output"
        return 1
    fi
}

# Start the test server
start_server() {
    log_info "Starting BACnet test server (device instance: $TEST_DEVICE_INSTANCE)..."
    
    if [ ! -x "$SERVER" ]; then
        log_info "Server not found, building..."
        (cd "$REPO_DIR" && make server) || {
            log_fail "Failed to build server"
            return 1
        }
    fi
    
    # Start server in background
    export BACNET_IFACE=lo
    export BACNET_IP_PORT=$SERVER_PORT
    
    $SERVER $TEST_DEVICE_INSTANCE &
    SERVER_PID=$!
    
    # Wait for server to start
    sleep 2
    
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        log_fail "Server failed to start"
        return 1
    fi
    
    log_info "Server started (PID: $SERVER_PID)"
}

# Stop the test server
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        log_info "Stopping test server..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}

# Cleanup on exit
cleanup() {
    stop_server
}
trap cleanup EXIT

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    if [ ! -x "$CLIENT" ]; then
        log_info "Client not found, building..."
        (cd "$CLIENT_DIR" && make) || {
            log_fail "Failed to build client"
            exit 1
        }
    fi
    
    log_pass "Prerequisites checked"
}

# Main test suite
run_integration_tests() {
    log_info "=== BACnet Client Integration Tests ==="
    echo ""
    
    # Set client environment
    export BACNET_IFACE=lo
    export BACNET_IP_PORT=$CLIENT_PORT
    
    # Test 1: Help message
    run_test "Help message" "$CLIENT --help" 0
    
    # Test 2: Version
    run_test "Version information" "$CLIENT --version" 0
    
    # Test 3: Who-Is (may not find devices without network)
    run_test "Who-Is broadcast (short timeout)" "$CLIENT whois --timeout 1000" 0
    
    # Skip network-dependent tests if no server
    if [ -n "$SERVER_PID" ] && kill -0 $SERVER_PID 2>/dev/null; then
        # Test 4: Who-Is with server
        run_test "Who-Is discovers server" "$CLIENT whois --timeout 3000" 0
        
        # Test 5: Read device object name
        run_test "Read device object-name" \
            "$CLIENT read --device $TEST_DEVICE_INSTANCE --object device:$TEST_DEVICE_INSTANCE --property object-name" 0
        
        # Test 6: Read device vendor
        run_test "Read device vendor-identifier" \
            "$CLIENT read --device $TEST_DEVICE_INSTANCE --object device:$TEST_DEVICE_INSTANCE --property vendor-identifier" 0
        
        # Test 7: Time sync
        run_test "Time synchronization" \
            "$CLIENT time-sync --device broadcast" 0
    else
        log_info "Skipping network tests (no server running)"
    fi
    
    # Test invalid commands
    run_test "Invalid command returns error" "$CLIENT invalid-command" 1
    
    # Test missing required arguments
    run_test "Read without device returns error" "$CLIENT read --object device:1 --property object-name" 1
    
    echo ""
    log_info "=== Test Summary ==="
    echo "Total:  $TESTS_RUN"
    echo "Passed: $TESTS_PASSED"
    echo "Failed: $TESTS_FAILED"
    
    return $TESTS_FAILED
}

# Main
main() {
    check_prerequisites
    
    # Try to start server (optional)
    start_server || log_info "Continuing without server..."
    
    run_integration_tests
    exit_code=$?
    
    exit $exit_code
}

# Run main if not sourced
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi
