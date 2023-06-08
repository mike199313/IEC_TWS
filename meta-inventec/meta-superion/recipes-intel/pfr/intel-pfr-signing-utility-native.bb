SUMMARY = "Intel(R) Platform Firmware Resilience Signing Utility"
DESCRIPTION = "Image signing tool for building Intel(R) PFR image"

inherit cmake native

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

DEPENDS = "openssl-native libxml2-native "

SRC_URI = "git://github.com/Intel-BMC/intel-pfr-signing-utility;protocol=https;branch=master \
           file://0001-support-openssl-3.0.patch \
           file://0002-fix-verify-error-if-block1-b0sig-hashalg-set-to-sha384.patch \
          "

SRCREV = "2c6f15434db57e5f51e3b1a4817f0e621a5bad25"

S = "${WORKDIR}/git"

do_install:append() {
   install -d ${D}/${bindir}
   install -m 775 ${B}/intel-pfr-signing-utility ${D}/${bindir}
}
