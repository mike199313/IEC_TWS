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
OBMC_IMAGE_EXTRA_INSTALL:append = " intel-ipmi-oem"
OBMC_IMAGE_EXTRA_INSTALL:append = " aspeed-app"
OBMC_IMAGE_EXTRA_INSTALL:append = " pfr-manager"
OBMC_IMAGE_EXTRA_INSTALL:append = " intel-asd"
OBMC_IMAGE_EXTRA_INSTALL:append = " post-complete"

# The offset of RWFS is smaller than ROFS, so creates do_generate_static task
# to overwrite the default setting which is from image_types_phosphor.bbclass
python do_generate_static() {
    import subprocess

    bb.build.exec_func("do_mk_static_nor_image", d)

    nor_image = os.path.join(d.getVar('IMGDEPLOYDIR', True),
                             '%s.static.mtd' % d.getVar('IMAGE_NAME', True))

    def _append_image(imgpath, start_kb, finish_kb):
        imgsize = os.path.getsize(imgpath)
        maxsize = (finish_kb - start_kb) * 1024
        bb.debug(1, 'Considering file size=' + str(imgsize) + ' name=' + imgpath)
        bb.debug(1, 'Spanning start=' + str(start_kb) + 'K end=' + str(finish_kb) + 'K')
        bb.debug(1, 'Compare needed=' + str(imgsize) + ' available=' + str(maxsize) + ' margin=' + str(maxsize - imgsize))
        if imgsize > maxsize:
            bb.fatal("Image '%s' is too large!" % imgpath)

        subprocess.check_call(['dd', 'bs=1k', 'conv=notrunc',
                               'seek=%d' % start_kb,
                               'if=%s' % imgpath,
                               'of=%s' % nor_image])

    uboot_offset = int(d.getVar('FLASH_UBOOT_OFFSET', True))

    spl_binary = d.getVar('SPL_BINARY', True)
    if spl_binary:
        _append_image(os.path.join(d.getVar('DEPLOY_DIR_IMAGE', True),
                                   'u-boot-spl.%s' % d.getVar('UBOOT_SUFFIX',True)),
                      int(d.getVar('FLASH_UBOOT_OFFSET', True)),
                      int(d.getVar('FLASH_UBOOT_SPL_SIZE', True)))
        uboot_offset += int(d.getVar('FLASH_UBOOT_SPL_SIZE', True))

    _append_image(os.path.join(d.getVar('DEPLOY_DIR_IMAGE', True),
                               'u-boot.%s' % d.getVar('UBOOT_SUFFIX',True)),
                  uboot_offset,
                  int(d.getVar('FLASH_RWFS_OFFSET', True)))

    _append_image(os.path.join(d.getVar('IMGDEPLOYDIR', True),
                               '%s.%s' % (
                                    d.getVar('IMAGE_LINK_NAME', True),
                                    d.getVar('OVERLAY_BASETYPE', True))),
                  int(d.getVar('FLASH_RWFS_OFFSET', True)),
                  int(d.getVar('FLASH_KERNEL_OFFSET', True)))

    _append_image(os.path.join(d.getVar('DEPLOY_DIR_IMAGE', True),
                               d.getVar('FLASH_KERNEL_IMAGE', True)),
                  int(d.getVar('FLASH_KERNEL_OFFSET', True)),
                  int(d.getVar('FLASH_ROFS_OFFSET', True)))

    _append_image(os.path.join(d.getVar('IMGDEPLOYDIR', True),
                               '%s.%s' % (
                                    d.getVar('IMAGE_LINK_NAME', True),
                                    d.getVar('IMAGE_BASETYPE', True))),
                  int(d.getVar('FLASH_ROFS_OFFSET', True)),
                  int(d.getVar('FLASH_SIZE', True)))

    bb.build.exec_func("do_mk_static_symlinks", d)
}


PFR_IMAGE_MODE = "${@bb.utils.contains('MACHINE_FEATURES', 'cerberus-pfr', 'cerberus-pfr-signing-image', 'intel-pfr-signing-image', d)}"
inherit ${PFR_IMAGE_MODE}
