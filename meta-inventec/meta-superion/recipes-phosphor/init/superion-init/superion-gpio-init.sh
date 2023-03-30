#!/bin/sh

# GPIO Init
gpioset `gpiofind BMC_READY`=0
echo BMC ready !!
gpioset `gpiofind RST_BMC_SGPIO`=1
echo Release reset SGPIO !!

#Connect cpu to bios spi rom as the spi mux default setting
gpioset `gpiofind SPI_BIOS_MUXA_SEL`=0
gpioset `gpiofind SPI_BIOS_MUXB_SEL`=0
