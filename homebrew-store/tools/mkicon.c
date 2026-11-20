/* Renders the 74x74 Onion app icon (RGBA PNG) using the app's own drawing code.
 * Drawn at 4x, twice: once as a white mask (for alpha) and once in colour. */
#include <stdio.h>
#include <stdlib.h>

#include "../src/gfx.h"
#include "../vendor/stb_image_write.h"

#define S 4
#define N 74

static void shape(int mask)
{
    uint32_t gold = mask ? 0xFFFFFFFF : 0xFFFFC94D;
    uint32_t dark = mask ? 0xFFFFFFFF : 0xFF0F1018;
    gfx_clear(0xFF000000);
    gfx_rrect(4 * S, 6 * S, 66 * S, 62 * S, 12 * S, gold);
    if (mask) return;
    gfx_rrect(13 * S, 14 * S, 48 * S, 30 * S, 4 * S, dark);
    /* download arrow */
    gfx_line(37 * S, 19 * S, 37 * S, 34 * S, 4.5f * S, gold);
    gfx_line(30 * S, 28 * S, 37 * S, 35 * S, 4.5f * S, gold);
    gfx_line(44 * S, 28 * S, 37 * S, 35 * S, 4.5f * S, gold);
    gfx_rect(28 * S, 38 * S, 18 * S, 3 * S, gold);
    /* buttons */
    gfx_rect(16 * S, 51 * S, 12 * S, 4 * S, dark);
    gfx_rect(20 * S, 47 * S, 4 * S, 12 * S, dark);
    gfx_circle(50 * S, 50 * S, 3.2f * S, dark);
    gfx_circle(57 * S, 55 * S, 3.2f * S, dark);
}

int main(int argc, char **argv)
{
    static uint32_t mask[SCR_W * SCR_H];
    shape(1);
    for (int i = 0; i < SCR_W * SCR_H; i++) mask[i] = g_px[i];
    shape(0);
    unsigned char out[N * N * 4];
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            unsigned r = 0, g = 0, b = 0, a = 0;
            for (int j = 0; j < S; j++)
                for (int i = 0; i < S; i++) {
                    int p = (y * S + j) * SCR_W + x * S + i;
                    a += mask[p] & 255;
                    r += (g_px[p] >> 16) & 255, g += (g_px[p] >> 8) & 255, b += g_px[p] & 255;
                }
            unsigned char *o = &out[(y * N + x) * 4];
            o[3] = a / (S * S);
            o[0] = a ? (r * 255 / a > 255 ? 255 : r * 255 / a) : 0;
            o[1] = a ? (g * 255 / a > 255 ? 255 : g * 255 / a) : 0;
            o[2] = a ? (b * 255 / a > 255 ? 255 : b * 255 / a) : 0;
        }
    return stbi_write_png(argc > 1 ? argv[1] : "icon.png", N, N, 4, out, N * 4) ? 0 : 1;
}
