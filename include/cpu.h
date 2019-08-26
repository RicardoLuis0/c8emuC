#ifndef CPU_H_INCLUDED
#define CPU_H_INCLUDED

#include "cpu_info.h"

CPU_info * init_cpu(CPU_info * cpu);//initializes struct data, returns same pointer that was passed to it
CPU_info * new_cpu();//allocates and initializes CPU_info
void execute_instruction(CPU_info * cpu);
#endif // CPU_H_INCLUDED
