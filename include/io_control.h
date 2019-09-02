#ifndef IO_CONTROL_H_INCLUDED
#define IO_CONTROL_H_INCLUDED

#include "cpu.h"

int check_time(int msdelay);

int init_io();

void draw(CPU_info * cpu);

int poll_io(CPU_info * cpu);

void exit_io();

#endif // IO_CONTROL_H_INCLUDED
