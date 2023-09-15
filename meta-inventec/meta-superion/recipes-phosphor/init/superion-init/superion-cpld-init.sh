#get cpld mb & scm version
CPLD_VERSION_PATH="/usr/share/version"
CPLD_MB_BUS=11
CPLD_MB_ADDR=0x37
CPLD_SCM_BUS=9
CPLD_SCM_ADDR=0x37

CPLD_VERSION_REG=0x2

detect_cpld_main()
{
    mkdir -p $CPLD_VERSION_PATH
    echo "CPLD_MB_BUS: $CPLD_MB_BUS CPLD_MB_ADDR: $CPLD_MB_ADDR"
    echo "CPLD_SCM_BUS: $CPLD_SCM_BUS CPLD_SCM_ADDR: $CPLD_SCM_ADDR"
    mb_version_hex=""
    mb_version_dec=""
    mb_version_hex=$(i2cget -y -f $CPLD_MB_BUS $CPLD_MB_ADDR $CPLD_VERSION_REG)
    mb_version_dec=`echo $((${mb_version_hex}))`  # convert version value from heximal to decimal
    echo $(printf "%02d" $mb_version_dec) > $CPLD_VERSION_PATH/MB_CPLD_Version  #fill 0 ahead
    echo "MB CPLD Verion: $(printf "%02d" $mb_version_dec)"

    scm_version_hex=""
    scm_version_dec=""
    scm_version_hex=$(i2cget -y -f $CPLD_SCM_BUS $CPLD_SCM_ADDR $CPLD_VERSION_REG)
    scm_version_dec=`echo $((${scm_version_hex}))`  # convert version value from heximal to decimal
    echo $(printf "%02d" $scm_version_dec) > $CPLD_VERSION_PATH/SCM_CPLD_Version  #fill 0 ahead
    echo "SCM CPLD Verion: $(printf "%02d" $scm_version_dec)"

}

detect_cpld_main

