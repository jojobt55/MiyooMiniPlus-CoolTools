#pragma once
#include <stdint.h>

enum {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
    BTN_A, BTN_B, BTN_X, BTN_Y,
    BTN_L1, BTN_R1, BTN_L2, BTN_R2,
    BTN_SELECT, BTN_START, BTN_MENU,
    BTN_COUNT
};

/* Platform layer: device framebuffer + evdev, or an SDL2 window on desktop. */
int plat_init(void);
void plat_present(const uint32_t *px); /* SCR_W x SCR_H ARGB */
/* returns 1 and fills btn/down when an event is available; btn = -1 for quit */
int plat_poll(int *btn, int *down);
void plat_quit(void);
