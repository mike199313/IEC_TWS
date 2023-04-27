#include "inventec-vgpio.hpp"
#include <nlohmann/json.hpp>

#include <iostream>
#include <getopt.h>
#include <errno.h>

#define SUCCESS 0

enum class Operation
{
    SET,
    GET,
    NONE
};


int convertStringToInt(std::string input)
{
    std::string temp;
    int ret;
    /* Check is hex string or not*/
    if (input.length() > 2)
    {
        if ((input.at(1) == 'x') || (input.at(1) == 'X'))
        {
            temp = input.substr(2);
            ret = std::stoul(temp, nullptr, 16);
        }
        else
        {
            ret = std::stoul(input, nullptr, 10);
        }
    }
    else
    {
        ret = std::stoul(input, nullptr, 10);
    }

    return ret;
}

int print_help(void)
{
    printf("inventec-vgpio usage\n");
    printf("  -s <value> Set value\n");
    printf("  -g         Get value\n");
    printf("  -n <name>  Specific virtual pin\n");
    printf("\n");
    printf("example:\n");
    printf("    inventec-vgpin -g\n");
    printf("    0x00000000\n");
    printf("    inventec-vgpin -s 0x12345678\n");
    printf("    inventec-vgpin -g\n");
    printf("    0x12345678\n");
    printf("    inventec-vgpio -g -n TEST_PIN\n");
    printf("    0x1\n");
    printf("    inventec-vgpio -s 0x1 -n TEST_PIN\n");
    return 0;
}


struct option options[] =
{
    { .name = "help", .has_arg = no_argument, .val = 'h' },
    { .name = "set", .has_arg = required_argument, .val = 's' },
    { .name = "get", .has_arg = no_argument, .val = 'g' },
    { .name = "name", .has_arg = required_argument, .val = 'n' },
    { 0 },
};


int main(int argc, char **argv)
{
    int c, ret;
    uint32_t value;
    std::string targetName;
    std::string targetValue;
    Operation operation = Operation::NONE;

    for (;;)
    {
        c = getopt_long(argc, argv, "hs:gn:", options, NULL);
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            print_help();
            return SUCCESS;
        case 's':
            operation = Operation::SET;
            targetValue = optarg;
            break;
        case 'g':
            operation = Operation::GET;
            break;
        case 'n':
            targetName = optarg;
            ret = InventecVgpio::loadConfigValues();
            if (ret < 0)
            {
                return ret;
            }
            break;
        default:
            print_help();
            return 0;
        }
    }
    switch (operation)
    {
    case Operation::SET:
        if (!targetValue.length())
        {
            return -EIO;
        }
        ret = InventecVgpio::set_vgpio(targetName, convertStringToInt(targetValue));
        break;
    case Operation::GET:
        ret = InventecVgpio::get_vgpio(targetName, &value);
        if (ret == SUCCESS)
        {
            printf("0x%x\n", value);
        }
        break;
    default:
        break;
    }

    return ret;
}
