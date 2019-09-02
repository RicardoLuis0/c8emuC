#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "io_control.h"

void print_usage(){
    printf("usage:\n  c8emu <ROM>");
}

int main(int argc,char ** argv){
    int target_fps=60;//frames per second, affects counters
    int target_ops=60;//operations per second (speed of processor in hz)
    int frame_time=1000/target_fps;
    int cpu_time=1000/target_ops;

    if(argc<2){
        printf("Too few arguments\n");
        print_usage();
    }else if(argc>2){
        printf("Too many arguments\n");
        print_usage();
    }else{
        //has file parameter, run emulator
        CPU_info * cpu=new_cpu();
        if(load_program(cpu,argv[1])){
            if(init_io())return 1;
            while(1){
                if(check_time(cpu_time)){
                    if(poll_io(cpu))break;
                    execute_instruction(cpu);
                }
                if(check_time(frame_time)){
                    draw(cpu);
                    delay_tick(cpu);
                }
            }
            exit_io();
            delete_cpu(cpu);
        }else{
            printf("Failed to load ROM (inexistent or too large)\n");
        }
    }
    return 0;
}
