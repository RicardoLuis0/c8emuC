#ifndef CPU_H_INCLUDED
#define CPU_H_INCLUDED

#include <stdint.h>

typedef struct _CPU_info CPU_info;

CPU_info * new_cpu();//allocates and initializes CPU_info
void delete_cpu(CPU_info *);
int execute_instruction(CPU_info * cpu);
int load_program(CPU_info * cpu,const char * file_path);
void keyup(CPU_info * cpu, uint8_t key);
void keydown(CPU_info * cpu, uint8_t key);
void delay_tick(CPU_info * cpu);

#endif // CPU_H_INCLUDED
