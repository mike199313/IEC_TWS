FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

inherit pkgconfig


SRC_URI:append  = " file://0001-changes-to-xyz.patch \
		    file://0002-Moidfy-Intel-Get-Device-ID-for-Firmware-Revision-1-a.patch \
                  "

