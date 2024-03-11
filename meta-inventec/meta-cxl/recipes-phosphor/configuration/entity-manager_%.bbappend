FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
  file://hpm.json \
  file://dcscm.json \
  file://fan_bd.json \
"

do_install:append() {
  install -d 0755 ${D}${datadir}/${PN}/configurations/
  rm -v -f ${D}${datadir}/${PN}/configurations/*.json
  install -m 0644 ${WORKDIR}/fan_bd.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/dcscm.json ${D}${datadir}/${PN}/configurations
  install -m 0644 ${WORKDIR}/hpm.json ${D}${datadir}/${PN}/configurations
}

DISTRO_FEATURES += "ipmi-fru"
