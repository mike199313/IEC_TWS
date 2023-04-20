#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>

#include <stdint.h>
#include <errno.h>

#include "inventec-aspeed-vw.hpp"


namespace InventecVgpio
{

std::string vwIoctl = "/dev/aspeed-espi-vw";
std::string configPath = "/usr/share/inventec-vgpio/inventec-vgpio.json";


enum class Direction
{
    IN,
    OUT
};



struct ConfigData
{
    std::string name;
    int index;
    Direction direction;
};


std::vector<ConfigData> vgpioConfigs;

int loadConfigValues()
{
    std::ifstream configFile(configPath.c_str());

    if (!configFile.is_open())
    {
        return -EIO;
    }
    auto jsonData = nlohmann::json::parse(configFile, nullptr, true, true);

    if (jsonData.is_discarded())
    {
        return -EINVAL;
    }
    auto vgpios = jsonData["vgpios"];

    ConfigData *tempData;

    for (nlohmann::json &vgpio : vgpios)
    {
        tempData = new (ConfigData);

        if (!vgpio.contains("Name"))
        {
            return -EINVAL;
        }
        tempData->name = vgpio["Name"];

        if (!vgpio.contains("Index"))
        {
            return -EINVAL;
        }
        tempData->index = vgpio["Index"].get<int>();

        if (!vgpio.contains("Direction"))
        {
            return -EINVAL;
        }
        if (vgpio["Direction"] == "in")
        {
            tempData->direction = Direction::IN;
        }
        else if (vgpio["Direction"] == "out")
        {
            tempData->direction = Direction::OUT;
        }
        else
        {
            return -EINVAL;
        }

        vgpioConfigs.push_back(*tempData);
    }
    return 0;
}



int readIO(uint32_t *value)
{
    int fd, ret;
    fd = open(vwIoctl.c_str(), O_RDWR);

    if (fd < 0)
    {
        return -EIO;
    }
    ret = ioctl(fd, ASPEED_ESPI_VW_GET_GPIO_VAL, value);

    close(fd);
    return ret;
}

int writeIO(uint32_t value)
{
    int fd, ret;

    fd = open(vwIoctl.c_str(), O_RDWR);

    if (fd < 0)
    {
        return -EIO;
    }

    ret = ioctl(fd, ASPEED_ESPI_VW_PUT_GPIO_VAL, &value);

    close(fd);
    return ret;
}

int findIndexByName(std::string name)
{
    int ret = -EINVAL;

    for (ConfigData vgpio : vgpioConfigs)
    {
        if (vgpio.name == name)
        {
            ret = vgpio.index;
        }
    }
    return ret;
}


int set_vgpio(std::string name, uint32_t value)
{
    int ret, index;
    uint32_t oldValue;

    ret = readIO(&oldValue);
    if (ret < 0)
    {
        return ret;
    }


    if (name.empty())
    {
        if (oldValue != value)
        {
            ret = writeIO(value);
        }
    }
    else
    {
        /* For specific pin, can only set 0 or 1*/
        if (value > 1)
        {
            return -EINVAL;
        }
        index = findIndexByName(name);
        if (index < 0)
        {
            return index;
        }

        if (value)
        {
            value = oldValue | (0x1 << index);
        }
        else
        {
            value = oldValue & ~(0x1 << index);
        }

        ret = writeIO(value);
    }
    return ret;
}

int get_vgpio(std::string name, uint32_t *value)
{
    int ret, index;

    ret = readIO(value);
    if (ret < 0)
    {
        return ret;
    }

    if (!name.empty())
    {
        index = findIndexByName(name);
        if (index < 0)
        {
            return index;
        }
        *value = (*value >> index) & 0x1;
    }

    return ret;
}
}






