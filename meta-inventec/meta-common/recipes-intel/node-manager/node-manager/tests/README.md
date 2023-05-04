# Standalone Unit Tests for NodeManager
The standalone UT for NM are designed to be compiled as a separate
binary in native environment.


# Requirements
The recommended compiler version is gcc-10 - same as YOCTO uses.

Below dependencies are required but usually met on systems configured for building openbmc-openbmc:
```
sudo apt-get install autoconf autoconf-archive systemd libsystemd-dev libpth-dev libudev-dev cmake python3-pip ninja-build pkg-config
```

Additionaly, to build the newest `sdbusplus` there are some python dependencies required as below:
```
sudo python3 -m pip install meson inflection
```

# Compiler upgrade hints
Some helpful hints for Ubuntu:
0. Checking your compiler version
```
gcc -v
g++ -v
```
1. Installing gcc versions
```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update && sudo apt upgrade
sudo apt install gcc-11 g++-11
```
2. Adding to alternative compilers:
```
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
```
3. Selecting compiler:
```
sudo update-alternatives --config g++
```
and
```
sudo update-alternatives --config gcc
```

# Build
How to build:
```
cd tests
mkdir build
cd build
cmake ..
make
```

# Running tests
How to use:
```
cd tests/build
make test
```
or
```
./tests/build/nm_tests
```

# UT naming convention
Macro TEST_F uses the test class specified in first argument The second
argument is the test name. The test name should be clear and unique:
*What is called -> What happens -> Under what conditions*

Example:
```TEST_F(BalancerTest, SetComponentLimitForDeviceIdOutOfRangeThrowsAlways)```

