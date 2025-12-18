# Yocto Recipe for BACnet MQTT Server
# Compatible with Yocto Dunfell (3.1) and later
#
# This recipe builds the bacnet-mqtt-server application which creates
# a dynamic BACnet server fed by MQTT messages and JSON configuration.
#
# Dependencies:
#   - mosquitto (libmosquitto)
#   - json-c
#   - libyaml
#
# Usage:
#   1. Copy this file to your meta layer:
#      meta-yourlayer/recipes-connectivity/bacnet-mqtt-server/bacnet-mqtt-server_git.bb
#   2. Add to your local.conf or image recipe:
#      IMAGE_INSTALL_append = " bacnet-mqtt-server"

SUMMARY = "Dynamic BACnet server fed by MQTT and JSON configuration"
DESCRIPTION = "A BACnet/IP server that dynamically creates and updates BACnet \
objects based on incoming MQTT messages and JSON sensor definitions. \
Designed for LoRaWAN/IoT sensor integration into building automation systems."
HOMEPAGE = "https://github.com/bacnet-stack/bacnet-stack"
SECTION = "connectivity"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9f4d92e0a3558d8b4a53b1e52ef45278"

# Dependencies
DEPENDS = "mosquitto json-c libyaml"

# Runtime dependencies
RDEPENDS_${PN} = "mosquitto json-c libyaml"

# Source configuration - adjust for your specific repository
SRC_URI = "git://github.com/bacnet-stack/bacnet-stack.git;protocol=https;branch=master"
SRCREV = "${AUTOREV}"

# For a specific release, use:
# SRCREV = "commit-hash-here"
# PV = "1.0.0+git${SRCPV}"

S = "${WORKDIR}/git"

# Build configuration
inherit pkgconfig

# Parallel make
PARALLEL_MAKE = "-j ${@oe.utils.cpu_count()}"

# Build arguments
EXTRA_OEMAKE = " \
    CC='${CC}' \
    AR='${AR}' \
    CFLAGS='${CFLAGS}' \
    LDFLAGS='${LDFLAGS}' \
    LEGACY=true \
    BACNET_PORT=linux \
"

do_compile() {
    # Build the BACnet library first
    oe_runmake -C ${S}/apps/lib clean
    oe_runmake -C ${S}/apps/lib

    # Build bacnet-mqtt-server
    oe_runmake -C ${S}/apps/bacnet-mqtt-server clean
    oe_runmake -C ${S}/apps/bacnet-mqtt-server
}

do_install() {
    # Install binary
    install -d ${D}${bindir}
    install -m 0755 ${S}/bin/bacnet-mqtt-server ${D}${bindir}/

    # Install systemd service file
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/apps/bacnet-mqtt-server/examples/bacnet-mqtt-server.service ${D}${systemd_system_unitdir}/

    # Install configuration directory and example files
    install -d ${D}${sysconfdir}/bacnet-mqtt-server
    install -m 0644 ${S}/apps/bacnet-mqtt-server/examples/config.yaml ${D}${sysconfdir}/bacnet-mqtt-server/

    # Install sensor configuration examples
    install -d ${D}${localstatedir}/config/scada/sensors/lora/nexelec
    install -d ${D}${localstatedir}/config/scada/sensors/lora/generic
    install -m 0644 ${S}/apps/bacnet-mqtt-server/examples/sensors_map.json ${D}${localstatedir}/config/scada/
    install -m 0644 ${S}/apps/bacnet-mqtt-server/examples/sensors/lora/nexelec/X565LS.json ${D}${localstatedir}/config/scada/sensors/lora/nexelec/
    install -m 0644 ${S}/apps/bacnet-mqtt-server/examples/sensors/lora/generic/temp-humidity.json ${D}${localstatedir}/config/scada/sensors/lora/generic/
}

# Systemd support
inherit systemd

SYSTEMD_SERVICE_${PN} = "bacnet-mqtt-server.service"
SYSTEMD_AUTO_ENABLE = "disable"

# Package configuration files separately
CONFFILES_${PN} = " \
    ${sysconfdir}/bacnet-mqtt-server/config.yaml \
    ${localstatedir}/config/scada/sensors_map.json \
"

# Files to package
FILES_${PN} = " \
    ${bindir}/bacnet-mqtt-server \
    ${systemd_system_unitdir}/bacnet-mqtt-server.service \
    ${sysconfdir}/bacnet-mqtt-server/* \
    ${localstatedir}/config/scada/* \
"

# Documentation
FILES_${PN}-doc = "${docdir}/${PN}"
