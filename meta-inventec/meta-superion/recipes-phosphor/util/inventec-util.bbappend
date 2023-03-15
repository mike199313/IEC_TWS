FILESEXTRAPATHS:append:superion := "${THISDIR}/${PN}:"

SRC_URI:append = " file://mac_util.hpp \
                 "


do_copyfile () {
    if [ -e ${WORKDIR}/mac_util.hpp ] ; then
        cp -v ${WORKDIR}/mac_util.hpp ${S}/include/
    else
        # if use devtool modify, then the append files were stored under oe-local-files
        cp -v ${S}/oe-local-files/mac_util.hpp  ${S}/include/
    fi
}

addtask do_copyfile after do_patch before do_compile


