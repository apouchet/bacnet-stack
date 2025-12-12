# BACnet Client Changelog

All notable changes to the BACnet Client are documented in this file.

## [1.0.0] - 2024

### Added

#### CLI Tool (`bin/bacnet-client`)
- Device discovery with `whois` command
  - Broadcast Who-Is messages
  - Filter by device instance range
  - JSON output support
- Property access with `read` and `write` commands
  - ReadProperty support
  - WriteProperty with priority
  - Multiple data type support (numeric, boolean, string, null)
- COV subscriptions with `cov-subscribe` command
  - Confirmed and unconfirmed notifications
  - Configurable lifetime and increment
  - Automatic unsubscribe on exit
- Time synchronization with `time-sync` command
  - Local and UTC time sync
  - Broadcast and directed messages
- File transfer with `file-read` and `file-write` commands
  - AtomicReadFile with segmentation
  - AtomicWriteFile with segmentation
- Device control commands
  - `reinit` for ReinitializeDevice
  - `device-comm-control` for DeviceCommunicationControl
- BBMD support
  - `register-foreign` for foreign device registration
- Global options
  - `--timeout` for APDU timeout
  - `--retries` for retry count
  - `--verbose` for debug output
  - `--json` for machine-readable output
  - `--log-level` for logging control

#### Client Library (`lib/bacnet_client.h`)
- `bacnet_client_init()` / `bacnet_client_shutdown()`
- `bacnet_client_discover()` - Device discovery
- `bacnet_client_read()` - Property read
- `bacnet_client_write()` - Property write
- `bacnet_client_subscribe_cov()` / `bacnet_client_unsubscribe_cov()`
- `bacnet_client_time_sync()` - Time synchronization
- `bacnet_client_read_file()` / `bacnet_client_write_file()`
- `bacnet_client_reinitialize()` - Device reinitialize
- `bacnet_client_device_comm_control()` - Communication control
- `bacnet_client_register_foreign_device()`
- `bacnet_client_ack_alarm()` - Alarm acknowledgment
- Callback-based asynchronous operations
- Comprehensive error handling

#### Documentation
- Complete usage documentation in `docs/README.md`
- PICS conformance statement in `examples/pics.md`
- Example scripts for common operations
- API documentation in header comments

#### Infrastructure
- Makefile build system
- GitHub Actions CI workflow
- Unit test framework
- Integration test script
- Systemd service file
- Docker support

### Protocol Support

#### BACnet Services (Client/Initiator)
- Who-Is / I-Am (Discovery)
- ReadProperty
- ReadPropertyMultiple (via library)
- WriteProperty
- WritePropertyMultiple (via library)
- SubscribeCOV
- TimeSynchronization
- UTCTimeSynchronization
- AtomicReadFile
- AtomicWriteFile
- ReinitializeDevice
- DeviceCommunicationControl
- AcknowledgeAlarm

#### Data Link Layer
- BACnet/IP (Annex J)
- Foreign Device Registration
- BBMD Client

### Known Limitations
- BACnet/SC not supported (use gateway)
- MS/TP requires router/gateway
- Complex type display may be limited

## Future Roadmap

### Planned Features
- ReadPropertyMultiple CLI command
- WritePropertyMultiple CLI command
- GetEventInformation command
- GetAlarmSummary command
- AddListElement / RemoveListElement
- ReadRange for trend logs
- Configuration file support (YAML)
- Metrics/statistics output
- Structured logging (syslog)

### Under Consideration
- BACnet/SC support (when bacnet-stack adds support)
- Web interface
- MQTT bridge
- Database logging
