FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0007-inventec-common-Modify-power-capability-for-DCMI.patch \
            file://0011-Add-and-modify-new-interface-related-to-system-boot-.patch \
            file://0013-Add-new-interface-related-to-bmc-global-enables-sett.patch \
            file://0024-Add-PLDM-version-purpose-enumeration.patch \
            file://0028-MCTP-Daemon-D-Bus-interface-definition.patch \
            file://0032-update-meson-build-for-MCTP-interfaces.patch \
            "
