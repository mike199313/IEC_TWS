FILESEXTRAPATHS:append := "${THISDIR}/${BPN}:"
# In order to reuse and easily maintain, we use the same patch files among u-boot-aspeed-sdk
FILESEXTRAPATHS:append:= "${THISDIR}/u-boot-aspeed-sdk:"


SRC_URI:append = " file://fw_env.config \
                   file://superion-ast2600.cfg \
                   file://superion-ast2600_defconfig \
                   file://ast2600-superion.dts \
                 "

do_install:append () {
        install -d ${D}${sysconfdir}
        install -m 0644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
        install -m 0644 ${WORKDIR}/fw_env.config ${S}/tools/env/fw_env.config
}

do_copyfile () {
    if [ -e ${WORKDIR}/ast2600-superion.dts ] ; then
        cp -v ${WORKDIR}/ast2600-superion.dts ${S}/arch/arm/dts/
    else
        # if use devtool modify, then the append files were stored under oe-local-files
        cp -v ${S}/oe-local-files/ast2600-superion.dts ${S}/arch/arm/dts/
    fi

    if [ -e ${WORKDIR}/superion-ast2600_defconfig  ] ; then
        cp -v ${WORKDIR}/superion-ast2600_defconfig  ${S}/configs/
    else
        cp -v ${S}/oe-local-files/superion-ast2600_defconfig ${S}/configs/
    fi
}

addtask copyfile after do_patch before do_configure
