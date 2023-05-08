FILESEXTRAPATHS:prepend:superion := "${THISDIR}/${PN}:"

SRC_URI:append:superion = " \
  file://motherboard.json \
"

do_install:append:superion() {
  install -d 0755 ${D}${datadir}/${PN}/configurations/
  rm -v -f ${D}${datadir}/${PN}/configurations/*.json
  install -m 0644 ${WORKDIR}/motherboard.json ${D}${datadir}/${PN}/configurations
}
