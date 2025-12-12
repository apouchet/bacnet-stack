#!/bin/bash
# BACnet Client Example: Time Synchronization
# This script demonstrates sending time synchronization to BACnet devices

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="${SCRIPT_DIR}/../bin/bacnet-client"

echo "=== BACnet Time Synchronization Example ==="
echo ""

# Check if client exists
if [ ! -x "$CLIENT" ]; then
    echo "Error: bacnet-client not found. Please build first:"
    echo "  cd client && make"
    exit 1
fi

# Broadcast time sync
echo "1. Broadcasting time synchronization to all devices..."
$CLIENT time-sync --device broadcast
echo ""

# UTC time sync broadcast
echo "2. Broadcasting UTC time synchronization..."
$CLIENT time-sync --device broadcast --utc
echo ""

# Time sync to specific device (if provided)
if [ -n "$1" ]; then
    echo "3. Sending time sync to device $1..."
    $CLIENT time-sync --device $1
    echo ""
fi

echo "=== Time Sync Complete ==="
