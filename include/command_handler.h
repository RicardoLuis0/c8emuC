#ifndef COMMAND_HANDLER_H_INCLUDED
#define COMMAND_HANDLER_H_INCLUDED

#include <stdlib.h>

#include "cpu.h"

#define MAX_BUFFER 256

typedef struct _debug_data{
    int paused;
    char buffer[256];
    size_t buffer_pos;
    int target_fps;
    int target_ops;
    int frame_time;
    int cpu_time;
    int echo;
    CPU_info * cpu;
} debug_data;

int execute_command(debug_data * data);

void calc_times(debug_data * data);
#endif // COMMAND_HANDLER_H_INCLUDED
