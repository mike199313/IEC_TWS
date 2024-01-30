LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit pkgconfig meson

SRC_URI = " file://include/provision.h;subdir=${S} \
            file://include/checkpoint.h;subdir=${S} \
            file://include/i2c_utils.h;subdir=${S} \
            file://include/status.h;subdir=${S} \
            file://include/info.h;subdir=${S} \
            file://include/mailbox_enums.h;subdir=${S} \
            file://include/arguments.h;subdir=${S} \
            file://include/config.h;subdir=${S} \
            file://provision.c;subdir=${S} \
            file://checkpoint.c;subdir=${S} \
            file://i2c_utils.c;subdir=${S} \
            file://status.c;subdir=${S} \
            file://info.c;subdir=${S} \
            file://main.c;subdir=${S} \
            file://meson.build;subdir=${S} \
            file://meson_options.txt;subdir=${S} \
            file://aspeed-pfr-tool.conf.in;subdir=${S} \
            file://BootCompleted.service;subdir=${S} \
          "

DEPENDS = "openssl i2c-tools"
RDEPENDS:${PN} = "openssl i2c-tools"

EXTRA_OEMESON:ast2600-pfr = " \
    -Di2c_bus=14 \
    -Dbmc_staging_offset=0x04a00000 \
    -Dbmc_recovery_offset=0x02a00000 \
    "

inherit obmc-phosphor-systemd
SYSTEMD_SERVICE:${PN} = "BootCompleted.service"

FILES:${PN}:append = " ${datadir}/pfrconfig"
