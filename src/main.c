#define _NO_OLDNAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "cpu.h"
#include "io_control.h"
#include "command_handler.h"

void print_usage(){
    printf("usage:\n  c8emu [-debug] <ROM>");
}

int execute(const char * filename){
    int target_fps=60;//frames per second, affects counters
    int target_ops=60;//operations per second (speed of processor in hz)
    int frame_time=1000/target_fps;
    int cpu_time=1000/target_ops;
    CPU_info * cpu=new_cpu();
    if(load_program(cpu,filename)){
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
    return 0;
}

int debug(const char * filename){
    
    debug_data data={
        .paused=1,
        .buffer={},
        .buffer_pos=0,
        .target_fps=60,//frames per second, affects counters
        .target_ops=60,//operations per second (speed of processor in hz)
        .frame_time=1000/60,
        .cpu_time=1000/60,
        .cpu=new_cpu(),
        .echo=1,
    };
    if(load_program(data.cpu,filename)){
        if(init_io())return 1;
        printf(">");
        while(1){
            if(data.paused){
                if(poll_noio())break;
            }else{
                if(check_time(data.cpu_time)){
                    if(poll_io(data.cpu))break;
                    execute_instruction(data.cpu);
                }
                if(check_time(data.frame_time)){
                    draw(data.cpu);
                    delay_tick(data.cpu);
                }
            }
            if(_kbhit()){
                char c=_getch();
                if(c=='\b'){
                    if(data.buffer_pos>0){
                        data.buffer[--data.buffer_pos]=0;
                        printf("\b \b");
                    }
                }else if(c!='\n'&&c!='\r'){
                    if(data.buffer_pos<(MAX_BUFFER-1)){
                        printf("%c",c);
                        data.buffer[data.buffer_pos++]=c;
                        data.buffer[data.buffer_pos]=0;
                    }
                }else{//proccess command
                    size_t l=strlen(data.buffer)+1;
                    if(data.echo){
                        printf("\n");
                    }else{
                        for(size_t i=0;i<l;i++){
                            printf("\b \b");
                        }
                    }
                    if(execute_command(&data)) break;
                    printf(">");
                }
            }
        }
        exit_io();
        delete_cpu(data.cpu);
    }else{
        printf("Failed to load ROM (inexistent or too large)\n");
    }
    return 0;
    
}

int main(int argc,char ** argv){
    if(argc<2){
        printf("Too few arguments\n");
    }else if(argc==2){
        //has file parameter, run emulator
        return execute(argv[1]);
    }else if(argc==3){
        if(strcmp(argv[1],"-debug")==0){
            //has debug parameter, run emulator
            return debug(argv[2]);
        }else{
            printf("Invalid argument\n");
        }
    }else{
        printf("Too many arguments\n");
    }
    print_usage();
    return 1;
}
