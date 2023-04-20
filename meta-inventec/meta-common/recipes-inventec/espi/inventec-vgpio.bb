DESCRIPTION = "Inventec espi vGPIO functions"

LICENSE = "CLOSED"

S = "${WORKDIR}/${BPN}"
EXTERNALSRC_SYMLINKS = ""
EXTERNALSRC = "${THISDIR}/${PN}"
EXTERNALSRC_BUILD = "${B}"

DEPENDS = "boost nlohmann-json phosphor-logging"

inherit meson externalsrc pkgconfig
