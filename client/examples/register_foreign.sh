#!/bin/bash
# BACnet Client Example: Register as Foreign Device with BBMD
# This script demonstrates BBMD foreign device registration

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="${SCRIPT_DIR}/../bin/bacnet-client"

# BBMD address (change as needed)
BBMD_ADDRESS=${1:-192.168.1.1}
BBMD_PORT=${2:-47808}
TTL=${3:-60}

echo "=== BACnet Foreign Device Registration Example ==="
echo "BBMD: ${BBMD_ADDRESS}:${BBMD_PORT}"
echo "TTL: $TTL seconds"
echo ""

# Check if client exists
if [ ! -x "$CLIENT" ]; then
    echo "Error: bacnet-client not found. Please build first:"
    echo "  cd client && make"
    exit 1
fi

# Register as foreign device
echo "Registering as foreign device..."
$CLIENT register-foreign --bbmd ${BBMD_ADDRESS}:${BBMD_PORT} --ttl $TTL

echo ""
echo "Registration complete. Now you can discover devices across the BBMD."
echo ""

# Discover devices through BBMD
echo "Discovering devices..."
$CLIENT whois --timeout 5000

echo ""
echo "=== Foreign Device Registration Example Complete ==="
