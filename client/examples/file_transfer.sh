#!/bin/bash
# BACnet Client Example: File Transfer
# This script demonstrates atomic file read/write operations

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="${SCRIPT_DIR}/../bin/bacnet-client"

# Default device instance (change as needed)
DEVICE=${1:-1234}
FILE_INSTANCE=${2:-1}

echo "=== BACnet File Transfer Example ==="
echo "Target device: $DEVICE, File object: $FILE_INSTANCE"
echo ""

# Check if client exists
if [ ! -x "$CLIENT" ]; then
    echo "Error: bacnet-client not found. Please build first:"
    echo "  cd client && make"
    exit 1
fi

# Create a test file to upload
TEST_FILE="/tmp/bacnet_test_upload.txt"
echo "This is a test file for BACnet file transfer." > $TEST_FILE
echo "Created at: $(date)" >> $TEST_FILE

# Read file from device
echo "1. Reading file from device..."
$CLIENT file-read --device $DEVICE --file $FILE_INSTANCE --out /tmp/bacnet_downloaded.txt || echo "  (File object may not exist)"
echo ""

if [ -f /tmp/bacnet_downloaded.txt ]; then
    echo "   Downloaded file contents:"
    head -10 /tmp/bacnet_downloaded.txt
    echo ""
fi

# Write file to device (commented out for safety)
# echo "2. Writing file to device..."
# $CLIENT file-write --device $DEVICE --file $FILE_INSTANCE --in $TEST_FILE

echo "=== File Transfer Complete ==="

# Cleanup
rm -f $TEST_FILE
