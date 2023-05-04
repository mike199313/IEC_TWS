SUMMARY = "Intel cups Utility"
DESCRIPTION = "cups utility"

LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=858e10cab31872ff374c48e5240d566a"

EXTERNALSRC = "${THISDIR}/${PN}"
EXTERNALSRC_BUILD = "${B}"

SRC_URI += " file://xyz.openbmc_project.CupsService.service"

S = "${WORKDIR}"

DEPENDS = "boost sdbusplus cli11 libpeci systemd"
inherit cmake externalsrc pkgconfig systemd obmc-phosphor-dbus-service
inherit obmc-phosphor-systemd

SYSTEMD_SERVICE:${PN} += "xyz.openbmc_project.CupsService.service"
DBUS_SERVICE:${PN} += "xyz.openbmc_project.CupsService.service"

do_install() {
    install -d ${D}${sbindir}
    install -m755 cups-service ${D}${sbindir}/cups-service
}
