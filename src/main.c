#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include <SDL2/SDL.h>

#define VRAMPOS(x,y) ((y)*64+(x))

void draw(SDL_Renderer * renderer,CPU_info * cpu){
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);
    SDL_Rect rect={.x=0,.y=0,.w=10,.h=10};
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    for(int y=0;y<32;y++){
        for(int x=0;x<64;x++){
            if(cpu->VRAM[VRAMPOS(x,y)]!=0){
                rect.x=x*10;
                rect.y=y*10;
                SDL_RenderFillRect(renderer,&rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

int get_key(int32_t sym){
    switch(sym){
    case SDLK_1:
        return 0;
    case SDLK_2:
        return 1;
    case SDLK_3:
        return 2;
    case SDLK_4:
        return 3;
    case SDLK_q:
        return 4;
    case SDLK_w:
        return 5;
    case SDLK_e:
        return 6;
    case SDLK_r:
        return 7;
    case SDLK_a:
        return 8;
    case SDLK_s:
        return 9;
    case SDLK_d:
        return 10;
    case SDLK_f:
        return 11;
    case SDLK_z:
        return 12;
    case SDLK_x:
        return 13;
    case SDLK_c:
        return 14;
    case SDLK_v:
        return 15;
    default:
        return -1;
    }
}

#undef VRAMPOS

void print_usage(){
    printf("usage:\n  c8emu <ROM>");
}



int main(int argc,char ** argv){
    int target_fps=10;//frames per second, affects counters
    int target_ops=10;//operations per second (speed of processor in hz)
    int frame_time=1000/target_fps;
    int cpu_time=1000/target_ops;
    if(argc<2){
        printf("Too few arguments\n");
        print_usage();
    }else if(argc>2){
        printf("Too many arguments\n");
        print_usage();
    }else{
        //has file parameter, run emulator
        CPU_info cpu;
        printf("frame time:%d\ncpu time:%d\n",frame_time,cpu_time);
        init_cpu(&cpu);
        if(load_program(&cpu,argv[1])){
            if(SDL_Init(SDL_INIT_VIDEO)!=0){
                printf("Error.\n >Error while Initializing SDL: %s\n",SDL_GetError());
                return 0;
            }
            SDL_Renderer * renderer;
            SDL_Window * window;
            SDL_CreateWindowAndRenderer(640,320,0,&window,&renderer);
            SDL_RenderPresent(renderer);
            while(1){
                if((SDL_GetTicks()%cpu_time)==0){
                    for(SDL_Event e;SDL_PollEvent(&e);){
                        switch(e.type){
                        case SDL_QUIT:
                            goto endloop;
                        case SDL_KEYDOWN:
                            if(e.key.keysym.sym==SDLK_ESCAPE)goto endloop;
                            keydown(&cpu,get_key(e.key.keysym.sym));
                            break;
                        case SDL_KEYUP:
                            keyup(&cpu,get_key(e.key.keysym.sym));
                            break;
                        default:
                            break;
                        }
                    }
                    execute_instruction(&cpu);
                }
                if((SDL_GetTicks()%frame_time)==0){
                    draw(renderer,&cpu);
                    delay_tick(&cpu);
                }
            }
        endloop:
            SDL_Quit();
        }else{
            printf("Failed to load ROM (inexistent or too large)\n");
        }
    }
    return 0;
}
