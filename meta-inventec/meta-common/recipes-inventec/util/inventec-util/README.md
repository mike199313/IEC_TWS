## Inventec utilities
### Mac utility 
MAC address (eth address) read
```
/usr/bin/mac_util r $Name
/usr/bin/mac_util r eth0
```
MAC address (eth address) write
```
/usr/bin/mac_util w $Name $MAC_ADDRESS
/usr/bin/mac_util w eth0 02:00:ff:00:00:01
```
<br>
The i2c parameters defined in include/mac_util.hpp.<br>
```
IntfInfo intfInfoList[] = {
    {"eth0", 8, 0x51, 0x400}
};
```
For different platform, should add bbappend to change the parameters.<br>

