FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " \
    clears-sel \
    log-threshold \
    log-pulse \
    log-watchdog \
    log-alarm"

SRC_URI:append = " \
    file://0001-Fix-IPMI-logging-service-fail-to-start-issue.patch \
    file://0002-Support-BMC-Global-Enables-Commands-to-control-SEL.patch \
    file://0003-Implement-PEF-features.patch \
    file://0004-Add-IpmiSelAddNoHook-DBus-method.patch \
    file://0005-Modified-sel-file-location-path-and-add-RecordID-pro.patch \
    file://0006-Add-SIGHUP-handler-to-check-clear_sel-to-restart-rec.patch \
    file://0007-Fix-SEL-log-ID-skip-number.patch \
"
