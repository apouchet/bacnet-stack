# BACnet Client

A production-quality BACnet/IP client implementation for Linux.

## Quick Start

```bash
# Build
cd client
make

# Discover devices
./bin/bacnet-client whois

# Read property
./bin/bacnet-client read --device 1234 --object analog-input:1 --property present-value

# Write property
./bin/bacnet-client write --device 1234 --object analog-output:1 --property present-value --value 75.5
```

## Directory Structure

```
client/
├── bin/              # CLI binary source
│   └── main.c        # Main CLI implementation
├── lib/              # Client library
│   ├── bacnet_client.h   # Public API header
│   └── bacnet_client.c   # Library implementation
├── examples/         # Example scripts
│   ├── discovery.sh
│   ├── read_write.sh
│   ├── cov_subscribe.sh
│   ├── time_sync.sh
│   ├── file_transfer.sh
│   └── register_foreign.sh
├── tests/            # Test suite
│   ├── test_main.c       # Unit tests
│   └── run_integration.sh
├── docs/             # Documentation
│   └── README.md     # Detailed documentation
├── systemd/          # Systemd service files
├── Dockerfile        # Docker build
├── Makefile          # Build system
└── README.md         # This file
```

## Documentation

See [docs/README.md](docs/README.md) for:
- Complete command reference
- Configuration options
- Library API documentation
- PICS conformance statement
- Troubleshooting guide

## Examples

See [examples/pics.md](examples/pics.md) for the Protocol Implementation Conformance Statement.

## License

MIT License - See LICENSE file in repository root.
