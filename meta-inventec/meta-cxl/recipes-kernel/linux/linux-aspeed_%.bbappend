FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI:append = " file://superion.cfg \
                   file://arch \
                   file://0001-Kernel-sync-Aspeed-tag-00.05.03-soc-i3c-drivers.patch \
                   file://0002-Kernel-sync-Aspeed-tag-00.05.03-misc-drivers.patch \
                   file://0003-Kernel-sync-Aspeed-tag-00.05.03-dsti.patch \
                   file://0004-sync-Intel-dev-5.15-to-Aspeed-tag-00.05.03-espi.patch \
                   file://0005-Add-SPI_ASPEED-driver-and-add-a-new-Macronix-flash.patch \
                   file://0006-Kernel-sync-intel-peci-drivers.patch \
                   file://0007-Add-cpu-ids-to-support-new-cpu-in-intel-peci-client.patch \
                   file://0008-Update-xdpe152xx-Family-for-kernel-driver.patch \
                   file://0009-aspeed-mctp-Fix-peci-mctp-device-name-error.patch \
                   file://0010-superion-Sync-AspeedTech-for-KCS.patch \
                   file://0011-Modify-JTAG-driver-with-intel-JTAG-driver.patch \
		   file://0012-peci-dimmtemp-presence-detection.patch \
                   file://0013-Cxl_ADT7462_driver_update_inv.patch \
                   file://0014-kernel-driver-update-for-CPU-SRF-SP.patch \
                 "

do_add_overwrite_files () {
    cp -r "${WORKDIR}/arch" \
          "${STAGING_KERNEL_DIR}"
}

addtask do_add_overwrite_files after do_patch before do_compile

