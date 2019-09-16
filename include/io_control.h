#ifndef IO_CONTROL_H_INCLUDED
#define IO_CONTROL_H_INCLUDED

#include "cpu.h"

int check_time(int msdelay);

int has_focus();

int init_io(int is_debug);

void draw(CPU_info * cpu);

int poll_io(CPU_info * cpu);

int poll_noio();

void exit_io();

#endif // IO_CONTROL_H_INCLUDED
