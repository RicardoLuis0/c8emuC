#include "io_control.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include "cpu_info.h"

#define VRAMPOS(x,y) ((y)*64+(x))

static SDL_Renderer * renderer;
static SDL_Window * window;

static int get_key(int32_t sym){
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

int check_time(int msdelay){
    return (SDL_GetTicks()%msdelay)==0;
}

int init_io(){
    if(SDL_Init(SDL_INIT_VIDEO)!=0){
        printf("Error.\n >Error while Initializing SDL: %s\n",SDL_GetError());
        return 1;
    }
    SDL_CreateWindowAndRenderer(640,320,0,&window,&renderer);
    SDL_RenderPresent(renderer);
    return 0;
}

void draw(CPU_info * cpu){
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

int poll_io(CPU_info * cpu){
    for(SDL_Event e;SDL_PollEvent(&e);){
        switch(e.type){
        case SDL_QUIT:
            return 1;
        case SDL_KEYDOWN:
            if(e.key.keysym.sym==SDLK_ESCAPE)return 1;
            keydown(cpu,get_key(e.key.keysym.sym));
            break;
        case SDL_KEYUP:
            keyup(cpu,get_key(e.key.keysym.sym));
            break;
        default:
            break;
        }
    }
    return 0;
}

int poll_noio(){
    for(SDL_Event e;SDL_PollEvent(&e);){
        if(e.type==SDL_QUIT){
            return 1;
        }
    }
    return 0;
}

void exit_io(){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
