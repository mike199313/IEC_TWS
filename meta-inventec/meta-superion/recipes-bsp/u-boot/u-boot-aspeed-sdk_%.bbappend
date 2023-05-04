FILESEXTRAPATHS:append := "${THISDIR}/${BPN}:"


SRCREV = "7297634df83bfe1b3bdadb82175d36887bdb34c9"



SRC_URI:append = " file://superion-ast2600.cfg \
                   file://superion-ast2600_defconfig \
                   file://ast2600-superion.dts \
                   file://0001-Add-U-boot-tree-for-superion.patch \
                   file://0002-superion-machine-init.patch \
                 "

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

do_deploy:append() {
    mv ${UBOOT_IMAGE} uboot-tmp.bin
    filesize=$(stat -c %s "uboot-tmp.bin")
    checksum=`md5sum uboot-tmp.bin | awk '{ print $1 }'`
    echo "INVENTEC_UBOOT_SIZE_${filesize}_CHECKSUM_${checksum}" > uboot-checksum
    cat uboot-tmp.bin uboot-checksum >> ${UBOOT_IMAGE}
}

addtask copyfile after do_patch before do_configure

include conf/machine/platform_configs.inc

EEPROM_MAC_I2C_BUS = "8"
EEPROM_MAC_I2C_ADDRESS = "0x51"
EEPROM_MAC_OFFSET = "0x400"
EEPROM_MAC_I2C_DEV_SPEED = "100000"
EEPROM_MAC_I2C_ADDR_LEN = "2"
EEPROM_ETH0_ADDR = "0x1e690000"

do_patch_headerfile () {
  cat >${S}/include/configs/IECplatformConfigs.h <<EOF
// This header file is automatically created, DO NOT EDIT IT.
#ifndef __IEC_PLATFORM_CONFIGS_H__
#define __IEC_PLATFORM_CONFIGS_H__

#define EEPROM_MAC_I2C_BUS (${EEPROM_MAC_I2C_BUS})
#define EEPROM_MAC_I2C_ADDRESS (${EEPROM_MAC_I2C_ADDRESS})
#define EEPROM_MAC_OFFSET (${EEPROM_MAC_OFFSET})
#define EEPROM_MAC_I2C_DEV_SPEED (${EEPROM_MAC_I2C_DEV_SPEED})
#define EEPROM_MAC_I2C_ADDR_LEN (${EEPROM_MAC_I2C_ADDR_LEN})
#define EEPROM_ETH0_ADDR (${EEPROM_ETH0_ADDR})

#endif /* __IEC_PLATFORM_CONFIGS_H__ */
EOF
}

addtask patch_headerfile after do_patch before do_configure
