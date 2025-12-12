#!/bin/bash
# BACnet Client Example: Device Discovery
# This script demonstrates how to discover BACnet devices on the network

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="${SCRIPT_DIR}/../bin/bacnet-client"

echo "=== BACnet Device Discovery Example ==="
echo ""

# Check if client exists
if [ ! -x "$CLIENT" ]; then
    echo "Error: bacnet-client not found. Please build first:"
    echo "  cd client && make"
    exit 1
fi

# Basic discovery - find all devices
echo "1. Discovering all devices (3 second timeout)..."
$CLIENT whois --timeout 3000
echo ""

# Discovery with device range
echo "2. Discovering devices in range 100-200..."
$CLIENT whois --device-instance 100-200 --timeout 3000
echo ""

# JSON output for machine parsing
echo "3. Discovery with JSON output..."
$CLIENT whois --timeout 2000 --json
echo ""

echo "=== Discovery Complete ==="
