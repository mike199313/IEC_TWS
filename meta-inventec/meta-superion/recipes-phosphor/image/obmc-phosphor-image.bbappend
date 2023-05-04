inherit extrausers

EXTRA_USERS_PARAMS:append = " \
useradd -e '' -ou 0 -d /home/root -G priv-admin,root,sudo,ipmi,web,redfish -p 'gzW59equAcJAg' sysadmin; \
useradd -e '' -ou 0 -d /home/root -G priv-admin,root,sudo,ipmi,web,redfish -p 'kFdHdjRkot8KQ' admin; \
"
OBMC_IMAGE_EXTRA_INSTALL:append = " openssh-sftp-server"
OBMC_IMAGE_EXTRA_INSTALL:append = " phosphor-ipmi-ipmb"
OBMC_IMAGE_EXTRA_INSTALL:append = " python3-smbus"
OBMC_IMAGE_EXTRA_INSTALL:append = " ipmitool"
#BMC_IMAGE_EXTRA_INSTALL:append = " rest-dbus"
OBMC_IMAGE_EXTRA_INSTALL:append = " mmc-utils"
OBMC_IMAGE_EXTRA_INSTALL:append = " superion-init"
OBMC_IMAGE_EXTRA_INSTALL:append = " inventec-util"
OBMC_IMAGE_EXTRA_INSTALL:append = " inventec-mac-config"
OBMC_IMAGE_EXTRA_INSTALL:append = " libpeci"
OBMC_IMAGE_EXTRA_INSTALL:append = " inventec-vgpio"
OBMC_IMAGE_EXTRA_INSTALL:append = " packagegroup-intel-apps"
OBMC_IMAGE_EXTRA_INSTALL:append = " libmctp-intel"
OBMC_IMAGE_EXTRA_INSTALL:append = " mctpd"
OBMC_IMAGE_EXTRA_INSTALL:append = " mctp-wrapper"
OBMC_IMAGE_EXTRA_INSTALL:append = " mctpwplus"

