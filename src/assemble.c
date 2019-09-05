#include "assemble.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct _line_data{
    int lines;
    int max_len;
} line_data;

typedef struct _line_array{
    char ** data;
    int lines;
} line_array;

static line_data count_lines(FILE * f){
    int line_count=1;
    int len=0;
    int max_len=0;
    for(char c;(c=fgetc(f))!=EOF;){
        if(c=='\n'){
            line_count++;
            if(len>max_len)max_len=len;
            len=0;
        }else{
            len++;
        }
    }
    fseek(f,0,SEEK_SET);
    return (line_data){line_count,max_len};
}

static void free_line_array(line_array arr){
    if(arr.data){
        for(int i=0;i<arr.lines;i++){
            free(arr.data[i]);
        }
        free(arr.data);
    }
}

//read file into a line array
static line_array read_file_into_line_array(const char * filename){
    FILE * f=fopen(filename,"r");
    if(!f)return (line_array){NULL,-1};
    line_data data=count_lines(f);
    char ** lines=calloc(data.lines,sizeof(char*));
    //char * buffer=calloc(data.max_len,sizeof(char));
    int line=0;
    int count=0;
    for(char c;(c=fgetc(f))!=EOF;){
        if(lines[line]==NULL){
            lines[line]=calloc(data.max_len,sizeof(char));
            count=0;
        }
        if(c=='\n'){
            count=0;
            line++;
        }else{
            lines[line][count++]=c;
        }
    }
    return (line_array){lines,data.lines};
}

int assemble(const char * in,const char * out){
    line_array arr=read_file_into_line_array(in);
    if(arr.data){
        free_line_array(arr);
        printf("Assembler not implemented\n");
        return 0;
    }else{
        printf("Could not open file %s\n",in);
        return 1;
    }
}

int disassemble(const char * in,const char * out){
    printf("Disassembler not implemented\n");
    return 0;
}
