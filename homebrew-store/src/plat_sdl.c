/* Desktop preview: SDL2 window, keyboard mapped to Miyoo buttons. */
#include "plat.h"
#include "gfx.h"

#include <SDL.h>
#include <stdio.h>

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;

int plat_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return -1; }
    win = SDL_CreateWindow("Homebrew Store (Miyoo preview)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           SCR_W * 2, SCR_H * 2, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(ren, SCR_W, SCR_H);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCR_W, SCR_H);
    printf("Keys: arrows=D-pad  Z/Space=A  X=B  A=X  S=Y  Q=L1  W=R1  1=L2  2=R2\n"
           "      RShift/Backspace=Select  Enter=Start  Esc=Menu\n");
    return 0;
}

void plat_present(const uint32_t *px)
{
    SDL_UpdateTexture(tex, NULL, px, SCR_W * 4);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

static int map_key(SDL_Keycode k)
{
    switch (k) {
    case SDLK_UP: return BTN_UP;
    case SDLK_DOWN: return BTN_DOWN;
    case SDLK_LEFT: return BTN_LEFT;
    case SDLK_RIGHT: return BTN_RIGHT;
    case SDLK_z: case SDLK_SPACE: return BTN_A;
    case SDLK_x: return BTN_B;
    case SDLK_a: return BTN_X;
    case SDLK_s: return BTN_Y;
    case SDLK_q: return BTN_L1;
    case SDLK_w: return BTN_R1;
    case SDLK_1: return BTN_L2;
    case SDLK_2: return BTN_R2;
    case SDLK_RSHIFT: case SDLK_BACKSPACE: return BTN_SELECT;
    case SDLK_RETURN: return BTN_START;
    case SDLK_ESCAPE: return BTN_MENU;
    }
    return -2;
}

int plat_poll(int *btn, int *down)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { *btn = -1; *down = 1; return 1; }
        if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && !e.key.repeat) {
            int b = map_key(e.key.keysym.sym);
            if (b < 0) continue;
            *btn = b;
            *down = e.type == SDL_KEYDOWN;
            return 1;
        }
    }
    return 0;
}

void plat_quit(void)
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
