SUMMARY = "post complete vgpio detecting"
DESCRIPTION = "Rebind driver to probe successfully after BIOS on."
PR = "r1"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd

DEPENDS += "systemd"
RDEPENDS:${PN} += "libsystemd bash"

FILESEXTRAPATHS:prepend := "${THISDIR}/post-complete:"

FILES:${PN}:append = " ${sbindir}/post-complete.sh"

SRC_URI = "\
            file://post-complete.sh \
          "

S = "${WORKDIR}"

do_install() {
        install -d ${D}${sbindir}
        install -m 0755 post-complete.sh ${D}${sbindir}
}

SYSTEMD_SERVICE:${PN} += "post-complete.service"
