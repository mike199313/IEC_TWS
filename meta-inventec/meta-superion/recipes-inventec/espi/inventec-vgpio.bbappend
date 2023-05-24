FILESEXTRAPATHS:append:${MACHINE} := "${THISDIR}/${PN}:"

SRC_URI:append = " file://inventec-vgpio.json \
                 "


do_install:append:superion() {
    install -d ${D}${datadir}/${PN}
    install -m 0644 ${WORKDIR}/inventec-vgpio.json ${D}${datadir}/${PN}
}
