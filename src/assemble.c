#include "assemble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "instruction_data.h"
#include "util.h"
#include "disassemble.h"

int is_hex_char(char c) {
    return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
}

int c2hex(char c) {
    if(c>='0'&&c<='9') {
        return c-'0';
    } else if(c>='a'&&c<='f') {
        return c-'a'+9;
    } else if(c>='A'&&c<='F') {
        return c-'A'+9;
    }
    return -1;
}

int is_hex_start(const char * s) {
    return s[0]=='0'&&s[1]=='x';
}

int is_register(const char * s) {
    return (s[0]=='V'||s[0]=='v');
}

int get_register_index(const char * s) {
    if(!is_register(s)) {
        return -1;
    }else if(s[1]=='\0') {
        return -1;
    }else if(s[2]!='\0') {
        return -1;
    }else if(s[1]>='0'&&s[1]<='9'){
        return s[1]-'0';
    }else if(s[1]>='a'&&s[1]<='f'){
        return (s[1]-'a')+10;
    }else if(s[1]>='A'&&s[1]<='F'){
        return (s[1]-'A')+10;
    }else{
        return -1;
    }
}

typedef struct _file_line_info {
    int lines;
    int max_len;
} file_line_info;

typedef struct _line_array {
    char ** data;
    int lines;
} line_array;

static file_line_info count_lines(FILE * f) {
    int line_count=1;
    int len=0;
    int max_len=0;
    for(char c; (c=fgetc(f))!=EOF;) {
        if(c=='\n') {
            line_count++;
            if(len>max_len)
                max_len=len;
            len=0;
        } else {
            len++;
        }
    }
    fseek(f,0,SEEK_SET);
    return (file_line_info) {
        line_count,max_len+1
    };
}

static void free_line_array(line_array arr) {
    if(arr.data) {
        for(int i=0; i<arr.lines; i++) {
            free(arr.data[i]);
        }
        free(arr.data);
    }
}

//read file into a line array
static line_array read_file_into_line_array(const char * filename) {
    FILE * f=fopen(filename,"r");
    if(!f) {
        return (line_array) {
            NULL,
            -1,
        };
    }
    file_line_info data=count_lines(f);
    char ** lines=calloc(data.lines,sizeof(char*));
    //char * buffer=calloc(data.max_len,sizeof(char));
    int line=0;
    int count=0;
    for(char c; (c=fgetc(f))!=EOF;) {
        if(lines[line]==NULL) {
            lines[line]=calloc(data.max_len,sizeof(char));
            count=0;
        }
        if(c=='\n') {
            count=0;
            line++;
            lines[line]=calloc(data.max_len,sizeof(char));
        } else {
            lines[line][count++]=c;
        }
    }
    return (line_array) {
        lines,
        data.lines,
    };
}

static int only_has_valid(const char * str,int (*valid_predicate)(char c),int size) {
    for(int i=0; i<size&&*str!='\0'; i++,str++) {
        if(!valid_predicate(*str)) {
            return 0;
        }
    }
    return 1;
}

static char * strip_whitespace_comments(const char * in) {
    const char * start=in;
    while(*start==' '||*start=='\t') { //skip trailing whitespace
        start++;
    }
    const char * end=start;
    while(*end!='\0'&&*end!=';') { //find end of string
        end++;
    }
    while(end>start&&end[-1]==' ') { //skip trailing whitespace
        end--;
    }
    int len=end-start;
    if(len>0) {
        char * temp=calloc(len+1,sizeof(char));
        strncpy(temp,start,len);
        return temp;
    }
    return calloc(1,sizeof(char));
}

typedef struct _label_data {
    char * name;
    int line;
    int position;//will be -1, set during line parsing right after label creation
    struct _line_data * parent;
    struct _label_data * next;
} label_data;

static label_data * add_label(label_data * other,int line,char * name) {
    label_data * temp=calloc(1,sizeof(label_data));
    temp->name=name;
    temp->line=line;
    temp->position=-1;
    temp->parent=NULL;
    temp->next=other;
    return temp;
}

static label_data * find_label_name(label_data * labels,const char * name) {
    while(labels!=NULL) {
        if(strcmp(name,labels->name)==0) {
            return labels;//label found
        }
        labels=labels->next;
    }
    return NULL;//no label found
}

static label_data * find_label_pos(label_data * labels,int pos) {
    while(labels!=NULL) {
        if(labels->position==pos) {
            return labels;//label found
        }
        labels=labels->next;
    }
    return NULL;//no label found
}

static int is_valid_label_start_char(char c) {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';
}

static int is_valid_label_char(char c) {
    return is_valid_label_start_char(c)||(c>='0'&&c<='9');
}

static int is_valid_label(const char * str,int len) {
    return only_has_valid(str,is_valid_label_char,len)&&is_valid_label_start_char(str[0]);
}

static int parse_label(const char * str,int line,label_data ** labels) {
    size_t len=strlen(str);
    if(str[len-1]==':') {
        char * temp=strlncpymake(str,len);
        if(is_valid_label(temp,len)) {
            if(!find_label_name(*labels,temp)) {
                (*labels)=add_label(*labels,line,temp);
                return 1;//success
            } else {
                printf("line %d: duplicate label '%s' ignored",line,temp);
            }
        } else {
            printf("line %d: invalid label name '%s'",line,temp);
        }
        free(temp);//fix invalid/duplicate label memory leak
    }
    return 0;//fail
}

typedef struct _unresolved_label_jump_instruction {
    uint8_t section1;
    char * label;
    int len;
} unresolved_label_jump_instruction;

typedef struct _unresolved_instruction {
    enum {
        UNRESOLVED_LABEL_JUMP,
    } type;
    union {
        unresolved_label_jump_instruction * label_jump;
    };
    struct _unresolved_instruction * next;
    struct _line_data * parent;
} unresolved_instruction;

typedef struct _resolved_instruction {
    instruction_data code;
} resolved_instruction;

typedef struct _line_data {
    enum {
        LINE_RESOLVED_INSTRUCTION,
        LINE_UNRESOLVED_INSTRUCTION,
        LINE_LABEL,
    } type;
    union {
        resolved_instruction * resolved;
        unresolved_instruction * unresolved;
        label_data * label;
    };
    int line;
    int pos;
    struct _line_data * next;
} line_data;

static line_data * new_unresolved_jump(int section1,const char * name,int len,int line) {
    unresolved_label_jump_instruction * temp=calloc(1,sizeof(unresolved_label_jump_instruction));
    temp->len=len;
    temp->label=strlncpymake(name,len+1);
    temp->section1=section1;
    unresolved_instruction * temp2=calloc(1,sizeof(unresolved_instruction));
    temp2->label_jump=temp;
    temp2->type=UNRESOLVED_LABEL_JUMP;
    line_data * temp3=calloc(1,sizeof(line_data));
    temp3->type=LINE_UNRESOLVED_INSTRUCTION;
    temp3->line=line;
    temp3->unresolved=temp2;
    return temp3;
}

static line_data * new_resolved_inst(instruction_data inst,int line) {
    resolved_instruction * temp=calloc(1,sizeof(resolved_instruction));
    temp->code=inst;
    line_data * temp2=calloc(1,sizeof(line_data));
    temp2->type=LINE_RESOLVED_INSTRUCTION;
    temp2->line=line;
    temp2->resolved=temp;
    return temp2;
}

static line_data * add(line_data * parent,line_data * new) {
    if(parent) {
        parent->next=new;
    }
    return new;
}

static int parse_line(const char * str1,int line,label_data * labels,line_data ** head) {
    if(*str1!='\0') {
        int ok=1;
        const char * str2=str1;
        while(*str2!='\0'&&*str2!=','&&*str2!=' '&&*str2!='\t') { //find first whitespace or end of string
            str2++;
        }
        int len1=str2-str1;
        char * op=strlncpymake(str1,len1+1);
        if(*str2=='\0') { //no argument instruction, skip empty lines
            if(strcmp("CLS",op)==0) { //CLS
                instruction_data inst;
                inst.whole=0x00E0;
                *head=add(*head,new_resolved_inst(inst,line));
            } else if(strcmp("RET",op)==0) { //RET
                instruction_data inst;
                inst.whole=0x00EE;
                *head=add(*head,new_resolved_inst(inst,line));
            } else {
                ok=0;
            }
        } else {
            while(*str2==','||*str2==' '||*str2=='\t') { //skip whitespace
                str2++;
            }
            const char * str3=str2;
            while(*str3!='\0'&&*str3!=','&&*str3!=' '&&*str3!='\t') { //find first whitespace or end of string
                str3++;
            }
            int len2=str3-str2;
            char * arg1=strlncpymake(str2,len2+1);
            if(*str3=='\0') { //single argument instruction
                if(strcmp("SYS",op)==0) { //SYS NNN
                    int num=parse_number(arg1);
                    if(num==-1) {
                        if(is_valid_label(arg1,len2)) {
                            label_data * lbl=find_label_name(labels,arg1);
                            if(lbl) {
                                instruction_data inst;
                                inst.section1=0x0;
                                inst.section234=lbl->position;
                                *head=add(*head,new_resolved_inst(inst,line));
                            } else { //label not defined yet, add
                                *head=add(*head,new_unresolved_jump(0x0,arg1,len2,line));
                            }
                        } else {
                            ok=0;
                            printf("Invalid argument for instruction 'SYS', expected number, got '%s'\n",arg1);
                        }
                    } else if(num>0xFFF) {
                        ok=0;
                        printf("Invalid argument for instruction 'SYS', number too large, max 0xFFF, got 0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0x0;
                        inst.section234=num;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("JMP",op)==0) { //JMP NNN
                    int num=parse_number(arg1);
                    if(num==-1) {
                        if(is_valid_label(arg1,len2)) {
                            label_data * lbl=find_label_name(labels,arg1);
                            if(lbl) {
                                instruction_data inst;
                                inst.section1=0x1;
                                inst.section234=lbl->position;
                                *head=add(*head,new_resolved_inst(inst,line));
                            } else { //label not defined yet, add
                                *head=add(*head,new_unresolved_jump(0x1,arg1,len2,line));
                            }
                        } else {
                            ok=0;
                            printf("Invalid argument for instruction 'JMP', expected number, got '%s'\n",arg1);
                        }
                    } else if(num>0xFFF) {
                        ok=0;
                        printf("Invalid argument for instruction 'JMP', number too large, max 0xFFF, got 0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0x1;
                        inst.section234=num;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("CALL",op)==0) { //CALL NNN
                    int num=parse_number(arg1);
                    if(num==-1) {
                        if(is_valid_label(arg1,len2)) {
                            label_data * lbl=find_label_name(labels,arg1);
                            if(lbl) {
                                instruction_data inst;
                                inst.section1=0x2;
                                inst.section234=lbl->position;
                                *head=add(*head,new_resolved_inst(inst,line));
                            } else { //label not defined yet, add
                                *head=add(*head,new_unresolved_jump(0x2,arg1,len2,line));
                            }
                        } else {
                            ok=0;
                            printf("Invalid argument for instruction 'CALL', expected number, got '%s'\n",arg1);
                        }
                    } else if(num>0xFFF) {
                        ok=0;
                        printf("Invalid argument for instruction 'CALL', number too large, max 0xFFF, got 0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0x2;
                        inst.section234=num;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("SKP",op)==0) { //SKP VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'SKP', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'SKP', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xE;
                        inst.section2=num;
                        inst.section34=0x9E;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("SNKP",op)==0) { //SNKP VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'SNKP', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'SNKP', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xE;
                        inst.section2=num;
                        inst.section34=0xA1;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("KEY",op)==0) { //KEY VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'KEY', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'KEY', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xF;
                        inst.section2=num;
                        inst.section34=0x0A;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("LDFNT",op)==0) { //LDFNT VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'LDFNT', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'LDFNT', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xF;
                        inst.section2=num;
                        inst.section34=0x29;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("BCD",op)==0) { //BCD VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'BCD', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'BCD', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xF;
                        inst.section2=num;
                        inst.section34=0x33;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("STR",op)==0) { //STR VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'STR', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'STR', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xF;
                        inst.section2=num;
                        inst.section34=0x55;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("LDR",op)==0) { //LDR VX
                    int num=get_register_index(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'LDR', expected register, got '%s'\n",arg1);
                    } else if(num>0xF) {
                        ok=0;
                        printf("Invalid argument for instruction 'LDR', invalid register, max VF, got V0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.section1=0xF;
                        inst.section2=num;
                        inst.section34=0x65;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else if(strcmp("DATA",op)==0) { //DATA NNNN
                    int num=parse_number(arg1);
                    if(num==-1) {
                        ok=0;
                        printf("Invalid argument for instruction 'DATA', expected number, got '%s'\n",arg1);
                    } else if(num>0xFFFF) {
                        ok=0;
                        printf("Invalid argument for instruction 'DATA', number too large, max 0xFFFF, got 0x%X\n",num);
                    } else {
                        instruction_data inst;
                        inst.whole=num;
                        *head=add(*head,new_resolved_inst(inst,line));
                    }
                } else {
                    ok=0;
                    printf("Invalid instruction '%s'\n",op);
                }
            } else {
                while(*str3==','||*str3==' '||*str3=='\t') { //skip whitespace
                    str3++;
                }
                const char * str4=str3;
                while(*str4!='\0'&&*str4!=','&&*str4!=' '&&*str4!='\t') { //find first whitespace or end of string
                    str4++;
                }
                int len3=str4-str3;
                char * arg2=strlncpymake(str3,len3+1);
                if(*str4=='\0') { //two argument instruction
                    if(strcmp("SE",op)==0) { //SE VX,NN | SE VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SE', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SE', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                int num=parse_number(arg2);
                                if(num==-1) {
                                    ok=0;
                                    printf("Invalid argument 2 for instruction 'SE', expected number or register, got '%s'\n",arg1);
                                } else if(num>0xFF) {
                                    ok=0;
                                    printf("Invalid argument for instruction 'SE', number too large, max 0xFF, got 0x%X\n",num);
                                } else { //SE VX,NN
                                    //number ok
                                    instruction_data inst;
                                    inst.section1=0x3;
                                    inst.section2=reg;
                                    inst.section34=num;
                                    *head=add(*head,new_resolved_inst(inst,line));
                                }
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SE', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //SE VX,VY
                                //register ok
                                instruction_data inst;
                                inst.section1=0x5;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x0;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("SNE",op)==0) { //SNE VX,NN | SNE VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SNE', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SNE', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                int num=parse_number(arg2);
                                if(num==-1) {
                                    ok=0;
                                    printf("Invalid argument 2 for instruction 'SNE', expected number or register, got '%s'\n",arg1);
                                } else if(num>0xFF) {
                                    ok=0;
                                    printf("Invalid argument for instruction 'SNE', number too large, max 0xFF, got 0x%X\n",num);
                                } else { //SNE VX,NN
                                    //number ok
                                    instruction_data inst;
                                    inst.section1=0x4;
                                    inst.section2=reg;
                                    inst.section34=num;
                                    *head=add(*head,new_resolved_inst(inst,line));
                                }
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SNE', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //SNE VX,VY
                                //register ok
                                instruction_data inst;
                                inst.section1=0x9;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x0;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("MOV",op)==0) { //MOV VX,NN | MOV VX,VY | MOV I,NNN | MOV VX,DT | MOV DT,VX | MOV ST,VX
                        if(strcmp("I",arg1)==0) {
                            int num=parse_number(arg2);
                            if(num==-1) {
                                if(is_valid_label(arg2,len3)) {
                                    label_data * lbl=find_label_name(labels,arg2);
                                    if(lbl) {
                                        instruction_data inst;
                                        inst.section1=0xA;
                                        inst.section234=lbl->position;
                                        *head=add(*head,new_resolved_inst(inst,line));
                                    } else { //label not defined yet, add
                                        *head=add(*head,new_unresolved_jump(0xA,arg2,len3,line));
                                    }
                                } else {
                                    ok=0;
                                    printf("Invalid argument 2 for instruction 'MOV', expected number, got '%s'\n",arg1);
                                }
                            } else if(num>0xFFF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'MOV', number too large, max 0xFFF, got 0x%X\n",num);
                            } else { //MOV I,NNN
                                instruction_data inst;
                                inst.section1=0xA;
                                inst.section234=num;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        } else if(strcmp("DT",arg1)==0) {
                            int reg=get_register_index(arg2);
                            if(reg==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'MOV', expected register, got '%s'\n",arg2);
                            } else if(reg>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'MOV', invalid register, max VF, got V0x%X\n",reg);
                            } else { //MOV DT,VX
                                instruction_data inst;
                                inst.section1=0xF;
                                inst.section2=reg;
                                inst.section34=0x15;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        } else if(strcmp("ST",arg1)==0) {
                            int reg=get_register_index(arg2);
                            if(reg==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'MOV', expected register, got '%s'\n",arg2);
                            } else if(reg>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'MOV', invalid register, max VF, got V0x%X\n",reg);
                            } else { //MOV ST,VX
                                instruction_data inst;
                                inst.section1=0xF;
                                inst.section2=reg;
                                inst.section34=0x18;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        } else {
                            int reg=get_register_index(arg1);
                            if(reg==-1) {
                                ok=0;
                                printf("Invalid argument 1 for instruction 'MOV', expected register, got '%s'\n",arg1);
                            } else if(reg>0xF) {
                                ok=0;
                                printf("Invalid argument 1 for instruction 'MOV', invalid register, max VF, got V0x%X\n",reg);
                            } else {
                                if(strcmp("DT",arg2)==0) { //MOV VX,DT
                                    instruction_data inst;
                                    inst.section1=0xF;
                                    inst.section2=reg;
                                    inst.section34=0x07;
                                    *head=add(*head,new_resolved_inst(inst,line));
                                } else {
                                    int reg2=get_register_index(arg2);
                                    if(reg2==-1) {
                                        int num=parse_number(arg2);
                                        if(num==-1) {
                                            ok=0;
                                            printf("Invalid argument 2 for instruction 'MOV', expected number or register, got '%s'\n",arg1);
                                        } else if(num>0xFF) {
                                            ok=0;
                                            printf("Invalid argument for instruction 'MOV', number too large, max 0xFF, got 0x%X\n",num);
                                        } else { //MOV VX,NN
                                            //number ok
                                            instruction_data inst;
                                            inst.section1=0x6;
                                            inst.section2=reg;
                                            inst.section34=num;
                                            *head=add(*head,new_resolved_inst(inst,line));
                                        }
                                    } else if(reg2>0xF) {
                                        ok=0;
                                        printf("Invalid argument 2 for instruction 'MOV', invalid register, max VF, got V0x%X\n",reg2);
                                    } else { //MOV VX,VY
                                        //register ok
                                        instruction_data inst;
                                        inst.section1=0x8;
                                        inst.section2=reg;
                                        inst.section3=reg2;
                                        inst.section4=0x0;
                                        *head=add(*head,new_resolved_inst(inst,line));
                                    }
                                }
                            }
                        }
                    } else if(strcmp("ADD",op)==0) { //ADD VX,NN | ADD VX,VY | ADD I,VX
                        if(strcmp("I",arg1)==0) {
                            int reg=get_register_index(arg2);
                            if(reg==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'ADD', expected register, got '%s'\n",arg2);
                            } else if(reg>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'ADD', invalid register, max VF, got V0x%X\n",reg);
                            } else { //ADD I,VX
                                instruction_data inst;
                                inst.section1=0xF;
                                inst.section2=reg;
                                inst.section34=0x29;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        } else {
                            int reg=get_register_index(arg1);
                            if(reg==-1) {
                                ok=0;
                                printf("Invalid argument 1 for instruction 'ADD', expected register, got '%s'\n",arg1);
                            } else if(reg>0xF) {
                                ok=0;
                                printf("Invalid argument 1 for instruction 'ADD', invalid register, max VF, got V0x%X\n",reg);
                            } else {
                                int reg2=get_register_index(arg2);
                                if(reg2==-1) {
                                    int num=parse_number(arg2);
                                    if(num==-1) {
                                        ok=0;
                                        printf("Invalid argument 2 for instruction 'ADD', expected number or register, got '%s'\n",arg1);
                                    } else if(num>0xFF) {
                                        ok=0;
                                        printf("Invalid argument for instruction 'ADD', number too large, max 0xFF, got 0x%X\n",num);
                                    } else { //ADD VX,NN
                                        //number ok
                                        instruction_data inst;
                                        inst.section1=0x7;
                                        inst.section2=reg;
                                        inst.section34=num;
                                        *head=add(*head,new_resolved_inst(inst,line));
                                    }
                                } else if(reg2>0xF) {
                                    ok=0;
                                    printf("Invalid argument 2 for instruction 'ADD', invalid register, max VF, got V0x%X\n",reg2);
                                } else { //ADD VX,VY
                                    //register ok
                                    instruction_data inst;
                                    inst.section1=0x8;
                                    inst.section2=reg;
                                    inst.section3=reg2;
                                    inst.section4=0x4;
                                    *head=add(*head,new_resolved_inst(inst,line));
                                }
                            }
                        }
                    } else if(strcmp("OR",op)==0) { //OR VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'OR', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'OR', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'OR', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'OR', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //OR VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x1;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("AND",op)==0) { //AND VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'AND', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'AND', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'AND', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'AND', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //AND VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x2;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("XOR",op)==0) { //XOR VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'XOR', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'XOR', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'XOR', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'XOR', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //XOR VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x3;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("SUB",op)==0) { //SUB VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SUB', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SUB', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SUB', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SUB', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //SUB VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x5;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("SHR",op)==0) { //SHR VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SHR', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SHR', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SHR', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SHR', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //SHR VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x6;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("RSB",op)==0) { //RSB VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'RSB', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'RSB', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'RSB', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'RSB', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //RSB VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0x7;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("SHL",op)==0) { //SHL VX,VY
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SHL', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'SHL', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int reg2=get_register_index(arg2);
                            if(reg2==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SHL', expected register, got '%s'\n",arg1);
                            } else if(reg2>0xF) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'SHL', invalid register, max VF, got V0x%X\n",reg2);
                            } else { //SHL VX,VY
                                instruction_data inst;
                                inst.section1=0x8;
                                inst.section2=reg;
                                inst.section3=reg2;
                                inst.section4=0xE;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else if(strcmp("RND",op)==0) { //RND VX,NN
                        int reg=get_register_index(arg1);
                        if(reg==-1) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'RND', expected register, got '%s'\n",arg1);
                        } else if(reg>0xF) {
                            ok=0;
                            printf("Invalid argument 1 for instruction 'RND', invalid register, max VF, got V0x%X\n",reg);
                        } else {
                            int num=parse_number(arg2);
                            if(num==-1) {
                                ok=0;
                                printf("Invalid argument 2 for instruction 'RND', expected number, got '%s'\n",arg1);
                            } else if(num>0xFF) {
                                ok=0;
                                printf("Invalid argument for instruction 'RND', number too large, max 0xFF, got 0x%X\n",num);
                            } else { //RND VX,NN
                                instruction_data inst;
                                inst.section1=0xC;
                                inst.section2=reg;
                                inst.section34=num;
                                *head=add(*head,new_resolved_inst(inst,line));
                            }
                        }
                    } else {
                        ok=0;
                        printf("Invalid instruction '%s'\n",op);
                    }
                } else {
                    while(*str4==','||*str4==' '||*str4=='\t') { //skip whitespace
                        str4++;
                    }
                    const char * strend=str4;
                    while(*strend!='\0'&&*strend!=','&&*strend!=' '&&*strend!='\t') { //find first whitespace or end of string
                        strend++;
                    }
                    int len4=strend-str4;
                    char * arg3=strlncpymake(str4,len4+1);
                    if(*strend=='\0') { //three argument instruction
                        if(strcmp("DRW",op)==0) {
                            int reg=get_register_index(arg1);
                            if(reg==-1) {
                                ok=0;
                                printf("Invalid argument 1 for instruction 'DRW', expected register, got '%s'\n",arg1);
                            } else if(reg>15) {
                                ok=0;
                                printf("Invalid argument 1 for instruction 'DRW', invalid register, max VF, got V0x%X\n",reg);
                            } else {
                                int reg2=get_register_index(arg2);
                                if(reg2==-1) {
                                    ok=0;
                                    printf("Invalid argument 2 for instruction 'DRW', expected register, got '%s'\n",arg1);
                                } else if(reg2>15) {
                                    ok=0;
                                    printf("Invalid argument 2 for instruction 'DRW', invalid register, max VF, got V0x%X\n",reg2);
                                } else {
                                    int num=parse_number(arg3);
                                    if(num==-1) {
                                        ok=0;
                                        printf("Invalid argument 3 for instruction 'DRW', expected number, got '%s'\n",arg3);
                                    } else if(num>0xF) {
                                        ok=0;
                                        printf("Invalid argument 3 for instruction 'DRW', number too large, max 0xF, got 0x%X\n",num);
                                    } else { //DRW VX,VY,N
                                        instruction_data inst;
                                        inst.section1=0xD;
                                        inst.section2=reg;
                                        inst.section3=reg2;
                                        inst.section4=num;
                                        *head=add(*head,new_resolved_inst(inst,line));
                                    }
                                }
                            }
                        } else {
                            ok=0;
                            printf("Invalid instruction '%s'\n",op);
                        }
                    } else {
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
    }else{
        return -1;
    }
}

static uint8_t * parse_lines(line_array arr,size_t * size) { //return unterminated array with binary code, store array length in size
    int ok=1;
    label_data * labels=NULL;
    line_data * start=NULL;
    line_data * head=NULL;
    unresolved_instruction * un_start=NULL;
    unresolved_instruction * un_head=NULL;
    int pos=0x200;
    for(int i=0; i<arr.lines; i++) {
        if(parse_label(arr.data[i],i,&labels)) { //try to parse label line
            line_data * temp=calloc(1,sizeof(line_data));
            labels->parent=temp;
            labels->position=pos;
            temp->type=LINE_LABEL;
            temp->label=labels;
            temp->line=i;
            temp->pos=pos;
            if(head) {
                head->next=temp;
                head=temp;
            } else {
                head=start=temp;
            }
        } else{
            int r=parse_line(arr.data[i],i,labels,&head);//try to parse code line
            if(r==-1){
                continue;
            }else if(r==0){
                ok=0;
            }else{
                if(!start) {
                    start=head;
                }
                head->pos=pos;
                pos+=2;
                head->unresolved->parent=head;
                if(head->type==LINE_UNRESOLVED_INSTRUCTION) {
                    if(un_start) {
                        un_head->next=head->unresolved;
                        un_head=head->unresolved;
                    } else {
                        un_head=un_start=head->unresolved;
                    }
                }
            }
        }
    }
    while(un_start!=NULL) {
        if(un_start->type==UNRESOLVED_LABEL_JUMP) {
            label_data *label=find_label_name(labels,un_start->label_jump->label);
            if(label) {
                line_data * dt=un_start->parent;
                dt->type=LINE_RESOLVED_INSTRUCTION;
                resolved_instruction * temp=calloc(1,sizeof(resolved_instruction));
                temp->code.section1=un_start->label_jump->section1;
                temp->code.section234=label->position;
                dt->resolved=temp;
            } else {
                ok=0;
                printf("Unknown label %s in line %d\n",un_start->label_jump->label,un_start->parent->line);
            }
            free(un_start->label_jump->label);
            free(un_start->label_jump);
        } else {
            ok=0;
            printf("Unknown unresolved instruction in line %d\n",un_start->parent->line);
            un_start=un_start->next;
            continue;
        }
        unresolved_instruction * temp=un_start;
        un_start=un_start->next;
        free(temp);
    }
    if(ok) {
        int len=pos-0x200;
        if(len>0) {
            uint8_t * data=calloc(len,sizeof(uint8_t));
            int cur=0;
            while(start!=NULL) {
                if(start->type==LINE_RESOLVED_INSTRUCTION) {
                    data[cur]=start->resolved->code.section12;
                    data[cur+1]=start->resolved->code.section34;
                    cur+=2;
                    free(start->resolved);
                } else if(start->type==LINE_UNRESOLVED_INSTRUCTION) {
                    ok=0;
                    printf("Unresolved instruction in line %d\n",un_start->parent->line);
                    free(start->unresolved);
                } else if(start->type==LINE_LABEL) {
                    free(start->label->name);
                    free(start->label);
                } else {
                    ok=0;
                    printf("Unknown line data in line %d\n",un_start->parent->line);
                }
                line_data * temp=start;
                start=start->next;
                free(temp);
            }
            *size=len;
            return data;
        } else {
            printf("No valid instructions to be assembled\n");
            return NULL;
        }
    } else {
        return NULL;
    }
}

int assemble(const char * in,const char * out) {
    printf("Reading '%s'\n",in);
    line_array arr=read_file_into_line_array(in);
    if(arr.data) {
        for(int i=0; i<arr.lines; i++) {
            char * temp=strip_whitespace_comments(arr.data[i]);
            if(temp!=arr.data[i]) {
                free(arr.data[i]);
                arr.data[i]=temp;
            }
        }
        FILE * f=fopen(out,"wb");
        if(f) {
            size_t len;
            printf("Assembling '%s'\n",in);
            uint8_t * data=parse_lines(arr,&len);
            if(data) {
                printf("Writing output to '%s'\n",out);
                fwrite(data,sizeof(uint8_t),len,f);
                free(data);
            } else {
                printf("Could not assemble %s\n",in);
            }
            fclose(f);
        } else {
            printf("Could not write to file %s\n",out);
        }
        free_line_array(arr);
        return 0;
    } else {
        printf("Could not open file %s\n",in);
        return 1;
    }
}

static instruction_data as_instr(uint8_t start,uint8_t end){
    instruction_data temp;
    temp.section12=start;
    temp.section34=end;
    return temp;
}

int disassemble(const char * in,const char * out) {
    //printf("Disassembler not implemented\n");
    FILE * f=fopen(in,"rb");
    if(f){
        fseek(f,0,SEEK_END);
        int len=ftell(f);
        if(len%2!=0){
            printf("ROM has odd number of bytes");
            fclose(f);
            return 1;
        }
        uint8_t * instructions=calloc(len,sizeof(uint8_t));
        fseek(f,0,SEEK_SET);
        if(fread(instructions,sizeof(uint8_t),len,f)!=len){
            printf("could not read all instructions");
            fclose(f);
            free(instructions);
            return 1;
        }
        fclose(f);
        f=fopen(out,"w");
        if(f){
            label_data * labels=NULL;
            for(int i=0;i<len;i+=2){//scan for jumps/labels
                instruction_data instr=as_instr(instructions[i],instructions[i+1]);
                switch(disassemble_instruction(instr).type){
                case JMP:
                case JMP_V0:
                case CALL:
                case MOV_I:
                    if(instr.section234<=(len+0x200)&&!find_label_pos(labels,instr.section234)){
                        labels=add_label(labels,instr.section234,strlnprintfmake("generated_label_0x%04X",instr.section234));
                        labels->position=instr.section234;
                    }
                default:
                    break;
                }
            }
            char buffer[64];
            label_data * temp_label=NULL;
            int i;
            for(i=0;i<len;i+=2){//write to file
                if((temp_label=find_label_pos(labels,i+0x200))!=NULL){
                    fprintf(f,"%s:\n",temp_label->name);
                }
                instruction_data instr=as_instr(instructions[i],instructions[i+1]);
                disassembled_instruction das=disassemble_instruction(instr);
                if(instr.section234<=(len+0x200)){
                    switch(das.type){
                    case JMP:
                        snprintf(buffer,64,"JMP generated_label_0x%04X",instr.section234);
                        break;
                    case JMP_V0:
                        snprintf(buffer,64,"JMP V0, generated_label_0x%04X",instr.section234);
                        break;
                    case CALL:
                        snprintf(buffer,64,"CALL generated_label_0x%04X",instr.section234);
                        break;
                    case MOV_I:
                        snprintf(buffer,64,"MOV I, generated_label_0x%04X",instr.section234);
                        break;
                    default:
                        get_instruction_str(das,buffer,64);
                    }
                }else{
                    get_instruction_str(das,buffer,64);
                }
                fprintf(f,"%s;0x%04X\n",buffer,i+0x200);
            }
            if((temp_label=find_label_pos(labels,i+0x200))!=NULL){
                fprintf(f,"%s:\n;0x%04X\n",temp_label->name,i+0x200);
            }
            fclose(f);
            return 0;
        }else{
            printf("Could not open file %s\n",out);
            return 1;
        }
    }else{
        printf("Could not open file %s\n",in);
        return 1;
    }
}
