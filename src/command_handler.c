#include "command_handler.h"
#include <stdio.h>
#include <string.h>

char * strlcpy(char *dst, const char *src, size_t size){
    strncpy(dst,src,size-1);
    dst[size-1]=0;
    return dst;
}

#define MAX_ARGS 5

static int find_command(const char cmd[MAX_BUFFER]);

///split string by spaces and put into array
static void split_cmd(const char cmd[MAX_BUFFER],char arr[MAX_ARGS][MAX_BUFFER]){
    for(size_t i=0;i<MAX_ARGS;i++){
        const char * tmp=strchr(cmd,' ');
        if(!tmp){
            tmp=strchr(cmd,0);
        }
        if(tmp){
            strlcpy(arr[i],cmd,tmp-cmd+1);
            if(*tmp==0)return;
            cmd=++tmp;
        }
    }
}

#define MAX_COMMAND 6

static const char CMD_STR [MAX_COMMAND][MAX_BUFFER]={
    "pause",
    "resume",
    "exit",
    "help",
    "echo",
    "cls",
};

static const char CMD_HELP [MAX_COMMAND][MAX_BUFFER]={
    "Pause Emulation Execution",
    "Resume Emulation Execution",
    "Exit Emulation",
    "List Commands/Show Usage",
    "Turn On/Off Command Echoing",
    "Clear Terminal Screen",
};

static const char CMD_USAGE [MAX_COMMAND][MAX_BUFFER]={
    "pause",
    "resume",
    "exit",
    "help [command]",
    "echo on|off",
    "cls",
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
            printf("Invalid Command '%s', usage: %s\n",cmd_data[1],CMD_USAGE[3]);
        }else{
            printf("'%s' command,\n   %s\nusage:\n %s\n",CMD_STR[num],CMD_HELP[num],CMD_USAGE[num]);
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
        printf("Invalid arguments for echo\n\nusage:\n %s\n",CMD_USAGE[4]);
    }
    return 0;
}

int cls(debug_data * data,const char cmd_data[MAX_ARGS][MAX_BUFFER]){
    system("cls");
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
    size_t num=find_command(cmd_data[0]);
    if(num==-1)return no_cmd(data,cmd_data);
    return CMD_FP[num](data,cmd_data);
}

void calc_times(debug_data * data){
    data->frame_time=1000/data->target_fps;
    data->cpu_time=1000/data->target_ops;
}
