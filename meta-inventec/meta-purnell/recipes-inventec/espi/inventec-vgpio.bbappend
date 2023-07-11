FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI:append = " file://inventec-vgpio.json \
                 "


do_install:append() {
    install -d ${D}${datadir}/${PN}
    install -m 0644 ${WORKDIR}/inventec-vgpio.json ${D}${datadir}/${PN}
}
