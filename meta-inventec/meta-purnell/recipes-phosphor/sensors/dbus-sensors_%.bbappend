FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI += "\
            file://0001-DBus-Sensor-Update-XDPE152C4-relative-Sysfs-Attribut.patch \
            file://0002-Dbus-Sensor-Add-Support-Ina220-Driver-Config.patch \
           "
