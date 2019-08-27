#include "cpu.h"
#include "cpu_info.h"
#include "fontset.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "instruction_data.h"

static void load_fonts(CPU_info * cpu){
    for(int i=0;i<16;i++){
        for(int j=0;j<5;j++){
            cpu->RAM[i*5+j]=fontset[i][j];
        }
    }
}

CPU_info * init_cpu(CPU_info * cpu){
    memset(cpu,0,sizeof(CPU_info));
    load_fonts(cpu);
    cpu->PC=0x200;
    cpu->wait_key=-1;
    return cpu;
}

CPU_info * new_cpu(){
    return init_cpu(malloc(sizeof(CPU_info)));
}

int execute_instruction(CPU_info * cpu){
    if(cpu->wait_key>=0)return 0;
    if(cpu->DT)cpu->DT--;
    if(cpu->ST){
        printf("\a");//TODO replace with sdl sound stuff
        cpu->ST--;
    }
    instruction_data inst={
        .section12=cpu->RAM[cpu->PC],
        .section34=cpu->RAM[cpu->PC+1],
    };
    cpu->PC+=2;
    switch(inst.section1){
    case 0x0:
        switch(inst.section234){
        case 0x0E0://CLS
            //clear screen
            memset(cpu->VRAM,0,sizeof(cpu->VRAM));
            break;
        case 0x0EE://RET
            //return from procedure
            cpu->PC=cpu->stack[--(cpu->SP)];
            break;
        default://SYS NNN
            //syscall
            //do nothing
            break;
        }
        break;
    case 0x1://JMP NNN
        //jump
        cpu->PC=inst.section234;
        break;
    case 0x2://CALL NNN
        //call procedure
        cpu->stack[(cpu->SP)++]=cpu->PC;
        cpu->PC=inst.section234;
        break;
    case 0x3://SE VX,NN
        //skip if equal
        if(cpu->V[inst.section2]==inst.section34)cpu->PC+=2;
        break;
    case 0x4://SNE VX,NN
        //skip if not equal
        if(cpu->V[inst.section2]!=inst.section34)cpu->PC+=2;
        break;
    case 0x5:
        if(inst.section4==0x0){//SE VX,VY
            //skip if registers equal
            if(cpu->V[inst.section2]==cpu->V[inst.section3])cpu->PC+=2;
        }else{
            //INVALID_OPERATION
            return 0;
        }
        break;
    case 0x6://MOV VX,NN
        //move value into register
        cpu->V[inst.section2]=inst.section34;
        break;
    case 0x7://ADD VX,NN
        //add value to register
        cpu->V[inst.section2]+=inst.section34;
        break;
    case 0x8:
        switch(inst.section4){
        case 0x0://MOV VX,VY
            //move register value into another register
            cpu->V[inst.section2]=cpu->V[inst.section3];
            break;
        case 0x1://OR VX,VY
            //bitwise or between 2 registers
            cpu->V[inst.section2]|=cpu->V[inst.section3];
            break;
        case 0x2://AND VX,VY
            //bitwise and between 2 registers
            cpu->V[inst.section2]&=cpu->V[inst.section3];
            break;
        case 0x3://XOR VX,VY
            //bitwise xor between 2 registers
            cpu->V[inst.section2]^=cpu->V[inst.section3];
            break;
        case 0x4://ADD VX,VY
            //add register to another
            cpu->VF=(cpu->V[inst.section2]+cpu->V[inst.section3]>0xFF);
            cpu->V[inst.section2]+=cpu->V[inst.section3];
            break;
        case 0x5://SUB VX,VY
            //subtract register from another
            cpu->VF=(cpu->V[inst.section2]-cpu->V[inst.section3]>0x0);
            cpu->V[inst.section2]-=cpu->V[inst.section3];
            break;
        case 0x6://SHR VX,VY
            //bitwise right shift
            cpu->VF=cpu->V[inst.section2]&1;
            cpu->V[inst.section2]>>=1;
            break;
        case 0x7://RSB VX,VY
            //reverse subtract
            cpu->VF=(cpu->V[inst.section3]-cpu->V[inst.section2]>0x0);
            cpu->V[inst.section2]=cpu->V[inst.section3]-cpu->V[inst.section2];
            break;
        case 0xE://SHL VX,VY
            //bitwise left shift
            cpu->VF=(cpu->V[inst.section2]>>7)&1;
            cpu->V[inst.section2]<<=1;
            break;
        default:
            //INVALID_OPERATION
            return 0;
        }
        break;
    case 0x9://SNE VX,VY
        if(inst.section4==0x0){
            //skip if registers not equal
            if(cpu->V[inst.section2]!=cpu->V[inst.section3])cpu->PC+=2;
        }else{
            //INVALID_OPERATION
            return 0;
        }
        break;
    case 0xA://MOV I,NNN
        //move value into I
        cpu->I=inst.section234;
        break;
    case 0xB://JMP V0,NNN
        //jump to V0+value
        cpu->PC=cpu->V0+inst.section234;
        break;
    case 0xC://RND VX,NN
        //masked random value
        cpu->V[inst.section2]=rand()&inst.section34;
        break;
    case 0xD://DRW VX,VY,N
        //draw sprite
        {
            #define VRAMPOS(x,y) ((y)*64+(x))
            #define GETBIT(v,b) ((v>>b)&0x1)
            uint8_t xpos=cpu->V[inst.section2];
            uint8_t ypos=cpu->V[inst.section3];
            cpu->VF=0x0;
            for(int i=0;i<inst.section4;i++){
                for(int j=0;j<8;j++){
                    if(GETBIT(cpu->RAM[cpu->I+i],j)){
                        size_t pos=VRAMPOS(xpos+(7-j),ypos+i);
                        cpu->VRAM[pos]^=1;
                        if(!cpu->VRAM[pos]){
                            cpu->VF=0x1;
                        }
                    }
                }
            }
            #undef VRAMPOS
            #undef GETBIT
        }
        break;
    case 0xE:
        switch(inst.section34){
        case 0x9E://SKP VX
            //skip if key in register is pressed
            if(cpu->KB[cpu->V[inst.section2]])cpu->PC+=2;
            break;
        case 0xA1://SNKP VX
            //skip if key in register is not pressed
            if(!cpu->KB[cpu->V[inst.section2]])cpu->PC+=2;
            break;
        default:
            //INVALID_OPERATION
            return 0;
        }
        break;
    case 0xF:
        switch(inst.section34){
        case 0x07://MOV VX,DT
            //move DT into register
            cpu->V[inst.section2]=cpu->DT;
            break;
        case 0x0A://KEY VX
            //wait for key, store it in register
            cpu->wait_key=inst.section2;
            break;
        case 0x15://MOV DT,VX
            //move register into DT
            cpu->DT=cpu->V[inst.section2];
            break;
        case 0x18://MOV ST,VX
            //move register into ST
            cpu->ST=cpu->V[inst.section2];
            break;
        case 0x1E://ADD I,VX
            //add VX to I
            cpu->VF=(cpu->I+cpu->V[inst.section2]>0xFFF);
            cpu->I+=cpu->V[inst.section2];
            break;
        case 0x29://LDFNT VX
            //load font location representing register into I
            cpu->I=cpu->V[inst.section2]*5;
            break;
        case 0x33://BCD VX
            //store binary coded decimal representation of register into RAM
            cpu->RAM[cpu->I+2]=cpu->V[inst.section2]%10;
            cpu->RAM[cpu->I+1]=(cpu->V[inst.section2]/10)%10;
            cpu->RAM[cpu->I]=(cpu->V[inst.section2]/100);
            break;
        case 0x55://STR VX
            //store values from register V0 to VX to RAM
            for(int i=0;i<=inst.section2;i++){
                cpu->RAM[cpu->I+i]=cpu->V[i];
            }
            break;
        case 0x65://LDR VX
            //load values to register V0 to VX from RAM
            for(int i=0;i<=inst.section2;i++){
                cpu->V[i]=cpu->RAM[cpu->I+i];
            }
            break;
        default:
            //INVALID_OPERATION
            return 0;
        }
        break;
    }
    return 1;
}

int load_program(CPU_info * cpu,const char * file_path){
    static const int MAX_LENGTH = 3584;// 4096 - 0x200 ( RAM size - initial PC position)
    FILE * f=fopen(file_path,"rb");
    if(f){
        fseek(f,0,SEEK_END);
        int len=ftell(f);
        if(len>MAX_LENGTH){
            fclose(f);
            return 0;
        }
        fseek(f,0,SEEK_SET);
        fread(cpu->RAM+0x200,len,1,f);
        fclose(f);
        return 1;
    }else{
        return 0;
    }
}

void keydown(CPU_info * cpu, uint8_t key){
    if(key<0||key>15)return;
    cpu->KB[key]=1;
    if(cpu->wait_key>=0){
        cpu->V[cpu->wait_key]=key;
        cpu->wait_key=-1;
    }
}

void keyup(CPU_info * cpu, uint8_t key){
    if(key<0||key>15)return;
    cpu->KB[key]=0;
}
