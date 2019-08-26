#ifndef CPU_H_INCLUDED
#define CPU_H_INCLUDED

#include "cpu_info.h"

typedef struct CPU_info CPU_info;

CPU_info * init_cpu(CPU_info * cpu);//initializes struct data, returns same pointer that was passed to it
CPU_info * new_cpu();//allocates and initializes CPU_info

#endif // CPU_H_INCLUDED
