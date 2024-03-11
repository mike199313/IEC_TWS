SUMMARY = "superion init service"
DESCRIPTION = "Essential init commands for superion"
PR = "r1"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd

DEPENDS += "systemd"
RDEPENDS:${PN} += "libsystemd"


FILESEXTRAPATHS:prepend := "${THISDIR}/superion-init:"
SRC_URI += "file://superion-init.sh \
            file://superion-fan-init.sh \
            file://superion-gpio-init.sh \
            file://superion-cpld-init.sh \
            "

S = "${WORKDIR}"

do_install() {
        install -d ${D}${sbindir}
        install -m 0755 superion-init.sh ${D}${sbindir}
        install -m 0755 superion-fan-init.sh ${D}${sbindir}
        install -m 0755 superion-gpio-init.sh ${D}${sbindir}
        install -m 0755 superion-cpld-init.sh ${D}${sbindir}
}

SYSTEMD_SERVICE:${PN} += "superion-init.service"
