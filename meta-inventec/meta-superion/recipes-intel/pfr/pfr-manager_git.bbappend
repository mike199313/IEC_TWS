FILESEXTRAPATHS:append := "${THISDIR}/${BPN}:"

inherit cmake pkgconfig systemd

SRC_URI += " file://0001-Enhance-the-DBus-properties-of-the-pfr-manager.patch \
           "
