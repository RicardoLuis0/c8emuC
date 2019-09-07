#include "disassemble.h"
#include <stdio.h>

disassembled_instruction disassemble_instruction(uint16_t rawbytes){
    instruction_data inst={
        .whole=rawbytes,
    };
    switch(inst.section1){
    case 0x0:
        switch(inst.section234){
        case 0x0E0: //CLS
            //clear screen
            return (disassembled_instruction){CLS,inst};
        case 0x0EE: //RET
            //return from procedure
            return (disassembled_instruction){RET,inst};
        default: //SYS NNN
            //syscall/data
            return (disassembled_instruction){DATA,inst};
        }
    case 0x1: //JMP NNN
        //jump
        return (disassembled_instruction){JMP,inst};
    case 0x2: //CALL NNN
        //call procedure
        return (disassembled_instruction){CALL,inst};
    case 0x3: //SE VX,NN
        //skip if equal
        return (disassembled_instruction){SE,inst};
    case 0x4: //SNE VX,NN
        //skip if not equal
        return (disassembled_instruction){SNE,inst};
    case 0x5:
        if(inst.section4==0x0){//SE VX,VY
            //skip if registers equal
            return (disassembled_instruction){SE_REG,inst};
        }else{
            //INVALID_OPERATION
            return (disassembled_instruction){DATA,inst};//return as data-only line
        }
    case 0x6: //MOV VX,NN
        //move value into register
        return (disassembled_instruction){MOV,inst};
    case 0x7: //ADD VX,NN
        //add value to register
        return (disassembled_instruction){ADD,inst};
    case 0x8:
        switch(inst.section4){
        case 0x0: //MOV VX,VY
            //move register value into another register
            return (disassembled_instruction){MOV_REG,inst};
        case 0x1: //OR VX,VY
            //bitwise or between 2 registers
            return (disassembled_instruction){OR,inst};
        case 0x2: //AND VX,VY
            //bitwise and between 2 registers
            return (disassembled_instruction){AND,inst};
        case 0x3: //XOR VX,VY
            //bitwise xor between 2 registers
            return (disassembled_instruction){XOR,inst};
        case 0x4: //ADD VX,VY
            //add register to another
            return (disassembled_instruction){ADD_REG,inst};
        case 0x5: //SUB VX,VY
            //subtract register from another
            return (disassembled_instruction){SUB,inst};
        case 0x6: //SHR VX,VY
            //bitwise right shift
            return (disassembled_instruction){SHR,inst};
        case 0x7: //RSB VX,VY
            //reverse subtract
            return (disassembled_instruction){RSB,inst};
        case 0xE: //SHL VX,VY
            //bitwise left shift
            return (disassembled_instruction){SHL,inst};
        default:
            //INVALID_OPERATION
            return (disassembled_instruction){DATA,inst};
        }
    case 0x9: //SNE VX,VY
        if(inst.section4==0x0){
            //skip if registers not equal
            return (disassembled_instruction){SNE_REG,inst};
        }else{
            //INVALID_OPERATION
            return (disassembled_instruction){DATA,inst};
        }
        break;
    case 0xA: //MOV I,NNN
        //move value into I
        return (disassembled_instruction){MOV_I,inst};
    case 0xB: //JMP V0,NN
        //jump to V0+value
        return (disassembled_instruction){JMP_V0,inst};
    case 0xC: //RND VX,NN
        //masked random value
        return (disassembled_instruction){RND,inst};
    case 0xD: //DRW VX,VY,N
        //draw sprite
        return (disassembled_instruction){DRW,inst};
    case 0xE:
        switch(inst.section34){
        case 0x9E: //SKP VX
            //skip if key in register is pressed
            return (disassembled_instruction){SKP,inst};
        case 0xA1: //SNKP VX
            //skip if key in register is not pressed
            return (disassembled_instruction){SNKP,inst};
        default:
            //INVALID_OPERATION
            return (disassembled_instruction){DATA,inst};
        }
    case 0xF:
        switch(inst.section34){
        case 0x07: //MOV VX,DT
            //move DT into register
            return (disassembled_instruction){MOV_REG_DT,inst};
        case 0x0A: //KEY VX
            //wait for key, store it in register
            return (disassembled_instruction){KEY,inst};
        case 0x15: //MOV DT,VX
            //move register into DT
            return (disassembled_instruction){MOV_DT,inst};
        case 0x18: //MOV ST,VX
            //move register into ST
            return (disassembled_instruction){MOV_ST,inst};
        case 0x1E: //ADD I,VX
            //add VX to I
            return (disassembled_instruction){ADD_I,inst};
        case 0x29: //LDFNT VX
            //load font location representing register into I
            return (disassembled_instruction){LDFNT,inst};
        case 0x33: //BCD VX
            //store binary coded decimal representation of register into RAM
            return (disassembled_instruction){BCD,inst};
        case 0x55: //STR VX
            //store values from register V0 to VX to RAM
            return (disassembled_instruction){STR,inst};
        case 0x65: //LDR VX
            //load values to register V0 to VX from RAM
            return (disassembled_instruction){LDR,inst};
        default:
            //INVALID_OPERATION
            return (disassembled_instruction){DATA,inst};
        }
    }
}

void get_instruction_str(disassembled_instruction inst,char * into,size_t size){
    switch(inst.type){
    /*
    case SYS: //SYS NNN
        snprintf(into,size,"(0x%04X) SYS(NOP) 0x%03X\n",inst.data.whole,inst.data.section234);
        break;
    */
    case CLS: //CLS
        snprintf(into,size,"(0x%04X) CLS\n",inst.data.whole);
        break;
    case RET: //RET
        snprintf(into,size,"(0x%04X) RET\n",inst.data.whole);
        break;
    case JMP: //JMP NNN
        snprintf(into,size,"(0x%04X) JMP 0x%03X\n",inst.data.whole,inst.data.section234);
        break;
    case JMP_V0: //JMP V0,NNN
        snprintf(into,size,"(0x%04X) JMP V0, 0x%03X\n",inst.data.whole,inst.data.section234);
        break;
    case CALL: //CALL NNN
        snprintf(into,size,"(0x%04X) CALL 0x%03X\n",inst.data.whole,inst.data.section234);
        break;
    case SE: //SE VX,NN
        snprintf(into,size,"(0x%04X) SE V%01X, 0x%02X\n",inst.data.whole,inst.data.section2,inst.data.section34);
        break;
    case SE_REG: //SE VX,VY
        snprintf(into,size,"(0x%04X) SE V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case SNE: //SNE VX,NN
        snprintf(into,size,"(0x%04X) SNE V%01X, 0x%02X\n",inst.data.whole,inst.data.section2,inst.data.section34);
        break;
    case SNE_REG: //SNE VX,VY
        snprintf(into,size,"(0x%04X) SNE V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case MOV: //MOV VX,NN
        snprintf(into,size,"(0x%04X) MOV V%01X, 0x%02X\n",inst.data.whole,inst.data.section2,inst.data.section34);
        break;
    case MOV_REG: //MOV VX,VY
        snprintf(into,size,"(0x%04X) MOV V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case MOV_I: //MOV I,NNN
        snprintf(into,size,"(0x%04X) MOV I, 0x%03X\n",inst.data.whole,inst.data.section234);
        break;
    case MOV_REG_DT: //MOV VX,DT
        snprintf(into,size,"(0x%04X) MOV V%01X, DT\n",inst.data.whole,inst.data.section2);
        break;
    case MOV_DT: //MOV DT,VX
        snprintf(into,size,"(0x%04X) MOV DT, V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case MOV_ST: //MOV ST,VX
        snprintf(into,size,"(0x%04X) MOV ST, V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case ADD: //ADD VX,NN
        snprintf(into,size,"(0x%04X) ADD V%01X, 0x%02X\n",inst.data.whole,inst.data.section2,inst.data.section34);
        break;
    case ADD_REG: //ADD VX,VY
        snprintf(into,size,"(0x%04X) ADD V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case ADD_I: //ADD I,VX
        snprintf(into,size,"(0x%04X) ADD I, V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case OR: //OR VX,VY
        snprintf(into,size,"(0x%04X) OR V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case AND: //AND VX,VY
        snprintf(into,size,"(0x%04X) AND V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case XOR: //XOR VX,VY
        snprintf(into,size,"(0x%04X) XOR V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case SUB: //SUB VX,VY
        snprintf(into,size,"(0x%04X) SUB V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case SHR: //SHR VX,VY
        snprintf(into,size,"(0x%04X) SHR V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case RSB: //RSB VX,VY
        snprintf(into,size,"(0x%04X) RSB V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case SHL: //SHL VX,VY
        snprintf(into,size,"(0x%04X) SHL V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case RND: //RND VX,NN
        snprintf(into,size,"(0x%04X) RND V%01X, V%01X\n",inst.data.whole,inst.data.section2,inst.data.section3);
        break;
    case DRW: //DRW VX,VY,N
        snprintf(into,size,"(0x%04X) DRW V%01X, V%01X, 0x%01X\n",inst.data.whole,inst.data.section2,inst.data.section3,inst.data.section4);
        break;
    case SKP: //SKP VX
        snprintf(into,size,"(0x%04X) SKP V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case SNKP: //SNKP VX
        snprintf(into,size,"(0x%04X) SNKP V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case KEY: //KEY VX
        snprintf(into,size,"(0x%04X) KEY V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case LDFNT: //LDFNT VX
        snprintf(into,size,"(0x%04X) LDFNT V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case BCD: //BCD VX
        snprintf(into,size,"(0x%04X) BCD V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case STR: //STR VX
        snprintf(into,size,"(0x%04X) STR V%01X\n",inst.data.whole,inst.data.section2);
        break;
    case LDR: //LDR VX
        snprintf(into,size,"(0x%04X) LDR V%01X\n",inst.data.whole,inst.data.section2);
        break;
    default:
    case DATA:
        snprintf(into,size,"(0x%04X) DATA 0x%04X\n",inst.data.whole,inst.data.whole);
        break;
    }
}
