FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
  file://motherboard.json \
  file://blacklist.json \
  file://runbmc.json \
  file://scmbridge.json \
  file://bp0.json \
  file://bp1.json \
  file://artesyn_csu2400ap-3-100_psu1.json \
  file://artesyn_csu2400ap-3-100_psu2.json \
"

do_install:append() {
  install -d 0755 ${D}${datadir}/${PN}/configurations/
  rm -v -f ${D}${datadir}/${PN}/configurations/*.json
  install -m 0644 ${WORKDIR}/motherboard.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/blacklist.json ${D}${datadir}/${PN}/
  install -m 0644 ${WORKDIR}/runbmc.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/scmbridge.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/bp0.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/bp1.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/artesyn_csu2400ap-3-100_psu1.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/artesyn_csu2400ap-3-100_psu2.json ${D}${datadir}/${PN}/configurations
}

DISTRO_FEATURES += "ipmi-fru"
