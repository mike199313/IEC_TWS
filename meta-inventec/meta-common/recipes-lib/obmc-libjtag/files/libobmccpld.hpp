#pragma once

#include <stdint.h>

enum cpld_part_list : uint8_t
{
    LATTICE = 0x00,
};

enum lattice_op_cmd : uint8_t
{
    lattice_read_usercode = 0xC0,
};

int getCpldUserCode(uint8_t part, uint32_t* buffer);
