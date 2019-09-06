#ifndef DISASSEMBLE_H_INCLUDED
#define DISASSEMBLE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include "instruction_data.h"

typedef struct _disassembled_instruction{
    enum{
        DATA,
        SYS,
        CLS,
        RET,
        JMP,
        JMP_V0,
        CALL,
        SE,
        SE_REG,
        SNE,
        SNE_REG,
        SKP,
        SNKP,
        KEY,
        MOV,
        MOV_REG,
        MOV_I,
        MOV_REG_DT,
        MOV_DT,
        MOV_ST,
        ADD,
        ADD_REG,
        ADD_I,
        SUB,
        RSB,
        OR,
        AND,
        XOR,
        SHR,
        SHL,
        RND,
        DRW,
        LDFNT,
        BCD,
        STR,
        LDR,
    }type;
    instruction_data data;
} disassembled_instruction;

disassembled_instruction disassemble_instruction(uint16_t rawbytes);
void print_instruction(disassembled_instruction inst,char * into,size_t size);

#endif // DISASSEMBLE_H_INCLUDED
