FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-AverageSensor-averagesensor-initial-commit.patch \
            file://0002-InvCfmSensor-Initial-Inventec-CFM-sensor.patch \
            file://0003-AccumulateSensor-initial-commit.patch \
            file://0004-IiohwmonSensor-add-the-iio_hwmon-sensor-initial-and-.patch \
           "

PACKAGECONFIG:append =" \
            averagesensor \
            invcfmsensor \
            accumulatesensor \
            iiohwmonsensor \
            "

PACKAGECONFIG:remove ="mcutempsensor intrusionsensor"

PACKAGECONFIG[averagesensor] = "-Daverage=enabled, -Daverage=disabled"
PACKAGECONFIG[invcfmsensor] = "-Dinvcfm=enabled, -Dinvcfm=disabled"
PACKAGECONFIG[accumulatesensor] = "-Daccumulate=enabled, -Daccumulate=disabled"
PACKAGECONFIG[iiohwmonsensor] = "-Daccumulate=enabled, -Daccumulate=disabled"



SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'averagesensor', \
                                               'xyz.openbmc_project.averagesensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'invcfmsensor', \
                                               'xyz.openbmc_project.invcfmsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'accumulatesensor', \
                                               'xyz.openbmc_project.accumulatesensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'external', \
                                               'xyz.openbmc_project.iiohwmonsensor.service', \
                                               '', d)}"
