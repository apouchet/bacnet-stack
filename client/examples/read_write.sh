#!/bin/bash
# BACnet Client Example: Read and Write Properties
# This script demonstrates reading and writing BACnet properties

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="${SCRIPT_DIR}/../bin/bacnet-client"

# Default device instance (change as needed)
DEVICE=${1:-1234}

echo "=== BACnet Read/Write Example ==="
echo "Target device: $DEVICE"
echo ""

# Check if client exists
if [ ! -x "$CLIENT" ]; then
    echo "Error: bacnet-client not found. Please build first:"
    echo "  cd client && make"
    exit 1
fi

# Read device object name
echo "1. Reading device object name..."
$CLIENT read --device $DEVICE --object device:$DEVICE --property object-name
echo ""

# Read device model name
echo "2. Reading device model name..."
$CLIENT read --device $DEVICE --object device:$DEVICE --property model-name
echo ""

# Read device vendor name
echo "3. Reading device vendor name..."
$CLIENT read --device $DEVICE --object device:$DEVICE --property vendor-name
echo ""

# Read analog input present value (if exists)
echo "4. Reading analog-input:1 present value..."
$CLIENT read --device $DEVICE --object analog-input:1 --property present-value || echo "  (Object may not exist)"
echo ""

# Read object list
echo "5. Reading device object list..."
$CLIENT read --device $DEVICE --object device:$DEVICE --property object-list
echo ""

# Write example (commented out - uncomment to test)
# echo "6. Writing to analog-output:1..."
# $CLIENT write --device $DEVICE --object analog-output:1 --property present-value --value 50.0 --priority 8

echo "=== Read/Write Complete ==="
