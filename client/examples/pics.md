# BACnet Protocol Implementation Conformance Statement (PICS)

## Product Identification

| Field | Value |
|-------|-------|
| Vendor Name | BACnet Stack Contributors |
| Product Name | bacnet-client |
| Product Model Number | 1.0.0 |
| Product Description | BACnet/IP Client for Linux |
| BACnet Protocol Revision | 24 |

## BACnet Standardized Device Profile (Annex L)

**Profile**: BACnet Application Specific Controller (B-ASC) - Client Only

This implementation acts as a BACnet client/initiator and does not serve objects to other devices.

## BACnet Interoperability Building Blocks Supported (BIBBs)

### Data Sharing Services

| BIBB | Service | Role | Status |
|------|---------|------|--------|
| DS-RP-B | ReadProperty | Client (B) | ✅ Supported |
| DS-RPM-B | ReadPropertyMultiple | Client (B) | ✅ Supported |
| DS-WP-B | WriteProperty | Client (B) | ✅ Supported |
| DS-WPM-B | WritePropertyMultiple | Client (B) | ✅ Supported |
| DS-COV-B | SubscribeCOV | Client (B) | ✅ Supported |
| DS-COVP-B | SubscribeCOVProperty | Client (B) | ⚠️ Partial |
| DS-COVU-B | UnconfirmedCOVNotification | Receiver | ✅ Supported |

### Alarm and Event Services

| BIBB | Service | Role | Status |
|------|---------|------|--------|
| AE-N-B | EventNotification | Receiver | ✅ Supported |
| AE-ACK-B | AcknowledgeAlarm | Client (B) | ✅ Supported |
| AE-INFO-B | GetEventInformation | Client (B) | ⚠️ Partial |
| AE-ASUM-B | GetAlarmSummary | Client (B) | ⚠️ Partial |

### File Access Services

| BIBB | Service | Role | Status |
|------|---------|------|--------|
| T-ATR-B | AtomicReadFile | Client (B) | ✅ Supported |
| T-AWF-B | AtomicWriteFile | Client (B) | ✅ Supported |

### Device Management Services

| BIBB | Service | Role | Status |
|------|---------|------|--------|
| DM-DDB-B | Who-Is / I-Am | Initiator | ✅ Supported |
| DM-DOB-B | Who-Has / I-Have | Initiator | ⚠️ Partial |
| DM-DCC-B | DeviceCommunicationControl | Client (B) | ✅ Supported |
| DM-RD-B | ReinitializeDevice | Client (B) | ✅ Supported |
| DM-TS-B | TimeSynchronization | Initiator | ✅ Supported |
| DM-UTC-B | UTCTimeSynchronization | Initiator | ✅ Supported |

### Virtual Terminal Services

Not supported (not applicable for client application).

### Network Management Services

| BIBB | Service | Role | Status |
|------|---------|------|--------|
| NM-RC-B | Who-Is-Router-To-Network | Initiator | ⚠️ Partial |

## Segmentation Capability

| Direction | Supported | Max Segments | Max APDU |
|-----------|-----------|--------------|----------|
| Transmit Segmented Requests | Yes | 64 | 1476 |
| Receive Segmented Requests | Yes | 64 | 1476 |
| Transmit Segmented Responses | Yes | 64 | 1476 |
| Receive Segmented Responses | Yes | 64 | 1476 |

## Standard Object Types Supported

As a client-only application, this implementation does not host BACnet objects. However, it can interact with all standard object types on remote devices.

### Object Types Readable/Writable

| Object Type | Read | Write | COV |
|-------------|------|-------|-----|
| Analog Input | ✅ | N/A | ✅ |
| Analog Output | ✅ | ✅ | ✅ |
| Analog Value | ✅ | ✅ | ✅ |
| Binary Input | ✅ | N/A | ✅ |
| Binary Output | ✅ | ✅ | ✅ |
| Binary Value | ✅ | ✅ | ✅ |
| Multi-state Input | ✅ | N/A | ✅ |
| Multi-state Output | ✅ | ✅ | ✅ |
| Multi-state Value | ✅ | ✅ | ✅ |
| Device | ✅ | Partial | N/A |
| File | ✅ | ✅ | N/A |
| Trend Log | ✅ | N/A | N/A |
| Schedule | ✅ | ✅ | N/A |
| Calendar | ✅ | ✅ | N/A |
| Notification Class | ✅ | ✅ | N/A |
| All Others | ✅ | ✅ | * |

\* COV support depends on the object type definition.

## Data Link Layer Options

### BACnet/IP (Annex J)

| Feature | Status |
|---------|--------|
| BACnet/IP | ✅ Supported |
| UDP Port | 47808 (configurable) |
| Foreign Device Registration | ✅ Supported |
| BBMD Client | ✅ Supported |
| Original-Unicast-NPDU | ✅ Supported |
| Original-Broadcast-NPDU | ✅ Supported |
| Forwarded-NPDU | ✅ Supported |

### BACnet/IPv6 (Annex U)

| Feature | Status |
|---------|--------|
| BACnet/IPv6 | ⚠️ Partial (via bacnet-stack) |

### MS/TP (Clause 9)

| Feature | Status |
|---------|--------|
| MS/TP Master | ❌ Not supported directly |
| MS/TP Slave | ❌ Not supported directly |
| Via Router/Gateway | ✅ Supported |

### BACnet/Ethernet (Clause 7)

| Feature | Status |
|---------|--------|
| BACnet/Ethernet | ❌ Not supported |

### BACnet/SC (Annex AB)

| Feature | Status |
|---------|--------|
| BACnet/SC | ❌ Not supported |
| TLS Support | ❌ Not supported |
| WebSocket | ❌ Not supported |

## Character Sets Supported

| Character Set | Status |
|---------------|--------|
| ANSI X3.4 | ✅ Supported |
| UTF-8 | ✅ Supported |
| ISO 8859-1 | ✅ Supported |
| UCS-2 | ⚠️ Partial |
| UCS-4 | ⚠️ Partial |

## Special Functionality

### Proprietary Properties

The client can read/write proprietary properties using numeric property identifiers.

### Private Transfer

Basic support for sending private transfer requests with vendor-specific payloads.

### Network Security

| Feature | Status |
|---------|--------|
| Network Security | ❌ Not supported |
| Authentication | ❌ Not supported |

Note: Network security should be implemented at the network level (VLANs, firewalls, VPNs).

## Conformance Testing

This implementation has not been formally tested by a BACnet Testing Laboratory (BTL). It is provided as-is for development and integration purposes.

## Known Limitations

1. **MS/TP**: Direct MS/TP communication is not supported. Use a BACnet router.
2. **BACnet/SC**: Secure Connect is not supported. Use network-level security.
3. **Complex Types**: Some complex BACnet types may not display correctly in CLI output.
4. **Alarm Summary**: GetAlarmSummary parsing may be incomplete.
5. **Priority Array**: Full priority array manipulation may require multiple operations.

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024 | Initial release |

## Contact Information

For support and updates, see the project repository:
- Repository: https://github.com/bacnet-stack/bacnet-stack
- Documentation: See client/docs/README.md
