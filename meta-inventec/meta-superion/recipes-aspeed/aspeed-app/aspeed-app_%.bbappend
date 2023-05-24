FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://idcode.svf"

do_install:append() {
    install -d 0755 ${D}/usr/share/aspeed-app/svf
    install -m 0644 ${WORKDIR}/idcode.svf ${D}/usr/share/aspeed-app/svf
}
