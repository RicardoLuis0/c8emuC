#ifndef DISASSEMBLE_H_INCLUDED
#define DISASSEMBLE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include "instruction_data.h"
#include "instructions.h"

typedef struct _disassembled_instruction{
    instruction_t type;
    instruction_data data;
} disassembled_instruction;

disassembled_instruction disassemble_instruction(instruction_data inst);
void get_instruction_str(disassembled_instruction inst,char * into,size_t size);

#endif // DISASSEMBLE_H_INCLUDED
