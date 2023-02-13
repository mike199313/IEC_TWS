FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

KERNEL_FEATURES:remove += " phosphor-gpio-keys \
                            phosphor-vlan \
                          "




SRC_URI:append = " file://superion.cfg \
                   file://arch \
                   file://0001-Kernel-sync-Aspeed-tag-00.05.03-soc-i3c-drivers.patch \
                   file://0002-Kernel-sync-Aspeed-tag-00.05.03-misc-drivers.patch \
                   file://0003-Kernel-sync-Aspeed-tag-00.05.03-dsti.patch \
                   file://0004-sync-Intel-dev-5.15-to-Aspeed-tag-00.05.03-espi.patch \
                   file://0005-Add-SPI_ASPEED-driver-and-add-a-new-Macronix-flash.patch \
                 "

do_add_overwrite_files () {
    cp -r "${WORKDIR}/arch" \
          "${STAGING_KERNEL_DIR}"
}

addtask do_add_overwrite_files after do_patch before do_compile

