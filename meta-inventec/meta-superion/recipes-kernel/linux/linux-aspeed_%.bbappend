FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

KERNEL_FEATURES:remove += " phosphor-gpio-keys \
                            phosphor-vlan \
                          "


SRCREV = "45451e8015a91de5d1a512c3e3d7373bbcb58fb0"



SRC_URI:append = " file://superion.cfg \
                   file://arch \
                   file://drivers \
                   file://0001-Kernel-sync-Aspeed-tag-00.05.00-misc-drivers.patch \
                   file://0002-Kernel-sync-Aspeed-tag-00.05.00-soc-drivers.patch \
                   file://0003-Kernel-sync-Aspeed-tag-00.05.00-dsti.patch \
                   file://0004-Bug-752-SW-common-kernel-Fix-MTD-partitions-were-del.patch \
                   file://0005-Kernel-sync-Intel-BMC-branch-dev-5.15-intel-peci-dri.patch \
                   file://0006-Kernel-sync-Intel-only-Intel-BMC-branch-dev-5.15-int.patch \
                   file://0007-Bug-936-SW-superion-kernel-Add-SAPPHIRERAPIDS-fo.patch \
                   file://0008-Initial-Common-Add-common-hwmon-drivers.patch \
                   file://0009-Initial-Common-Add-virtual-driver-to-simulate-driver.patch \
                   file://0010-Subject-PATCH-Revise-ADT7462-driver-in-kernel.patch \
                   file://0011-Subject-PATCH-Kernel-dts-driver-Patched-hsc-setting.patch \
                   file://0012-Subject-Patch-kernel-driver-Set-max31790-default-ena.patch \
                   file://0013-Subject-Patch-kernel-RTC-Set-a-default-timestamp-for.patch \
                   file://0014-Modify-RGMII-TX-Clock-delay-and-TX-driving-strength.patch \
                   file://0015-Fix-KCS3-cannot-be-created.patch \
                   file://0016-Enable-mmcblk0-block.patch \
                   file://0017-add-aspeed-pwm-tachometer-driver-and-modify-the-dtsi.patch \
                   file://0018-force-spi-to-run-at-single-mode.patch \
                   file://0019-Implement-a-memory-driver-share-memory.patch \
                 "

do_add_overwrite_files () {
    cp -r "${WORKDIR}/arch" \
          "${STAGING_KERNEL_DIR}"
    cp -r "${WORKDIR}/drivers" \
          "${STAGING_KERNEL_DIR}"
}

addtask do_add_overwrite_files after do_patch before do_compile

