#include "assemble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "instruction_data.h"
#include "util.h"

int is_hex_char(char c){
    return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
}

int c2hex(char c){
    if(c>='0'&&c<='9'){
        return c-'0';
    }else if(c>='a'&&c<='f'){
        return c-'a'+9;
    }else if(c>='A'&&c<='F'){
        return c-'A'+9;
    }
    return -1;
}

int is_hex_start(const char * s){
    return s[0]=='0'&&s[1]=='x';
}

int is_register(const char * s){
    return (s[0]=='V'||s[0]=='v');
}

int get_register_index(const char * s){
    if(!is_register(s))return -1;
    if(s[1]=='\0')return -1;
    if(s[1]=='0'&&s[2]=='x')return -1;
    return parse_number(s+1);
}

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
            lines[line]=calloc(data.max_len,sizeof(char));
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
    enum _jump_type{
        INSTR_JMP,
        INSTR_JMPV0,
        INSTR_PROC_CALL,
    } type;
    const char * label;
} unresolved_label_jump_instruction;

typedef struct _unresolved_instruction{
    enum{
        UNRESOLVED_LABEL_JUMP,
    }type;
    union{
        unresolved_label_jump_instruction * label_jump;
    };
    struct _unresolved_instruction * next;
    struct _line_data * parent;
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

static line_data * new_resolved_inst(instruction_data inst,int line){
    line_data * temp=calloc(1,sizeof(line_data));
    temp->type=LINE_RESOLVED_INSTRUCTION;
    temp->line=line;
    resolved_instruction * temp2=calloc(1,sizeof(resolved_instruction));
    temp2->code=inst.whole;
    temp->resolved=temp2;
    return temp;
}

static line_data * add(line_data * parent,line_data * new){
    parent->next=new;
    return new;
}

static int parse_line(const char * str1,int line,label_data * labels,line_data ** head){
    int ok=1;
    const char * str2=str1;
    while(*str2!='\0'&&*str2!=','&&*str2!=' '&&*str2!='\t')str2++;//find first whitespace or end of string
    int len1=str2-str1;
    char * op=strlnmake(str1,len1+1);
    if(*str2=='\0'&&*str1!='\0'){//no argument instruction, skip empty lines
        if(strcmp("CLS",op)==0){//CLS
            *head=add(*head,new_resolved_inst((instruction_data){.whole=0x00E0},line));
        }else if(strcmp("RET",op)==0){//RET
            *head=add(*head,new_resolved_inst((instruction_data){.whole=0x00EE},line));
        }else{
            ok=0;
        }
    }else{
        while(*str2==','||*str2==' '||*str2=='\t')str2++;//skip whitespace
        const char * str3=str2;
        while(*str3!='\0'&&*str3!=','&&*str3!=' '&&*str3!='\t')str3++;//find first whitespace or end of string
        int len2=str3-str2;
        char * arg1=strlnmake(str2,len2+1);
        if(*str3=='\0'){//single argument instruction
            if(strcmp("SYS",op)==0){
                int num=parse_number(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'SYS', expected number, got '%s'\n",arg1);
                }else{
                    if(num>4095){
                        ok=0;
                        printf("Invalid argument for instruction 'SYS', number too large, max 0xFFF, got 0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0x0,.section234=num},line));
                    }
                }
            }else if(strcmp("JMP",op)==0){
                int num=parse_number(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'JMP', expected number, got '%s'\n",arg1);
                }else{
                    if(num>4095){
                        ok=0;
                        printf("Invalid argument for instruction 'JMP', number too large, max 0xFFF, got 0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0x1,.section234=num},line));
                    }
                }
            }else if(strcmp("CALL",op)==0){
                int num=parse_number(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'CALL', expected number, got '%s'\n",arg1);
                }else{
                    if(num>4095){
                        ok=0;
                        printf("Invalid argument for instruction 'CALL', number too large, max 0xFFF, got 0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0x2,.section234=num},line));
                    }
                }
            }else if(strcmp("SKP",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'SKP', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'SKP', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xE,.section2=num,.section34=0x9E},line));
                    }
                }
            }else if(strcmp("SNKP",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'SNKP', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'SNKP', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xE,.section2=num,.section34=0xA1},line));
                    }
                }
            }else if(strcmp("KEY",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'KEY', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'KEY', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xF,.section2=num,.section34=0x0A},line));
                    }
                }
            }else if(strcmp("LDFNT",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'LDFNT', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'LDFNT', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xF,.section2=num,.section34=0x29},line));
                    }
                }
            }else if(strcmp("BCD",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'BCD', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'BCD', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xF,.section2=num,.section34=0x33},line));
                    }
                }
            }else if(strcmp("STR",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'STR', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'STR', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xF,.section2=num,.section34=0x55},line));
                    }
                }

            }else if(strcmp("LDR",op)==0){
                int num=get_register_index(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'LDR', expected register, got '%s'\n",arg1);
                }else{
                    if(num>15){
                        ok=0;
                        printf("Invalid argument for instruction 'LDR', invalid register, max VF, got V0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.section1=0xF,.section2=num,.section34=0x65},line));
                    }
                }

            }else if(strcmp("DATA",op)==0){
                int num=parse_number(arg1);
                if(num==-1){
                    ok=0;
                    printf("Invalid argument for instruction 'DATA', expected number, got '%s'\n",arg1);
                }else{
                    if(num>65535){
                        ok=0;
                        printf("Invalid argument for instruction 'DATA', number too large, max 0xFFFF, got 0x%X\n",num);
                    }else{
                        *head=add(*head,new_resolved_inst((instruction_data){.whole=num},line));
                    }
                }
            }else{
                ok=0;
                printf("Invalid instruction '%s'\n",op);
            }
        }else{
            while(*str3==','||*str3==' '||*str3=='\t')str3++;//skip whitespace
            const char * str4=str3;
            while(*str4!='\0'&&*str4!=','&&*str4!=' '&&*str4!='\t')str4++;//find first whitespace or end of string
            int len3=str4-str3;
            char * arg2=strlnmake(str3,len3+1);
            if(*str4=='\0'){//two argument instruction
                if(0){
                    //TODO 2 argument instructions
                }else{
                    ok=0;
                    printf("Invalid instruction '%s'\n",op);
                }
            }else{
                while(*str4==','||*str4==' '||*str4=='\t')str4++;//skip whitespace
                const char * strend=str4;
                while(*strend!='\0'&&*strend!=','&&*strend!=' '&&*strend!='\t')strend++;//find first whitespace or end of string
                int len4=strend-str4;
                char * arg3=strlnmake(str4,len4+1);
                if(*strend=='\0'){//three argument instruction
                    if(0){
                        //TODO 3 argument instructions
                    }else{
                        ok=0;
                        printf("Invalid instruction '%s'\n",op);
                    }
                }else{
                    ok=0;
                    printf("Invalid instruction '%s'\n",op);
                }
                free(arg3);
            }
            free(arg2);
        }
        free(arg1);
    }
    free(op);
    return ok;
}

static uint8_t * parse_lines(line_array arr,size_t * size){//return unterminated array with binary code, store array length in size
    int ok=1;
    label_data * labels=NULL;
    line_data * start=NULL;
    line_data * head=NULL;
    unresolved_instruction * un_start=NULL;
    unresolved_instruction * un_head=NULL;
    int pos=0x200;
    for(int i=0;i<arr.lines;i++){
        if(parse_label(arr.data[i],i,&labels)){//try to parse label line
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
        }else if(parse_line(arr.data[i],i,labels,&head)){//try to parse code line
            if(!start)start=head;
            head->pos=pos;
            pos+=2;
            head->unresolved->parent=head;
            if(head->type==LINE_UNRESOLVED_INSTRUCTION){
                if(un_head){
                    un_head->next=head->unresolved;
                    un_head=head->unresolved;
                }else{
                    un_head=un_start=head->unresolved;
                }
            }
        }else{
            ok=0;
        }
    }
    while(un_start!=NULL){
        if(un_start->type==UNRESOLVED_LABEL_JUMP){
            label_data *label=find_label_name(labels,un_start->label_jump->label,strlen(un_start->label_jump->label));
            if(label){
                //TODO resolve unresolved instructions (forwards label jumps)
            }else{
                ok=0;
                printf("Unknown label %s in line %d\n",un_start->label_jump->label,un_start->parent->line);
            }
        }
        un_start=un_start->next;
    }
    if(ok){
        int len=pos-0x200;
        if(len>0){
            uint8_t * data=calloc(len,sizeof(uint8_t));
            printf("Assembler not implemented\n");
            //TODO copy instructions into code array
            *size=len;
            return data;
        }else{
            printf("No valid instructions to be assembled\n");
            return NULL;
        }
    }else{
        return NULL;
    }
}

int assemble(const char * in,const char * out){
    line_array arr=read_file_into_line_array(in);
    if(arr.data){
        for(int i=0;i<arr.lines;i++){
            char * temp=strip_whitespace_comments(arr.data[i]);
            if(temp!=arr.data[i]){
                free(arr.data[i]);
                arr.data[i]=temp;
            }
        }
        FILE * f=fopen(out,"wb");
        if(f){
            size_t len;
            uint8_t * data=parse_lines(arr,&len);
            if(data){
                fwrite(data,sizeof(uint8_t),len,f);
                free(data);
            }else{
                printf("Could not assemble %s\n",in);
            }
            fclose(f);
        }else{
            printf("Could not write to file %s\n",out);
        }
        free_line_array(arr);
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
