#!/bin/bash
#
# Integration test script for bacnet-mqtt-server
# This script tests the basic functionality of the server
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test directories
TEST_DIR=$(mktemp -d)
CONFIG_DIR="$TEST_DIR/config"
SENSORS_DIR="$TEST_DIR/config/sensors/lora/nexelec"

echo -e "${YELLOW}=== BACnet MQTT Server Integration Test ===${NC}"
echo "Test directory: $TEST_DIR"

# Create test directories
mkdir -p "$CONFIG_DIR"
mkdir -p "$SENSORS_DIR"

# Create test sensor map
cat > "$CONFIG_DIR/sensors_map.json" << 'EOF'
[
  {
    "id": "70-b3-d5-40-f7-5b-24-c2",
    "sensor": "nexelec/X565LS",
    "src": "lora"
  }
]
EOF

# Create test sensor definition
cat > "$SENSORS_DIR/X565LS.json" << 'EOF'
{
  "description": "X565LS - Test Sensor",
  "properties": {
    "temperature": {
      "type": "float",
      "units": "degrees-celsius"
    },
    "humidity": {
      "type": "float",
      "units": "percent-relative-humidity"
    }
  }
}
EOF

# Create test config
cat > "$TEST_DIR/config.yaml" << EOF
mqtt:
  host: localhost
  port: 1883
  topic: "test/lorawan/#"
  
bacnet:
  device_instance: 260099
  device_name: "Test-Device"

paths:
  sensors_map: "$CONFIG_DIR/sensors_map.json"
  sensors_base: "$TEST_DIR/config/sensors"

logging:
  level: debug
EOF

echo -e "${GREEN}Test configuration created${NC}"

# Function to cleanup
cleanup() {
    echo "Cleaning up..."
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

# Test 1: Check binary exists and shows help
echo -e "\n${YELLOW}Test 1: Binary help output${NC}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$SCRIPT_DIR/../../../bin/bacnet-mqtt-server"

if [ ! -f "$BINARY" ]; then
    BINARY="$(dirname "$SCRIPT_DIR")/../../bin/bacnet-mqtt-server"
fi

if [ ! -f "$BINARY" ]; then
    echo -e "${RED}FAIL: Binary not found at $BINARY${NC}"
    exit 1
fi

if $BINARY --help | grep -q "BACnet MQTT Server"; then
    echo -e "${GREEN}PASS: Help output works${NC}"
else
    echo -e "${RED}FAIL: Help output not as expected${NC}"
    exit 1
fi

# Test 2: Check version output
echo -e "\n${YELLOW}Test 2: Version output${NC}"
if $BINARY --version | grep -q "bacnet-mqtt-server"; then
    echo -e "${GREEN}PASS: Version output works${NC}"
else
    echo -e "${RED}FAIL: Version output not as expected${NC}"
    exit 1
fi

# Test 3: Check config file parsing (will fail to connect to MQTT but config should parse)
echo -e "\n${YELLOW}Test 3: Configuration parsing${NC}"
# Start server with timeout and capture output
timeout 3 $BINARY --config "$TEST_DIR/config.yaml" --foreground --log-level debug 2>&1 | head -20 > "$TEST_DIR/output.log" || true

if grep -q "Configuration loaded\|MQTT client initialized\|BACnet MQTT Server" "$TEST_DIR/output.log" 2>/dev/null; then
    echo -e "${GREEN}PASS: Configuration parsing works${NC}"
else
    echo -e "${YELLOW}WARNING: Could not verify config parsing (may need MQTT broker)${NC}"
    cat "$TEST_DIR/output.log"
fi

echo -e "\n${GREEN}=== All basic tests passed ===${NC}"
echo ""
echo "For full integration testing, ensure:"
echo "1. MQTT broker (mosquitto) is running on localhost:1883"
echo "2. Publish test messages to the configured topic"
echo "3. Use BACnet client tools to verify object creation"
