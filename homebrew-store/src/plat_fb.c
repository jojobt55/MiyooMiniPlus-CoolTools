/* Miyoo Mini / Mini Plus: draw straight to /dev/fb0 (panel is mounted
 * upside down, so frames are rotated 180 degrees) and read buttons from
 * /dev/input/event*. Same approach Onion OS uses for its own tools.
 * Every step logs to stderr (launch.sh sends it to log.txt). */
#include "plat.h"
#include "gfx.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static int fb = -1;
static uint8_t *fbmem;
static size_t fbsize;
static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;
static int pages = 1, cur = 0, bpp = 4, pitch;
static int rotate = 1;

static int in_fd[8], n_in;

/* Linux input event as the 32-bit kernel sends it (16 bytes). */
struct ev32 {
    uint32_t sec, usec;
    uint16_t type, code;
    int32_t value;
};

#define LOG(...) fprintf(stderr, "[fb] " __VA_ARGS__)

int plat_init(void)
{
    fb = open("/dev/fb0", O_RDWR);
    if (fb < 0) { LOG("open /dev/fb0 failed: %s\n", strerror(errno)); return -1; }
    if (ioctl(fb, FBIOGET_FSCREENINFO, &fi)) { LOG("FBIOGET_FSCREENINFO: %s\n", strerror(errno)); return -1; }
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vi)) { LOG("FBIOGET_VSCREENINFO: %s\n", strerror(errno)); return -1; }
    LOG("id=%.16s xres=%u yres=%u virt=%ux%u yoffset=%u bpp=%u line_length=%u smem_len=%u\n", fi.id, vi.xres,
        vi.yres, vi.xres_virtual, vi.yres_virtual, vi.yoffset, vi.bits_per_pixel, fi.line_length, fi.smem_len);

    if (vi.bits_per_pixel != 32 && vi.bits_per_pixel != 16) {
        LOG("unexpected depth, asking for 32 bpp\n");
        vi.bits_per_pixel = 32;
        ioctl(fb, FBIOPUT_VSCREENINFO, &vi);
        ioctl(fb, FBIOGET_VSCREENINFO, &vi);
        ioctl(fb, FBIOGET_FSCREENINFO, &fi);
        LOG("now bpp=%u line_length=%u\n", vi.bits_per_pixel, fi.line_length);
        if (vi.bits_per_pixel != 32 && vi.bits_per_pixel != 16) return -1;
    }
    bpp = vi.bits_per_pixel / 8;
    pitch = fi.line_length ? (int)fi.line_length : (int)(vi.xres_virtual * bpp);
    if (vi.xres == 0 || vi.yres == 0) { LOG("bad resolution\n"); return -1; }

    size_t need = (size_t)pitch * (vi.yres_virtual > vi.yres ? vi.yres_virtual : vi.yres);
    fbsize = fi.smem_len ? fi.smem_len : need;
    fbmem = mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fbmem == MAP_FAILED && fbsize != need) {
        LOG("mmap %zu failed (%s), retrying with %zu\n", fbsize, strerror(errno), need);
        fbsize = need;
        fbmem = mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    }
    if (fbmem == MAP_FAILED) { fbmem = NULL; LOG("mmap failed: %s\n", strerror(errno)); return -1; }

    pages = (int)(vi.yres_virtual / vi.yres);
    if (pages < 1) pages = 1;
    while (pages > 1 && (size_t)pages * vi.yres * pitch > fbsize) pages--;
    cur = (int)(vi.yoffset / vi.yres);
    if (cur >= pages) cur = 0;
    if (access("norotate", F_OK) == 0) rotate = 0;
    LOG("pages=%d current=%d rotate=%d\n", pages, cur, rotate);

    for (int i = 0; i < 8; i++) {
        char p[32];
        snprintf(p, sizeof p, "/dev/input/event%d", i);
        int fd = open(p, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) in_fd[n_in++] = fd;
    }
    LOG("input devices opened: %d\n", n_in);
    return 0;
}

static void blit_page(int page, const uint32_t *px)
{
    int W = vi.xres, H = vi.yres;
    int ox = (W - SCR_W) / 2, oy = (H - SCR_H) / 2;
    uint8_t *base = fbmem + (size_t)page * H * pitch;
    for (int y = 0; y < H; y++) {
        uint8_t *row = base + (size_t)(rotate ? H - 1 - y : y) * pitch;
        int sy = y - oy;
        for (int x = 0; x < W; x++) {
            int sx = x - ox;
            uint32_t c = (sy >= 0 && sy < SCR_H && sx >= 0 && sx < SCR_W) ? px[sy * SCR_W + sx] : 0xFF000000u;
            int dx = rotate ? W - 1 - x : x;
            if (bpp == 4) {
                ((uint32_t *)row)[dx] = c;
            } else {
                ((uint16_t *)row)[dx] = (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F));
            }
        }
    }
}

void plat_present(const uint32_t *px)
{
    if (pages > 1) {
        int back = (cur + 1) % pages;
        blit_page(back, px);
        vi.yoffset = back * vi.yres;
        if (ioctl(fb, FBIOPAN_DISPLAY, &vi) == 0) { cur = back; return; }
        LOG("FBIOPAN_DISPLAY failed (%s), drawing to the visible page\n", strerror(errno));
        pages = 1;
        vi.yoffset = cur * vi.yres;
    }
    blit_page(cur, px);
}

static int map_key(int code)
{
    switch (code) {
    case 103: return BTN_UP;
    case 108: return BTN_DOWN;
    case 105: return BTN_LEFT;
    case 106: return BTN_RIGHT;
    case 57: return BTN_A;       /* KEY_SPACE */
    case 29: return BTN_B;       /* KEY_LEFTCTRL */
    case 42: return BTN_X;       /* KEY_LEFTSHIFT */
    case 56: return BTN_Y;       /* KEY_LEFTALT */
    case 18: return BTN_L1;      /* KEY_E */
    case 20: return BTN_R1;      /* KEY_T */
    case 15: return BTN_L2;      /* KEY_TAB */
    case 14: return BTN_R2;      /* KEY_BACKSPACE */
    case 97: return BTN_SELECT;  /* KEY_RIGHTCTRL */
    case 28: return BTN_START;   /* KEY_ENTER */
    case 1: return BTN_MENU;     /* KEY_ESC */
    }
    return -2;
}

int plat_poll(int *btn, int *down)
{
    for (int i = 0; i < n_in; i++) {
        struct ev32 e;
        while (read(in_fd[i], &e, sizeof e) == (ssize_t)sizeof e) {
            if (e.type != 1 || e.value == 2) continue; /* EV_KEY, ignore autorepeat */
            int b = map_key(e.code);
            if (b < 0) continue;
            *btn = b;
            *down = e.value != 0;
            return 1;
        }
    }
    return 0;
}

void plat_quit(void)
{
    if (fbmem) {
        for (int p = 0; p < pages; p++) memset(fbmem + (size_t)p * vi.yres * pitch, 0, (size_t)vi.yres * pitch);
        vi.yoffset = 0;
        ioctl(fb, FBIOPAN_DISPLAY, &vi);
        munmap(fbmem, fbsize);
    }
    if (fb >= 0) close(fb);
    for (int i = 0; i < n_in; i++) close(in_fd[i]);
}
