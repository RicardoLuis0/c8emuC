#include "assemble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct _file_line_info{
    int lines;
    int max_len;
} file_line_info;

typedef struct _line_array{
    char ** data;
    int lines;
} line_array;

static file_line_info count_lines(FILE * f){
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
    return (file_line_info){line_count,max_len+1};
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
    file_line_info data=count_lines(f);
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
    struct _line_data * parent;
    struct _label_data * next;
} label_data;

static label_data * add_label(label_data * other,int line,char * name){
    label_data * temp=calloc(1,sizeof(label_data));
    temp->name=name;
    temp->line=line;
    temp->position=-1;
    temp->parent=NULL;
    temp->next=other;
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

static int is_valid_label_start_char(char c){
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';
}

static int is_valid_label_char(char c){
    return is_valid_label_start_char(c)||(c>='0'&&c<='9');
}

static int parse_label(const char * str,int line,label_data ** labels){
    int len=strlen(str);
    if(str[len-1]==':'){
        char * temp=calloc(len,sizeof(char));
        strncpy(temp,str,len-1);
        if(only_has_valid(temp,is_valid_label_char,len)&&is_valid_label_start_char(temp[0])){
            if(!find_label_name(*labels,temp,len)){
                (*labels)=add_label(*labels,line,temp);
                return 1;//success
            }else{
                printf("line %d: duplicate label '%s' ignored",line,temp);
            }
        }else{
            printf("line %d: invalid label name '%s'",line,temp);
        }
    }
    return 0;//fail
}

typedef struct _unresolved_label_jump_instruction{
    const char * label;
    
} unresolved_label_jump_instruction;

typedef struct _unresolved_instruction{
    enum{
        LABEL_JUMP,
    }type;
    union{
        unresolved_label_jump_instruction * label_jump;
    };
} unresolved_instruction;

typedef struct _resolved_instruction{
    uint16_t code;
} resolved_instruction;

typedef struct _line_data{
    enum {
        LINE_RESOLVED_INSTRUCTION,
        LINE_UNRESOLVED_INSTRUCTION,
        LINE_LABEL,
    } type;
    union{
        resolved_instruction * resolved;
        unresolved_instruction * unresolved;
        label_data * label;
    };
    int line;
    int pos;
    struct _line_data * next;
} line_data;

static int parse_line(const char * str,int line,label_data * labels,line_data ** head){
    return 0;
}

static void parse_lines(line_array arr){
    label_data * labels=NULL;
    line_data * start=NULL;
    line_data * head=NULL;
    int pos=0x200;
    for(int i=0;i<arr.lines;i++){
        if(!parse_label(arr.data[i],i,&labels)){//try to parse label line
            line_data * temp=calloc(0,sizeof(line_data));
            labels->parent=temp;
            labels->position=pos;
            temp->type=LINE_LABEL;
            temp->label=labels;
            temp->line=i;
            temp->pos=pos;
            if(head){
                head->next=temp;
                head=temp;
            }else{
                head=start=temp;
            }
        }else if(!parse_line(arr.data[i],i,labels,&head)){//try to parse code line
            if(!start)start=head;
            head->pos=pos;
            pos+=2;
            //TODO?
        }
    }
}

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
