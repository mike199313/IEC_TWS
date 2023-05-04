SUMMARY = "Intel node-manager Utility"
DESCRIPTION = "node-manager utility"

LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ca52b323cca9072ccd0b842f1a668258"

EXTERNALSRC = "${THISDIR}/${PN}"
EXTERNALSRC_BUILD = "${B}"

SRC_URI += " file://xyz.openbmc_project.NodeManager.service"

S = "${WORKDIR}"

DEPENDS = "boost sdbusplus systemd cli11 libpeci phosphor-ipmi-host phosphor-dbus-interfaces phosphor-logging nlohmann-json"
inherit cmake externalsrc pkgconfig systemd obmc-phosphor-dbus-service obmc-phosphor-ipmiprovider-symlink

SYSTEMD_SERVICE:${PN} += "xyz.openbmc_project.NodeManager.service"
DBUS_SERVICE:${PN} += "xyz.openbmc_project.NodeManager.service"

do_install() {
    install -d ${D}${sbindir}
    install -m755 node-manager ${D}${sbindir}/node-manager
}

EXTRA_OECMAKE += "-DYOCTO_DEPENDENCIES=ON"
