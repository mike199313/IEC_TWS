FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append  = " file://0001-Common-Add-RequestedPowerIntervalMs-property.patch \
                    file://0002-Common-Add-host-power-off-and-host-power-on-hook-ser.patch \
                    file://0003-Bug623-SW-Transformers-OpenBMC-IPMI-Implement-get-ch.patch \
                    file://0004-Bug-396-Transformers-OpenBMC-Redfish-Should-be-chass.patch \
                    file://0005-Clear-last-power-evnent-after-power-on.patch \
                    file://0006-Change-to-lg2-according-to-official-commit-c46ebb.patch \
                    file://host-power-off.service \
                    file://host-power-on.service \
                    file://host-power-off.target \
                    file://host-power-on.target \
                  "

SYSTEMD_SERVICE:${PN} += "host-power-off.service \
                          host-power-on.service \
                          host-power-off.target \
                          host-power-on.target"
