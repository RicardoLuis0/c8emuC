#include <stdio.h>
#include <string.h>
#include "assemble.h"

void print_usage(){
    printf("usage:\n  c8asm [-disassemble] <input> <output>");
}

int main(int argc,char ** argv){
    if(argc<3){
        printf("Too few arguments\n");
    }else if(argc==3){
        //has file parameter, run emulator
        return assemble(argv[1],argv[2]);
    }else if(argc==4){
        if(strcmp(argv[1],"-disassemble")==0){
            //has debug parameter, run emulator
            return disassemble(argv[2],argv[3]);
        }else{
            printf("Invalid argument\n");
        }
    }else{
        printf("Too many arguments\n");
    }
    print_usage();
    return 0;
}
