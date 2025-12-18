# Yocto Recipe for BACnet MQTT Server

This directory contains a Yocto/OpenEmbedded recipe for building the `bacnet-mqtt-server` application. The recipe is compatible with **Yocto Dunfell (3.1)** and later releases.

## Prerequisites

Your Yocto build environment must include the following layers:

- **meta-openembedded/meta-oe** - For `mosquitto`, `json-c`, and `libyaml` recipes
- **meta-openembedded/meta-networking** - For mosquitto dependencies

## Installation

### Option 1: Copy to your meta layer

1. Create the recipe directory in your meta layer:
   ```bash
   mkdir -p meta-yourlayer/recipes-connectivity/bacnet-mqtt-server
   ```

2. Copy the recipe file:
   ```bash
   cp bacnet-mqtt-server_git.bb meta-yourlayer/recipes-connectivity/bacnet-mqtt-server/
   ```

3. Add to your image recipe or `local.conf`:
   ```bitbake
   IMAGE_INSTALL_append = " bacnet-mqtt-server"
   ```

### Option 2: Create a bbappend

If you prefer to use a bbappend for customization, create:
```
meta-yourlayer/recipes-connectivity/bacnet-mqtt-server/bacnet-mqtt-server_git.bbappend
```

## Configuration

### Using a specific version

To pin to a specific git commit, modify the recipe:
```bitbake
SRCREV = "your-commit-hash"
PV = "1.0.0+git${SRCPV}"
```

### Enabling the systemd service

By default, the service is disabled. To enable at boot:
```bitbake
SYSTEMD_AUTO_ENABLE = "enable"
```

Or add to your image recipe:
```bitbake
SYSTEMD_AUTO_ENABLE_bacnet-mqtt-server = "enable"
```

## Build

```bash
bitbake bacnet-mqtt-server
```

## Installed Files

| Path | Description |
|------|-------------|
| `/usr/bin/bacnet-mqtt-server` | Main binary |
| `/lib/systemd/system/bacnet-mqtt-server.service` | Systemd service file |
| `/etc/bacnet-mqtt-server/config.yaml` | Main configuration |
| `/var/config/scada/sensors_maps.json` | Sensor mapping |
| `/var/config/scada/sensors/lora/...` | Sensor definitions |

## Runtime Configuration

After installation, configure the server:

1. Edit `/etc/bacnet-mqtt-server/config.yaml` with your MQTT broker settings
2. Update `/var/config/scada/sensors_maps.json` with your device mappings
3. Add sensor definitions to `/var/config/scada/sensors/lora/`

Start the service:
```bash
systemctl start bacnet-mqtt-server
```

## Customization

### Adding additional sensor types

Add new sensor definitions to the recipe by extending the `do_install` task:

```bitbake
# In your bbappend file
do_install_append() {
    install -d ${D}${localstatedir}/config/scada/sensors/lora/custom
    install -m 0644 ${WORKDIR}/my-sensor.json ${D}${localstatedir}/config/scada/sensors/lora/custom/
}
```

### Custom configuration

To use a custom configuration file, add it to `SRC_URI` and modify installation:

```bitbake
SRC_URI_append = " file://my-config.yaml"

do_install_append() {
    install -m 0644 ${WORKDIR}/my-config.yaml ${D}${sysconfdir}/bacnet-mqtt-server/config.yaml
}
```

## Troubleshooting

### Missing dependencies

If you see errors about missing `mosquitto`, `json-c`, or `libyaml`:

1. Ensure `meta-openembedded` layers are included in `bblayers.conf`:
   ```bash
   bitbake-layers add-layer ../meta-openembedded/meta-oe
   bitbake-layers add-layer ../meta-openembedded/meta-networking
   ```

### Build failures

For verbose build output:
```bash
bitbake bacnet-mqtt-server -v
```

Check the work directory for logs:
```bash
ls tmp/work/*/bacnet-mqtt-server/*/temp/
```

## License

MIT License - See the main repository LICENSE file.
