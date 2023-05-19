SUMMARY = "Aspeed Apps"
DESCRIPTION = "To install Aspeed applications."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

S = "${WORKDIR}/git"

SRC_URI = "git://github.com/AspeedTech-BMC/aspeed_app.git;branch=master;protocol=https"

# Tag for v00.01.08
SRCREV = "7245d73f53097e9b9ffc75d1fda88d18072974d9"

DEPENDS = "openssl"

FILES:${PN}:append = " /usr/share/* "
