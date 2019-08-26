#ifndef INSTRUCTION_DATA_H_INCLUDED
#define INSTRUCTION_DATA_H_INCLUDED

#include <stdint.h>

union _instruction_data{
    uint16_t whole;
    uint8_t bytes[2];
    struct{
        union{
            uint8_t section34;
        };
        union{
            uint8_t section12;
        };
    };
    struct{
        union{
            struct{
                uint16_t section234:12;
            };
            struct{
                uint8_t section4:4,section3:4,section2:4,section1:4;
            };
        };
    };
};

typedef union _instruction_data instruction_data;

#endif // INSTRUCTION_DATA_H_INCLUDED
