DESCRIPTION = "Inventec mac utility"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

FILESEXTRAPATHS:append := "${THISDIR}/mac-util:"

S = "${WORKDIR}"

SRC_URI += "file://mac_util.py"

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${S}/mac_util.py ${D}${bindir}
}
