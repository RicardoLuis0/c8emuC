#ifndef CPU_H_INCLUDED
#define CPU_H_INCLUDED

#include "cpu_info.h"

CPU_info * init_cpu(CPU_info * cpu);//initializes struct data, returns same pointer that was passed to it
CPU_info * new_cpu();//allocates and initializes CPU_info
int execute_instruction(CPU_info * cpu);
int load_program(CPU_info * cpu,const char * file_path);
void keyup(CPU_info * cpu, uint8_t key);
void keydown(CPU_info * cpu, uint8_t key);
#endif // CPU_H_INCLUDED
