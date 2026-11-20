#include "gfx.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_GIF
#include "../vendor/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/stb_image_write.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb_truetype.h"

uint32_t g_px[SCR_W * SCR_H];

static int cx0 = 0, cy0 = 0, cx1 = SCR_W, cy1 = SCR_H;

void gfx_clip(int x, int y, int w, int h)
{
    cx0 = x < 0 ? 0 : x;
    cy0 = y < 0 ? 0 : y;
    cx1 = x + w > SCR_W ? SCR_W : x + w;
    cy1 = y + h > SCR_H ? SCR_H : y + h;
}

void gfx_noclip(void) { cx0 = 0, cy0 = 0, cx1 = SCR_W, cy1 = SCR_H; }

static inline uint32_t blend(uint32_t d, uint32_t s, unsigned a)
{
    if (a >= 255) return s | 0xFF000000u;
    if (a == 0) return d;
    unsigned ia = 255 - a;
    unsigned r = (((s >> 16) & 255) * a + ((d >> 16) & 255) * ia + 127) / 255;
    unsigned g = (((s >> 8) & 255) * a + ((d >> 8) & 255) * ia + 127) / 255;
    unsigned b = ((s & 255) * a + (d & 255) * ia + 127) / 255;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static inline void plot(int x, int y, uint32_t c, unsigned cov)
{
    if (x < cx0 || y < cy0 || x >= cx1 || y >= cy1) return;
    unsigned a = ((c >> 24) * cov + 127) / 255;
    uint32_t *p = &g_px[y * SCR_W + x];
    *p = blend(*p, c, a);
}

void gfx_clear(uint32_t c)
{
    for (int i = 0; i < SCR_W * SCR_H; i++) g_px[i] = c | 0xFF000000u;
}

void gfx_rect(int x, int y, int w, int h, uint32_t c)
{
    int x0 = x < cx0 ? cx0 : x, y0 = y < cy0 ? cy0 : y;
    int x1 = x + w > cx1 ? cx1 : x + w, y1 = y + h > cy1 ? cy1 : y + h;
    unsigned a = c >> 24;
    for (int yy = y0; yy < y1; yy++) {
        uint32_t *row = &g_px[yy * SCR_W];
        if (a == 255)
            for (int xx = x0; xx < x1; xx++) row[xx] = c;
        else
            for (int xx = x0; xx < x1; xx++) row[xx] = blend(row[xx], c, a);
    }
}

/* coverage of a pixel against a circle edge, cheap AA */
static unsigned cov_circle(float dx, float dy, float r)
{
    float d = sqrtf(dx * dx + dy * dy);
    float v = r - d + 0.5f;
    if (v <= 0) return 0;
    if (v >= 1) return 255;
    return (unsigned)(v * 255);
}

void gfx_rrect(int x, int y, int w, int h, int r, uint32_t c)
{
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    if (r <= 0) { gfx_rect(x, y, w, h, c); return; }
    gfx_rect(x, y + r, w, h - 2 * r, c);
    gfx_rect(x + r, y, w - 2 * r, r, c);
    gfx_rect(x + r, y + h - r, w - 2 * r, r, c);
    for (int j = 0; j < r; j++)
        for (int i = 0; i < r; i++) {
            float dx = r - i - 0.5f, dy = r - j - 0.5f;
            unsigned cv = cov_circle(dx, dy, (float)r);
            if (!cv) continue;
            plot(x + i, y + j, c, cv);
            plot(x + w - 1 - i, y + j, c, cv);
            plot(x + i, y + h - 1 - j, c, cv);
            plot(x + w - 1 - i, y + h - 1 - j, c, cv);
        }
}

void gfx_circle(float cx, float cy, float r, uint32_t c)
{
    int x0 = (int)floorf(cx - r - 1), x1 = (int)ceilf(cx + r + 1);
    int y0 = (int)floorf(cy - r - 1), y1 = (int)ceilf(cy + r + 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            unsigned cv = cov_circle(x + 0.5f - cx, y + 0.5f - cy, r);
            if (cv) plot(x, y, c, cv);
        }
}

/* distance-to-segment AA line with thickness */
void gfx_line(float x0, float y0, float x1, float y1, float th, uint32_t c)
{
    float hr = th / 2;
    int bx0 = (int)floorf(fminf(x0, x1) - hr - 1), bx1 = (int)ceilf(fmaxf(x0, x1) + hr + 1);
    int by0 = (int)floorf(fminf(y0, y1) - hr - 1), by1 = (int)ceilf(fmaxf(y0, y1) + hr + 1);
    float vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy;
    for (int y = by0; y <= by1; y++)
        for (int x = bx0; x <= bx1; x++) {
            float px = x + 0.5f - x0, py = y + 0.5f - y0;
            float t = len2 > 0 ? (px * vx + py * vy) / len2 : 0;
            t = t < 0 ? 0 : t > 1 ? 1 : t;
            float dx = px - t * vx, dy = py - t * vy;
            float v = hr - sqrtf(dx * dx + dy * dy) + 0.5f;
            if (v <= 0) continue;
            plot(x, y, c, v >= 1 ? 255 : (unsigned)(v * 255));
        }
}

void gfx_image(const Image *im, int x, int y)
{
    if (!im) return;
    for (int j = 0; j < im->h; j++) {
        int yy = y + j;
        if (yy < cy0 || yy >= cy1) continue;
        for (int i = 0; i < im->w; i++) {
            int xx = x + i;
            if (xx < cx0 || xx >= cx1) continue;
            g_px[yy * SCR_W + xx] = im->px[j * im->w + i];
        }
    }
}

/* ---------------- fonts ---------------- */

typedef struct {
    unsigned char *data;
    stbtt_fontinfo info;
    int ok;
    int pixel; /* threshold alpha for crisp pixel font */
} Font;

static Font fonts[F_COUNT];

typedef struct {
    int font, size, cp; /* key; cp == 0 means empty slot */
    int w, h, xoff, yoff, adv;
    unsigned char *bmp;
} Glyph;

#define GCAP 4096
static Glyph gcache[GCAP];
static int gcount;

static unsigned char *read_file(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *b = malloc(n + 1);
    if (b && fread(b, 1, n, f) != (size_t)n) { free(b); b = NULL; }
    fclose(f);
    if (len) *len = n;
    return b;
}

static int load_font(int id, const char *resdir, const char *name, int pixel)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s", resdir, name);
    fonts[id].data = read_file(path, NULL);
    if (!fonts[id].data) return -1;
    if (!stbtt_InitFont(&fonts[id].info, fonts[id].data, stbtt_GetFontOffsetForIndex(fonts[id].data, 0)))
        return -1;
    fonts[id].ok = 1;
    fonts[id].pixel = pixel;
    return 0;
}

int gfx_init(const char *resdir)
{
    if (load_font(F_REG, resdir, "Atkinson-Regular.ttf", 0)) return -1;
    if (load_font(F_BOLD, resdir, "Atkinson-Bold.ttf", 0)) return -1;
    if (load_font(F_PIXEL, resdir, "PressStart2P.ttf", 1)) return -1;
    return 0;
}

static void gcache_reset(void)
{
    for (int i = 0; i < GCAP; i++) {
        free(gcache[i].bmp);
        gcache[i].bmp = NULL;
        gcache[i].cp = 0;
    }
    gcount = 0;
}

static Glyph *glyph(int font, int size, int cp)
{
    unsigned h = ((unsigned)cp * 2654435761u) ^ ((unsigned)size * 40503u) ^ ((unsigned)font * 977u);
    for (int probe = 0; probe < GCAP; probe++) {
        Glyph *g = &gcache[(h + probe) & (GCAP - 1)];
        if (g->cp == 0) break;
        if (g->cp == cp && g->size == size && g->font == font) return g;
    }
    if (gcount > GCAP * 3 / 4) gcache_reset();
    Font *f = &fonts[font];
    float sc = stbtt_ScaleForPixelHeight(&f->info, (float)size);
    int gi = stbtt_FindGlyphIndex(&f->info, cp);
    if (!gi && cp != '?') gi = stbtt_FindGlyphIndex(&f->info, '?');
    Glyph *g = NULL;
    for (int probe = 0; probe < GCAP; probe++) {
        g = &gcache[(h + probe) & (GCAP - 1)];
        if (g->cp == 0) break;
    }
    int adv, lsb;
    stbtt_GetGlyphHMetrics(&f->info, gi, &adv, &lsb);
    g->font = font, g->size = size, g->cp = cp;
    g->adv = (int)lroundf(adv * sc);
    g->bmp = stbtt_GetGlyphBitmap(&f->info, sc, sc, gi, &g->w, &g->h, &g->xoff, &g->yoff);
    if (g->bmp && f->pixel)
        for (int i = 0; i < g->w * g->h; i++) g->bmp[i] = g->bmp[i] > 100 ? 255 : 0;
    gcount++;
    return g;
}

static int ascent_px(int font, int size)
{
    int asc, desc, gap;
    stbtt_GetFontVMetrics(&fonts[font].info, &asc, &desc, &gap);
    return (int)lroundf(asc * stbtt_ScaleForPixelHeight(&fonts[font].info, (float)size));
}

int gfx_line_h(int font, int size)
{
    int asc, desc, gap;
    stbtt_GetFontVMetrics(&fonts[font].info, &asc, &desc, &gap);
    float sc = stbtt_ScaleForPixelHeight(&fonts[font].info, (float)size);
    return (int)lroundf((asc - desc + gap) * sc);
}

/* decode one UTF-8 codepoint, advancing *s */
static int utf8_next(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    int c = p[0], n = 1;
    if (c < 0x80) n = 1;
    else if ((c & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) c = ((c & 0x1F) << 6) | (p[1] & 0x3F), n = 2;
    else if ((c & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80)
        c = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F), n = 3;
    else if ((c & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80)
        c = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F), n = 4;
    else c = '?', n = 1;
    *s += n;
    return c;
}

static int text_n(int font, int size, int x, int y, uint32_t c, const char *s, const char *end, int draw)
{
    int base = y + ascent_px(font, size);
    int pen = x;
    while (*s && (!end || s < end)) {
        int cp = utf8_next(&s);
        if (cp == '\n' || cp == '\r') continue;
        if (cp == '\t') cp = ' ';
        Glyph *g = glyph(font, size, cp);
        if (draw && g->bmp) {
            int gx = pen + g->xoff, gy = base + g->yoff;
            for (int j = 0; j < g->h; j++)
                for (int i = 0; i < g->w; i++) {
                    unsigned a = g->bmp[j * g->w + i];
                    if (a) plot(gx + i, gy + j, c, a);
                }
        }
        pen += g->adv;
    }
    return pen - x;
}

int gfx_text_w(int font, int size, const char *s) { return text_n(font, size, 0, 0, 0, s, NULL, 0); }

int gfx_text(int font, int size, int x, int y, uint32_t c, const char *s)
{
    return text_n(font, size, x, y, c, s, NULL, 1);
}

int gfx_text_fit(int font, int size, int x, int y, int maxw, uint32_t c, const char *s)
{
    int w = gfx_text_w(font, size, s);
    if (w <= maxw) return gfx_text(font, size, x, y, c, s);
    int ew = gfx_text_w(font, size, "...");
    const char *p = s, *last = s;
    int acc = 0;
    while (*p) {
        const char *q = p;
        int cp = utf8_next(&q);
        if (cp != '\n' && cp != '\r') acc += glyph(font, size, cp == '\t' ? ' ' : cp)->adv;
        if (acc + ew > maxw) break;
        last = q;
        p = q;
    }
    while (last > s && last[-1] == ' ') last--;
    int tw = text_n(font, size, x, y, c, s, last, 1);
    return tw + gfx_text(font, size, x + tw, y, c, "...");
}

/* Word-wrap. Draws lines [skip, skip+maxlines). Returns total line count. */
int gfx_text_wrap(int font, int size, int x, int y, int maxw, int lineh, int maxlines, int skip,
                  uint32_t c, const char *s)
{
    int line = 0;
    const char *p = s;
    while (*p) {
        /* find the longest prefix of the current paragraph that fits */
        const char *ls = p, *brk = NULL, *q = p;
        int lw = 0;
        while (*q && *q != '\n') {
            const char *nq = q;
            int cp = utf8_next(&nq);
            if (cp == '\r') { q = nq; continue; }
            lw += glyph(font, size, cp == '\t' ? ' ' : cp)->adv;
            if (lw > maxw) break;
            if (*q == ' ') brk = q;
            q = nq;
        }
        const char *le;
        if (!*q || *q == '\n') le = q;
        else if (brk && brk > ls) le = brk;
        else le = q > ls ? q : q + 1; /* hard break inside a long word */

        int vis = line - skip;
        if (vis >= 0 && vis < maxlines) {
            int more = *le && !(vis < maxlines - 1);
            /* last visible line with more text after it: ellipsize */
            if (more) {
                char tmp[1024];
                int n = (int)(le - ls) < 1000 ? (int)(le - ls) : 1000;
                memcpy(tmp, ls, n);
                strcpy(tmp + n, " ...");
                gfx_text_fit(font, size, x, y + vis * lineh, maxw, c, tmp);
            } else {
                text_n(font, size, x, y + vis * lineh, c, ls, le, 1);
            }
        }
        line++;
        p = le;
        if (*p == '\n') p++;
        else while (*p == ' ') p++;
    }
    return line;
}

/* ---------------- images ---------------- */

Image *img_new(int w, int h)
{
    Image *im = malloc(sizeof *im);
    if (!im) return NULL;
    im->w = w, im->h = h;
    im->px = calloc((size_t)w * h, 4);
    if (!im->px) { free(im); return NULL; }
    return im;
}

void img_free(Image *im)
{
    if (!im) return;
    free(im->px);
    free(im);
}

/* Load and scale to fit inside bw x bh. Pixel art upscales with nearest
 * neighbour (integer factor when close), downscales use box averaging. */
Image *img_load_fit(const char *path, int bw, int bh, uint32_t bg)
{
    int w, h, n;
    unsigned char *src = stbi_load(path, &w, &h, &n, 4);
    if (!src) return NULL;
    if (w <= 0 || h <= 0) { stbi_image_free(src); return NULL; }

    float s = fminf((float)bw / w, (float)bh / h);
    int dw, dh;
    if (s >= 1) {
        int si = (int)s;
        if ((float)si / s >= 0.85f) s = (float)si;
    }
    dw = (int)lroundf(w * s), dh = (int)lroundf(h * s);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    if (dw > bw) dw = bw;
    if (dh > bh) dh = bh;

    Image *im = img_new(dw, dh);
    if (!im) { stbi_image_free(src); return NULL; }

    for (int y = 0; y < dh; y++) {
        int sy0 = (int)((long)y * h / dh), sy1 = (int)((long)(y + 1) * h / dh);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int x = 0; x < dw; x++) {
            int sx0 = (int)((long)x * w / dw), sx1 = (int)((long)(x + 1) * w / dw);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            unsigned r = 0, g = 0, b = 0, a = 0, cnt = 0;
            for (int yy = sy0; yy < sy1; yy++)
                for (int xx = sx0; xx < sx1; xx++) {
                    unsigned char *p = &src[((size_t)yy * w + xx) * 4];
                    r += p[0] * p[3], g += p[1] * p[3], b += p[2] * p[3], a += p[3];
                    cnt++;
                }
            uint32_t col = bg | 0xFF000000u;
            if (a) {
                uint32_t sc = ((r / a) << 16) | ((g / a) << 8) | (b / a);
                col = blend(col, sc, a / cnt);
            }
            im->px[y * dw + x] = col;
        }
    }
    stbi_image_free(src);
    return im;
}

static int save_argb(const uint32_t *px, int w, int h, const char *path)
{
    unsigned char *rgb = malloc((size_t)w * h * 3);
    if (!rgb) return -1;
    for (int i = 0; i < w * h; i++) {
        rgb[i * 3] = (px[i] >> 16) & 255;
        rgb[i * 3 + 1] = (px[i] >> 8) & 255;
        rgb[i * 3 + 2] = px[i] & 255;
    }
    int ok = stbi_write_png(path, w, h, 3, rgb, w * 3);
    free(rgb);
    return ok ? 0 : -1;
}

int img_save_png(const Image *im, const char *path) { return save_argb(im->px, im->w, im->h, path); }
int canvas_save_png(const char *path) { return save_argb(g_px, SCR_W, SCR_H, path); }
