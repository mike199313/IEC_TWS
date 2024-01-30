SUMMARY = "Inventec mac address config service"
DESCRIPTION = "Setting mac address from eeprom"
PR = "r1"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd

DEPENDS += "systemd inventec-util"
RDEPENDS:${PN} += "libsystemd"


FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://inventec-mac-config.sh"

S = "${WORKDIR}"

do_install() {
        install -d ${D}${sbindir}
        install -m 0755 inventec-mac-config.sh ${D}${sbindir}
}

SYSTEMD_SERVICE:${PN} += "inventec-mac-config.service"
