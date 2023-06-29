inherit obmc-phosphor-systemd

SUMMARY = "Intel ASD Utility"
DESCRIPTION = "Intel At-Scale Debug utility"

LICENSE = "INTEL"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8929d33c051277ca2294fe0f5b062f38"

SRC_URI = "git://github.com/Intel-BMC/asd;protocol=https;branch=${BRANCH} \
           file://intel.asd.service \
           file://0001-Pick-up-i3cdev.h-from-linux-aspeed.patch \
           "
SRCREV = "5f6d69696bd1114c38041faad120b3fb6f661b78"
BRANCH = "master"

S = "${WORKDIR}/git"
PV = "0.1+git${SRCPV}"

inherit cmake
DEPENDS = "sdbusplus openssl libpam libgpiod safec"
RDEPENDS:${PN} = "safec"

do_configure[depends] += "virtual/kernel:do_shared_workdir"

# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = "-DBUILD_UT=OFF"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SYSTEMD_SERVICE:${PN} += "intel.asd.service"

do_install:append() {
    install -d ${D}/lib/systemd/system/
    install -m 0644 ${WORKDIR}/intel.asd.service ${D}/lib/systemd/system/
}
