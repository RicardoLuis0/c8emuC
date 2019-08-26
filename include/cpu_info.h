#ifndef CPU_INFO_H_INCLUDED
#define CPU_INFO_H_INCLUDED

#include <stdint.h>

struct CPU_info {
    uint8_t RAM[4096];
    uint8_t VRAM[2048];
    uint8_t KB[16];
    uint8_t DT;
    uint8_t ST;
    uint16_t I;
    uint16_t PC;
    uint16_t stack[24];
    uint16_t SP;
    union{
        uint8_t V[16];
        struct{
            uint8_t V0;
            uint8_t V1;
            uint8_t V2;
            uint8_t V3;
            uint8_t V4;
            uint8_t V5;
            uint8_t V6;
            uint8_t V7;
            uint8_t V8;
            uint8_t V9;
            uint8_t VA;
            uint8_t VB;
            uint8_t VC;
            uint8_t VD;
            uint8_t VE;
            uint8_t VF;
        };
    };
};

#endif // CPU_INFO_H_INCLUDED
