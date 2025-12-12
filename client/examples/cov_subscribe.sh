#!/bin/bash
# BACnet Client Example: COV Subscriptions
# This script demonstrates COV (Change of Value) subscriptions

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="${SCRIPT_DIR}/../bin/bacnet-client"

# Default device instance (change as needed)
DEVICE=${1:-1234}
OBJECT_TYPE=${2:-analog-input}
OBJECT_INSTANCE=${3:-1}
LIFETIME=${4:-300}

echo "=== BACnet COV Subscription Example ==="
echo "Target: Device $DEVICE, $OBJECT_TYPE:$OBJECT_INSTANCE"
echo "Lifetime: $LIFETIME seconds"
echo ""

# Check if client exists
if [ ! -x "$CLIENT" ]; then
    echo "Error: bacnet-client not found. Please build first:"
    echo "  cd client && make"
    exit 1
fi

echo "Subscribing to COV notifications..."
echo "Press Ctrl+C to unsubscribe and exit"
echo ""

# Subscribe with unconfirmed notifications
$CLIENT cov-subscribe \
    --device $DEVICE \
    --object ${OBJECT_TYPE}:${OBJECT_INSTANCE} \
    --lifetime $LIFETIME

echo ""
echo "=== COV Subscription Ended ==="
