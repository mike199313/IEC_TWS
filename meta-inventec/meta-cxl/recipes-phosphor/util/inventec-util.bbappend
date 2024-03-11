FILESEXTRAPATHS:append:${MACHINE} := "${THISDIR}/${PN}:"

SRC_URI:append = " file://mac_util.hpp \
                   file://pfr-update   \
                   file://pfr-update.service   \
                 "

SYSTEMD_SERVICE:${BPN} += "pfr-update.service"

do_copyfile () {
    if [ -e ${WORKDIR}/mac_util.hpp ] ; then
        cp -v ${WORKDIR}/mac_util.hpp ${S}/include/
    else
        # if use devtool modify, then the append files were stored under oe-local-files
        cp -v ${S}/oe-local-files/mac_util.hpp  ${S}/include/
    fi

    if [ -e ${WORKDIR}/pfr-update ] ; then
        cp -v ${WORKDIR}/pfr-update* ${S}/
    else
        # if use devtool modify, then the append files were stored under oe-local-files
        cp -v ${S}/oe-local-files/pfr-update*  ${S}/
    fi
}

do_install:append () {
    install -d ${D}${bindir}
    install -m 0755 ${S}/pfr-update ${D}${bindir}
}

addtask do_copyfile after do_patch before do_compile


