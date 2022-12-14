FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

KERNEL_FEATURES:remove += " phosphor-gpio-keys \
                            phosphor-vlan \
                          "




SRC_URI:append = " file://superion.cfg \
                   file://arch \
                   file://0001-Kernel-sync-Aspeed-tag-00.05.03-soc-i3c-drivers.patch \
                 "

do_add_overwrite_files () {
    cp -r "${WORKDIR}/arch" \
          "${STAGING_KERNEL_DIR}"
}

addtask do_add_overwrite_files after do_patch before do_compile

