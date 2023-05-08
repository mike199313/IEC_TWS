FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

inherit pkgconfig


SRC_URI:append  = " file://0001-changes-to-xyz.patch \
                  "

