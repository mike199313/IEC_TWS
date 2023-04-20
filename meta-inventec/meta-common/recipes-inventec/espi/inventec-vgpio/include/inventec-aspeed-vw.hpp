#pragma once
#include <stdint.h>


/* Refer from Aspeed kernel drivers/soc/aspeed/aspeed-espi-ioc.h */

#define __ASPEED_ESPI_IOCTL_MAGIC   0xb8
/*
 * Virtual Wire Channel (CH1)
 *  - ASPEED_ESPI_VW_GET_GPIO_VAL
 *      Read the input value of GPIO over the VW channel
 *  - ASPEED_ESPI_VW_PUT_GPIO_VAL
 *      Write the output value of GPIO over the VW channel
 */
#define ASPEED_ESPI_VW_GET_GPIO_VAL _IOR(__ASPEED_ESPI_IOCTL_MAGIC, \
                         0x10, uint8_t)
#define ASPEED_ESPI_VW_PUT_GPIO_VAL _IOW(__ASPEED_ESPI_IOCTL_MAGIC, \
                         0x11, uint8_t)



