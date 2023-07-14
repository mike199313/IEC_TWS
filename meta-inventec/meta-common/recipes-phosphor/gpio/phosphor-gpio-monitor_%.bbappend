FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI += "file://phosphor-multi-gpio-monitor.json"
SRC_URI += "file://phosphor-multi-gpio-monitor.service"
SRC_URI += "file://0001-support-gpioevent-for-edge-in-both-directions.patch"
SRC_URI += "file://0002-support-triggering-multiple-services.patch"

do_install:append(){
    install -d ${D}/usr/share/phosphor-gpio-monitor
    install -m 0444 ${WORKDIR}/phosphor-multi-gpio-monitor.json ${D}/usr/share/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json
}
