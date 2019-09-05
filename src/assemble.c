#include "assemble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return (line_data){line_count,max_len+1};
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

static int only_has_valid(char * str,int (*valid_predicate)(char c),int size){
    for(int i=0;i<size&&*str!='\0';i++,str++){
        if(!valid_predicate(*str))return 0;
    }
    return 1;
}

static int has_char(char * str,char c,int size){
    for(int i=0;i<size&&*str!='\0';i++,str++){
        if(*str==c)return 1;
    }
    return 0;
}

static char * strip_whitespace_comments(const char * in){
    const char * start=in;
    while(*start==' '||*start=='\t')start++;//skip trailing whitespace
    const char * end=start;
    while(*end!='\0'&&*end!=';')end++;//find end of string
    while(end>start&&end[-1]==' ')end--;//skip trailing whitespace
    int len=end-start;
    if(len>0){
        char * temp=calloc(len+1,sizeof(char));
        strncpy(temp,start,len);
        return temp;
    }
    return calloc(1,sizeof(char));
}

typedef struct _label_data{
    char * name;
    int line;
    int position;//will be -1, set during line parsing
    struct _label_data * next;
} label_data;

static label_data * add_label(label_data * parent,int line,char * name){
    label_data * temp=calloc(1,sizeof(label_data));
    temp->name=name;
    temp->line=line;
    temp->position=-1;
    temp->next=parent;
    return temp;
}

static label_data * find_label_name(label_data * labels,const char * name,size_t n){
    while(labels!=NULL){
        if(strncmp(name,labels->name,n)==0)return labels;//label found
        labels=labels->next;
    }
    return NULL;//no label found
}

static label_data * find_label_line(label_data * labels,int line){
    while(labels!=NULL){
        if(line==labels->line)return labels;//label found
        labels=labels->next;
    }
    return NULL;//no label found
}

static int is_valid_label_char(char c){
    return (c>'a'&&c<'z')||(c>'A'&&c<'Z')||c=='_';
}

static label_data * parse_labels(line_array arr){
    label_data * labels=NULL;
    for(int i=0;i<arr.lines;i++){
        int len=strlen(arr.data[i]);
        if(arr.data[i][len-1]==':'){
            char * temp=calloc(len,sizeof(char));
            strncpy(temp,arr.data[i],len-1);
            if(only_has_valid(temp,is_valid_label_char,len)){
                if(!find_label_name(labels,temp,len)){
                    labels=add_label(labels,i,temp);
                }else{
                    printf("line %d: duplicate label '%s' ignored",i,temp);
                }
            }else{
                printf("line %d: invalid label name '%s'",i,temp);
            }
        }
    }
    return labels;
}

typedef struct _instruction{
    
}instruction;

typedef struct _line{
    
}line;

int assemble(const char * in,const char * out){
    line_array arr=read_file_into_line_array(in);
    if(arr.data){
        for(int i=0;i<arr.lines;i++){
            char * temp=strip_whitespace_comments(arr.data[i]);
            free(arr.data[i]);
            arr.data[i]=temp;
        }
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
