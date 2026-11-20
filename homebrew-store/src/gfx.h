#pragma once
#include <stdint.h>

#define SCR_W 640
#define SCR_H 480

extern uint32_t g_px[SCR_W * SCR_H]; /* ARGB8888 canvas */

typedef struct {
    int w, h;
    uint32_t *px;
} Image;

enum { F_REG, F_BOLD, F_PIXEL, F_COUNT };

int gfx_init(const char *resdir);
void gfx_clear(uint32_t c);
void gfx_clip(int x, int y, int w, int h);
void gfx_noclip(void);

void gfx_rect(int x, int y, int w, int h, uint32_t c);
void gfx_rrect(int x, int y, int w, int h, int r, uint32_t c);
void gfx_circle(float cx, float cy, float r, uint32_t c);
void gfx_line(float x0, float y0, float x1, float y1, float th, uint32_t c);
void gfx_image(const Image *im, int x, int y);

int gfx_line_h(int font, int size);
int gfx_text_w(int font, int size, const char *s);
int gfx_text(int font, int size, int x, int y, uint32_t c, const char *s);
int gfx_text_fit(int font, int size, int x, int y, int maxw, uint32_t c, const char *s);
int gfx_text_wrap(int font, int size, int x, int y, int maxw, int lineh, int maxlines, int skip,
                  uint32_t c, const char *s);

Image *img_new(int w, int h);
Image *img_load_fit(const char *path, int bw, int bh, uint32_t bg);
void img_free(Image *im);
int img_save_png(const Image *im, const char *path);
int canvas_save_png(const char *path);
