#!/bin/bash

# espi vGPIO FM_BIOS_POST_CMPLT_N wil pull low after BIOS on.
# Rebind the peci driver after FM_BIOS_POST_CMPLT_N pull low to probe successfully.
VALUE_HIGH=0x1
VALUE_LOW=0x0
FIRST_PULSE_MAX_LOOP_TIME=60 # unit: second
SECOND_PULSE_MAX_LOOP_TIME=300 # unit: second
vGPIO_INIT_DIRECTION=0xf
vGPIO_BASE_ADDR=0x1E6EE0C0
ERROR_OK=0
ERROR_NG=-1

function post_complete_rebind() {
    # rebind peci driver
    echo Rebind peci driver
    echo 0-30 > /sys/bus/peci/drivers/intel_peci_client/unbind
    echo 0-30 > /sys/bus/peci/drivers/intel_peci_client/bind
    systemctl restart xyz.openbmc_project.cpusensor.service
}

function show_log() {
    # parameter($1): process status
    case "$1" in
        get_high_level)
            echo Pulse detecting timeout. Can not get high level of palse.
            ;;
        get_low_level)
            echo Pulse detecting timeout. Can not get falling part of pulse.
            ;;
        detecting_success)
            echo Pulse detecting success.
            ;;
    esac
}

function detect_pulse() {
    # parameter($1): max time of loop
    ret=$ERROR_NG
    MAX_CNT=$1
    process="get_high_level"

    # detect the falling part of pulse
    for ((i=1;i<=$MAX_CNT;i++))
    do
        GET_POST_CMPLT=`inventec-vgpio -g -n FM_BIOS_POST_CMPLT_N`
        case "$process" in
            get_high_level)
                if [ $GET_POST_CMPLT == $VALUE_HIGH ]; then
                    process="get_low_level"
                fi
                ;;
            get_low_level)
                if [ $GET_POST_CMPLT == $VALUE_LOW ]; then
                    process="detecting_success"
                fi
                ;;
            detecting_success)
                ret=$ERROR_OK
                break
                ;;
        esac
        sleep 1
    done

    show_log "$process"

    return $ret
}

# first vgpio direction init is in order to find the BIOS reset point
devmem $vGPIO_BASE_ADDR w $vGPIO_INIT_DIRECTION

echo "Detecting BIOS reset..."
detect_pulse "$FIRST_PULSE_MAX_LOOP_TIME"

if [ $? == $ERROR_OK ]; then
    echo "Init vgpio direction"
    devmem $vGPIO_BASE_ADDR w $vGPIO_INIT_DIRECTION

    # find the actual time when the BIOS turn on
    detect_pulse "$SECOND_PULSE_MAX_LOOP_TIME"

    if [ $? == $ERROR_OK ]; then
        post_complete_rebind
    else
        echo Second pulse detecting failed, please check the host status.
    fi
else
    echo First pulse detecting failed, please check the host status.
fi
