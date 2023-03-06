DESCRIPTION = "Inventec utilities"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

FILESEXTRAPATHS:prepend := "${THISDIR}:"

inherit cmake obmc-phosphor-systemd

DEPENDS += "systemd sdbusplus libgpiod phosphor-dbus-interfaces"
RDEPENDS:${PN} += "libsystemd sdbusplus libgpiod"


S = "${WORKDIR}/${BPN}"

SRC_URI = "file://${BPN}/CMakeLists.txt             \
           file://${BPN}/include/eeprom_util.hpp       \
           file://${BPN}/include/mac_util.hpp       \
           file://${BPN}/include/util.hpp       \
           file://${BPN}/README.md                  \
           file://${BPN}/src/eeprom_util.cpp           \
           file://${BPN}/src/mac_util.cpp           \
           file://${BPN}/mac_util_config@.service   \
           file://${BPN}/mac_util_config.sh         \
           file://${BPN}/eeprom-manager.service   \
           file://${BPN}/post_bootup.sh         \
           file://${BPN}/pre_shutdown.sh         \
           file://${BPN}/post-bootup.service   \
           file://${BPN}/pre-shutdown.service   \
"

SYSTEMD_SERVICE:${BPN} += "mac_util_config@.service"

SYSTEMD_SERVICE:${BPN} += "eeprom-manager.service"

SYSTEMD_SERVICE:${BPN} += "post-bootup.service pre-shutdown.service"

EXTRA_OECMAKE=""
