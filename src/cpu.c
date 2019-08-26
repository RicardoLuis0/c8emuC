#include "cpu.h"
#include "cpu_info.h"
#include "fontset.h"
#include <string.h>
#include <stdlib.h>

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
    return cpu;
}

CPU_info * new_cpu(){
    return init_cpu(malloc(sizeof(CPU_info)));
}
