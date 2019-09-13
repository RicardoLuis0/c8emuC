#include "util.h"
#include <string.h>
#include <stdarg.h>

char * strlncpy(char *dst, const char *src, size_t size){
    strncpy(dst,src,size-1);
    dst[size-1]=0;
    return dst;
}

char * strlncpymake(const char *src, size_t size){
    return strlncpy(calloc(size,sizeof(char)),src,size);
}

char * strlnprintfmake(const char * fmt,...){
    va_list args;
    va_start(args,fmt);
    size_t len=vsnprintf(NULL,0,fmt,args);
    char * buf=calloc(len+1,sizeof(char));
    vsnprintf(buf,len+1,fmt,args);
    return buf;
}

int parse_number(const char * str){
    if(str[0]>='0'&&str[0]<='9'){
        char * endptr;
        int num=-1;
        if(str[0]=='0'&&str[1]=='x'){//if starts with 0x, parse as base16(hexadecimal), else parse as base10(decimal)
            num=strtol(str+2,&endptr,16);
        }else{
            num=strtol(str,&endptr,10);
        }
        if(*endptr=='\0'){
            return num;
        }//if there's non-numbers in the string, fail
    }
    return -1;//fail
}
