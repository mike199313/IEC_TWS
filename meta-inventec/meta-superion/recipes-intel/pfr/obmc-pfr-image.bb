SUMMARY = "PFR image utils"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

# 1 = SHA256
# 2 = SHA384
PFR_SHA ?= "1"

SRC_URI = " \
           file://pfr_image.py \
           file://pfr_manifest.json \
           file://pfr_manifest_ast2600_dcscm.json \
           file://pfm_config.xml \
           file://bmc_config.xml \
           file://csk_prv.pem \
           file://csk_pub.pem \
           file://rk_pub.pem \
           file://rk_prv.pem \
           file://rk_cert.pem \
           file://csk384_prv.pem \
           file://csk384_pub.pem \
           file://rk384_pub.pem \
           file://rk384_prv.pem \
           file://pfm_config_secp384r1.xml \
           file://bmc_config_secp384r1.xml \
           file://pfr_manifest_ast2600_superion.json \
           file://gen-pfr-tar \
          "

do_install () {
        install -d ${D}${bindir}
        install -d ${D}/${datadir}/pfrconfig
        install -m 775 ${WORKDIR}/pfr_image.py ${D}${bindir}/pfr_image.py
        install -m 400 ${WORKDIR}/*.json ${D}/${datadir}/pfrconfig
        install -m 400 ${WORKDIR}/pfm_config.xml ${D}/${datadir}/pfrconfig/pfm_config.xml
        install -m 400 ${WORKDIR}/bmc_config.xml ${D}/${datadir}/pfrconfig/bmc_config.xml
        install -m 400 ${WORKDIR}/csk_prv.pem ${D}/${datadir}/pfrconfig/csk_prv.pem
        install -m 400 ${WORKDIR}/csk_pub.pem ${D}/${datadir}/pfrconfig/csk_pub.pem
        install -m 400 ${WORKDIR}/rk_pub.pem ${D}/${datadir}/pfrconfig/rk_pub.pem
        install -m 400 ${WORKDIR}/rk_prv.pem ${D}/${datadir}/pfrconfig/rk_prv.pem
        install -m 0644 ${WORKDIR}/rk_cert.pem ${D}/${datadir}/pfrconfig/rk_cert.pem
        install -m 400 ${WORKDIR}/csk384_prv.pem ${D}/${datadir}/pfrconfig/csk384_prv.pem
        install -m 400 ${WORKDIR}/csk384_pub.pem ${D}/${datadir}/pfrconfig/csk384_pub.pem
        install -m 400 ${WORKDIR}/rk384_pub.pem ${D}/${datadir}/pfrconfig/rk384_pub.pem
        install -m 400 ${WORKDIR}/rk384_prv.pem ${D}/${datadir}/pfrconfig/rk384_prv.pem
        install -m 400 ${WORKDIR}/pfm_config_secp384r1.xml ${D}/${datadir}/pfrconfig/pfm_config_secp384r1.xml
        install -m 400 ${WORKDIR}/bmc_config_secp384r1.xml ${D}/${datadir}/pfrconfig/bmc_config_secp384r1.xml
        install -m 775 ${WORKDIR}/gen-pfr-tar ${D}${bindir}/gen-pfr-tar
}

do_install:class-target () {
        install -d ${D}/${datadir}/pfrconfig

        if [ "${PFR_SHA}" == "1" ]; then
                install -m 400 ${WORKDIR}/rk_pub.pem ${D}/${datadir}/pfrconfig/rk_pub.pem
        else
                install -m 400 ${WORKDIR}/rk384_pub.pem ${D}/${datadir}/pfrconfig/rk384_pub.pem
        fi
}

FILES:${PN}:append = " ${datadir}/pfrconfig"

BBCLASSEXTEND = "native nativesdk"
