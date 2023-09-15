FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DEPENDS += " \
        nlohmann-json \
        i2c-tools \
"



SRC_URI += " \
            file://0001-add-bios-image-upload-flow.patch \
            file://0002-Fix-BIOS-version-is-null-issue.patch \
            file://0003-MS-M-LOG-0008.patch\
            file://0004-Fix-Updateable-parameter-displays-false-issue.patch \
            file://0005-add-cpld-mb-and-scm-dbus-object-and-version.patch \
            file://0006-MS-spec-BIOS-securely-transfer.patch \
            file://0007-PLDM-BMC-software-updater-integration.patch \
            file://0008-PLDM-firmware-update-with-specified-PID-and-interface.patch \
           "

#add cpld and psu service
SYSTEMD_SERVICE:${PN}-updater += " \
   obmc-cpld-update@.service \
"


PACKAGECONFIG[cpld_mb] = "-Dcpld-mb-upgrade=enabled, -Dcpld-mb-upgrade=disabled"
PACKAGECONFIG[cpld_scm] = "-Dcpld-scm-upgrade=enabled, -Dcpld-scm-upgrade=disabled"


#enable host-bios-update cpld_mb cpld_scm feature
PACKAGECONFIG:append =" \
            flash_bios \
            cpld_mb \
            cpld_scm \
            "
