#include "command_handler.h"
#include "cpu_info.h"
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "disassemble.h"

debug_data * new_debug_data(){
    debug_data * data=calloc(1,sizeof(debug_data));
    data->paused=1;
    data->target_fps=60;//frames per second, affects counters
    data->target_ops=60;//operations per second (speed of processor in hz)
    data->frame_time=1000/60;
    data->cpu_time=1000/60;
    data->cpu=new_cpu();
    data->echo=1;
    return data;
}

#define MAX_ARGS 5

static int find_command(const char cmd[MAX_BUFFER]);

///split string by spaces and put into array
static void split_cmd(const char cmd[MAX_BUFFER],char arr[MAX_ARGS][MAX_BUFFER]){
    for(size_t i=0;i<MAX_ARGS;i++){
        const char * tmp=strchr(cmd,' ');
        if(!tmp){
            tmp=strchr(cmd,0);
        }else{
        }
        if(tmp){
            strlncpy(arr[i],cmd,tmp-cmd+1);
            while(tmp[0]!=0&&tmp[1]==' ')tmp++;//skip multiple spaces
            if(*tmp==0)return;
            cmd=++tmp;
        }
    }
}

#define MAX_COMMAND 9

static const char CMD_STR [MAX_COMMAND][MAX_BUFFER]={
    "pause",//0
    "resume",//1
    "exit",//2
    "help",//3
    "echo",//4
    "cls",//5
    "break",//6
    "peek",//7
    "step",//8
};

static const char CMD_HELP [MAX_COMMAND][MAX_BUFFER]={
    "Pause Emulation Execution",
    "Resume Emulation Execution",
    "Exit Emulation",
    "List Commands/Show Usage",
    "Turn On/Off Command Echoing",
    "Clear Terminal Screen",
    "Manage Breakpoints",
    "View Registers/RAM",
    "Execute one Instruction",
};

static const char CMD_USAGE [MAX_COMMAND][MAX_BUFFER*5]={
    "'pause'",//pause
    "'resume'",//resume
    "'exit'",//exit
    "'help [command]'",//help
    "'echo on|off'",//echo
    "'cls'",//cls
    "'break set <location>'\n  - Sets a breakpoint at 'location' in memory\n"
    " 'break unset <location>'\n  - Unsets the breakpoint at 'location' in memory\n"
    " 'break clear'\n  - Unsets all breakpoints\n"
    " 'break list'\n  - Lists all breakpoints\n",//break
    "'peek reg'\n  - List register values\n"
    " 'peek mem abs <start_location> <end_location>'\n  - Displays the contents of memory between 'start_location' and 'end_location'\n"
    " 'peek mem rel <negative_offset> <positive_offset>'\n  - Displays the contents of memory with offsets from the PC location\n",//peek
    "'step'",//step
};

int no_cmd(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    printf("Invalid Command '%s'\n",cmd_data[0]);
    return 0;
}

int pause(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    printf("Pausing Execution\n");
    data->paused=1;
    return 0;
}

int break_cmd(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    if(strcmp(cmd_data[1],"set")==0){
        int num=parse_number(cmd_data[2]);
        if(num==-1){
            printf("Invalid arguments for 'break set'\n\nUsage:\n %s\n",CMD_USAGE[6]);
            return 0;
        }
        for(int i=0;i<data->break_count;i++){
            if(data->breaks[i]==num){
                printf("Could not set breakpoint 0x%X , it's already set\n",num);
                return 0;
            }
        }
        if(data->break_count==MAX_BREAKS){
            printf("Could not set breakpoint 0x%X, max number of breakpoints (%d) reached\n",num,MAX_BREAKS);
            return 0;
        }
        //set breakpoint
        data->breaks[data->break_count++]=num;
        printf("Breakpoint 0x%X set (%d/%d)\n",num,data->break_count,MAX_BREAKS);
    }else if(strcmp(cmd_data[1],"unset")==0){
        int num=parse_number(cmd_data[2]);
        if(num==-1){
            printf("Invalid arguments for 'break set'\n\nUsage:\n %s\n",CMD_USAGE[6]);
            return 0;
        }
        int move=0;
        for(int i=0;i<data->break_count;i++){
            if(move){//move is always 0 in first loop
                data->breaks[i-1]=data->breaks[i];
            }else if(data->breaks[i]==num){
                move=1;
            }
        }
        if(move==0){
            printf("Could not unset breakpoint 0x%X , it's not set\n",num);
        }else{
            data->break_count--;
            printf("Breakpoint 0x%X unset\n",num);
        }
    }else if(strcmp(cmd_data[1],"clear")==0){
        if(data->break_count>0){
            data->break_count=0;
            printf("Breakpoints cleared\n");
        }else{
            printf("Could not clear breakpoints, there are no breakpoints set\n");
        }
    }else if(strcmp(cmd_data[1],"list")==0){
        if(data->break_count>0){
            printf("Breakpoints (%d/%d):\n\n",data->break_count,MAX_BREAKS);
            for(int i=0;i<data->break_count;i++){
                printf(" #%d = 0x%X\n",i+1,data->breaks[i]);
            }
        }else{
            printf("Could not list breakpoints, there are no breakpoints set\n");
        }
    }else{
        printf("Invalid arguments for 'break'\n\nUsage:\n %s\n",CMD_USAGE[6]);
    }
    return 0;
}

void break_proccess(debug_data * data){
    if(data->paused)return;//don't run while paused
    for(size_t i=0;i<data->break_count;i++){
        if(data->cpu->PC==data->breaks[i]){
            size_t l=strlen(data->buffer)+1;
            for(size_t j=0;j<l;j++){
                printf("\b \b");//erase all characters
            }
            data->paused=1;
            printf("Breakpoint (0x%X) triggered, Pausing Execution\n>%s",data->cpu->PC,data->buffer);
            return;
        }
    }
}

int resume(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    printf("Resuming Execution\n");
    data->paused=0;
    return 0;
}

int exit_cmd(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    printf("Exiting Emulator\n");
    return 1;
}

int help(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    if(cmd_data[1][0]==0){
        printf("Available Commands:\n");
        for(size_t i=0;i<MAX_COMMAND;i++){
            printf(" - %s\n",CMD_STR[i]);
        }
    }else{
        int num=find_command(cmd_data[1]);
        if(num==-1){
            printf("Invalid Command '%s', Usage: %s\n",cmd_data[1],CMD_USAGE[3]);
        }else{
            printf("'%s' command,\n   %s\nUsage:\n %s\n",CMD_STR[num],CMD_HELP[num],CMD_USAGE[num]);
        }
    }
    return 0;
}

int echo(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    if(cmd_data[1][0]=='o'&&cmd_data[1][1]=='n'&&cmd_data[1][2]==0){
        data->echo=1;
    }else if(cmd_data[1][0]=='o'&&cmd_data[1][1]=='f'&&cmd_data[1][2]=='f'&&cmd_data[1][3]==0){
        data->echo=0;
    }else{
        printf("Invalid arguments for 'echo'\n\nUsage:\n %s\n",CMD_USAGE[4]);
    }
    return 0;
}

int cls(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    system("cls");
    return 0;
}

int peek(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    if(strcmp(cmd_data[1],"reg")==0){
        printf("PC:0x%03X\n",data->cpu->PC);
        printf("SP:0x%03X\n",data->cpu->SP);
        printf("DT:0x%03X\n",data->cpu->DT);
        printf("ST:0x%03X\n",data->cpu->ST);
        printf("I:0x%03X\n",data->cpu->I);
        for(uint8_t i=0;i<8;i++){
            printf("V%X: 0x%03X, V%X: 0x%03X; KB '%X': %s, KB '%X': %s;\n",i,data->cpu->V[i],i+8,data->cpu->V[i+8],i,(data->cpu->KB[i]>0?"on ":"off"),i+8,(data->cpu->KB[i+8]>0?"on ":"off"));
        }
    }else if(strcmp(cmd_data[1],"mem")==0){
        int rel=0;
        if(strcmp(cmd_data[2],"abs")!=0){
            if(strcmp(cmd_data[2],"rel")==0){
                rel=1;
            }else{
                printf("Invalid arguments for 'peek'\n\nUsage:\n %s\n",CMD_USAGE[7]);
                return 0;
            }
        }
        int start=parse_number(cmd_data[3]);
        int end=parse_number(cmd_data[4]);
        if(start==-1||end==-1){
            printf("'start_location' and 'end_location' must be valid decimal/hex numbers\n\nUsage:\n %s\n",CMD_USAGE[7]);
            return 0;
        }
        if(start%2||end%2){
            printf("'start_location' and 'end_location' must be even numbers\n\nUsage:\n %s\n",CMD_USAGE[7]);
            return 0;
        }
        if(rel){
            start=data->cpu->PC-start;
            end=data->cpu->PC+end;
        }else{
            if(start>end){//make sure end is larger than start
                printf("'start_location' must be lower than 'end_location'\n\nUsage:\n %s\n",CMD_USAGE[7]);
                return 0;
            }
        }
        if(start<0)start=0;
        if(end>0xFFF)end=0xFFF;
        printf("(0x%04X-0x%04X)\n",start,end);
        instruction_data temp;
        char buf[64];
        for(int i=start;i<=end;i+=2){
            temp.section12=data->cpu->RAM[i];
            temp.section34=data->cpu->RAM[i+1];
            get_instruction_str(disassemble_instruction(temp),buf,64);
            printf("0x%04X - %s\n",i,buf);
        }
    }else{
        printf("Invalid arguments for 'peek'\n\nUsage:\n %s\n",CMD_USAGE[7]);
    }
    return 0;
}

int step(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    if(data->paused){
        printf("Stepping one instruction\n");
        execute_instruction(data->cpu);
    }else{
        printf("Program not paused, cannot step\n");
    }
    return 0;
}

typedef int (*cmd_fp)(debug_data*,const char[MAX_ARGS][MAX_BUFFER]);


static const cmd_fp CMD_FP [MAX_COMMAND]={
    pause,
    resume,
    exit_cmd,
    help,
    echo,
    cls,
    break_cmd,
    peek,
    step,
};

int find_command(const char cmd[MAX_BUFFER]){
    for(size_t i=0;i<MAX_COMMAND;i++){
        if(strcmp(cmd,CMD_STR[i])==0)return i;
    }
    return -1;
}

int execute_command(debug_data * data){
    char cmd_data[MAX_ARGS][MAX_BUFFER];
    memset(cmd_data,0,sizeof(cmd_data));
    split_cmd(data->buffer,cmd_data);
    data->buffer_pos=0;
    data->buffer[0]=0;
    size_t num=find_command(cmd_data[0]);
    if(num==-1)return no_cmd(data,cmd_data);
    return CMD_FP[num](data,cmd_data);
}

void calc_times(debug_data * data){
    data->frame_time=1000/data->target_fps;
    data->cpu_time=1000/data->target_ops;
}
