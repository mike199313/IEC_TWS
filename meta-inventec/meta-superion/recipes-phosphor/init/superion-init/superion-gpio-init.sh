#!/bin/sh

# GPIO Init
gpioset `gpiofind BMC_READY`=0
echo BMC ready !!
gpioset `gpiofind RST_BMC_SGPIO`=1
echo Release reset SGPIO !!

# Connect cpu to bios spi rom as the spi mux default setting
gpioset `gpiofind SPI_BIOS_MUXA_SEL`=0
gpioset `gpiofind SPI_BIOS_MUXB_SEL`=0

# x86-power-control gpio (default for HOST ON)
gpioset `gpiofind RST_BTN_BMC_OUT`=1
gpioset `gpiofind BMC_PWR_BTN_OUT`=1
# gpioget `gpiofind PGD_SYS_PWROK` intput

# JTAG mux control (default for CPLD to BMC)
gpioset `gpiofind CPLD_JTAG_OE_R_N`=0

# I3C mux control (default for CPLD to HOST)
gpioset `gpiofind I3C_SPDMUX_SELECT1`=0
gpioset `gpiofind I3C_SPDMUX_SELECT0`=0
gpioset `gpiofind FM_SPD_SWITCH_CTRL`=0

