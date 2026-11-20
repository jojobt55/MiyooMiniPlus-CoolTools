/* Homebrew Store for Miyoo Mini / Mini Plus (Onion OS).
 * Browses the Homebrew Hub catalog (hh.gbdev.io) of free, legal homebrew
 * for GB, GBC, GBA and NES, and installs games into Onion's Roms folders. */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "catalog.h"
#include "gfx.h"
#include "net.h"
#include "plat.h"

/* ---------- palette ---------- */
#define C_BG     0xFF0F1018
#define C_PANEL  0xFF191B27
#define C_PANEL2 0xFF222536
#define C_SEL    0xFF2C3150
#define C_TEXT   0xFFEEF0F6
#define C_DIM    0xFF9096AE
#define C_FAINT  0xFF5A6078
#define C_GOLD   0xFFFFC94D
#define C_OK     0xFF3DDC84
#define C_ERR    0xFFFF6B6B
#define C_SHADE  0xC0000000

static const uint32_t plat_col[P_COUNT] = {0xFF9BBC0F, 0xFFB28DFF, 0xFF5B8CFF, 0xFFFF5A5F};

/* ---------- layout ---------- */
#define ROWS 11
#define ROW_H 34
#define LIST_X 12
#define LIST_Y 80
#define LIST_W 318
#define PANEL_X 342
#define PANEL_W 286
#define COVER_W 266
#define COVER_H 196

/* ---------- state ---------- */
enum { SCR_LOADING, SCR_BROWSE, SCR_DETAIL };
enum { SORT_AZ, SORT_NEW, SORT_RANDOM, SORT_COUNT };
enum { TF_GAMES, TF_DEMOS, TF_MUSIC, TF_TOOLS, TF_ALL, TF_COUNT };
static const char *sort_names[] = {"A-Z", "Newest", "Shuffle"};
static const char *tf_names[] = {"Games", "Demos", "Music", "Tools", "Everything"};
static const char *tab_names[] = {"ALL", "GB", "GBC", "GBA", "NES"};

static Catalog cat;
static int *view, nview;
static int sel, top, tab, typef, sortm;
static char query[48];
static int screen = SCR_LOADING;
static int det_shot, det_scroll, det_lines;
static int modal; /* 1 = confirm remove */
static int searching, kb_row, kb_col;
static char load_err[160];
static char toast_msg[160];
static uint32_t toast_until, sel_changed_at, now_ms;
static int running = 1, dirty = 1;

static char roms_root[512];
static Job jcat, jshot, jinst;
static int jshot_e = -1, jshot_k, jinst_e = -1;
static int cat_from_cache, pending_reload;

static uint32_t ticks(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void toast(const char *fmt, const char *arg)
{
    snprintf(toast_msg, sizeof toast_msg, fmt, arg);
    toast_until = now_ms + 2600;
    dirty = 1;
}

static int file_exists(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0;
}

static void mkdirs(const char *path)
{
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++)
        if (*p == '/') { *p = 0; mkdir(tmp, 0777); *p = '/'; }
    mkdir(tmp, 0777);
}

/* ---------- paths ---------- */
static void rom_path(const Entry *e, char *out, int n)
{
    snprintf(out, n, "%s/%s/%s", roms_root, plat_dir(e->plat), e->localname);
}

static void art_path(const Entry *e, char *out, int n)
{
    char base[256];
    snprintf(base, sizeof base, "%s", e->localname);
    char *dot = strrchr(base, '.');
    if (dot) *dot = 0;
    snprintf(out, n, "%s/%s/Imgs/%s.png", roms_root, plat_dir(e->plat), base);
}

static void shot_cache_path(const Entry *e, int k, char *out, int n)
{
    char slug[160];
    snprintf(slug, sizeof slug, "%s", e->slug);
    for (char *p = slug; *p; p++)
        if (*p == '/' || *p == '\\') *p = '_';
    snprintf(out, n, "cache/%s-%d.png", slug, k);
}

static void entry_url(const Entry *e, const char *file, char *out, int n)
{
    char rel[700], esc[1000];
    snprintf(rel, sizeof rel, "%s/entries/%s/%s", e->basepath, e->slug, file);
    url_escape_path(esc, sizeof esc, rel);
    snprintf(out, n, HUB_BASE "/static/%s", esc);
}

/* Onion caches each Roms folder's game list; drop it so new games show up. */
static void invalidate_rom_cache(int plat)
{
    char dir[600];
    snprintf(dir, sizeof dir, "%s/%s", roms_root, plat_dir(plat));
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        size_t l = strlen(de->d_name);
        if (l > 3 && !strcmp(de->d_name + l - 3, ".db") && strstr(de->d_name, "_cache")) {
            char p[900];
            snprintf(p, sizeof p, "%s/%s", dir, de->d_name);
            unlink(p);
        }
    }
    closedir(d);
}

/* ---------- image cache ---------- */
typedef struct {
    int e, k, bw, bh;
    Image *img;
    uint32_t used;
} ImgSlot;
static ImgSlot icache[24];

static void icache_clear(void)
{
    for (int i = 0; i < 24; i++) {
        img_free(icache[i].img);
        icache[i].img = NULL;
        icache[i].e = -1;
    }
}

static Image *get_shot(int ei, int k, int bw, int bh)
{
    Entry *e = &cat.e[ei];
    if (k >= e->nshots || e->shot_state[k] != 2) return NULL;
    int lru = 0;
    for (int i = 0; i < 24; i++) {
        if (icache[i].img && icache[i].e == ei && icache[i].k == k && icache[i].bw == bw && icache[i].bh == bh) {
            icache[i].used = now_ms;
            return icache[i].img;
        }
        if (!icache[i].img || icache[i].used < icache[lru].used) lru = i;
        if (!icache[i].img) break;
    }
    char p[512];
    shot_cache_path(e, k, p, sizeof p);
    Image *im = img_load_fit(p, bw, bh, C_PANEL2);
    if (!im) {
        e->shot_state[k] = 3;
        unlink(p);
        return NULL;
    }
    img_free(icache[lru].img);
    icache[lru] = (ImgSlot){ei, k, bw, bh, im, now_ms};
    return im;
}

/* ---------- view (filter + sort) ---------- */
static int ci_contains(const char *hay, const char *needle)
{
    if (!*needle) return 1;
    size_t n = strlen(needle);
    for (; *hay; hay++)
        if (!strncasecmp(hay, needle, n)) return 1;
    return 0;
}

/* sort by the first letter or digit, so "(GB) Foo" files under F */
static const char *sort_key(const char *t)
{
    const char *p = t;
    while (*p && !isalnum((unsigned char)*p)) p++;
    return *p ? p : t;
}

static int cmp_az(const void *a, const void *b)
{
    const Entry *x = &cat.e[*(const int *)a], *y = &cat.e[*(const int *)b];
    return strcasecmp(sort_key(x->title), sort_key(y->title));
}

static int cmp_new(const void *a, const void *b)
{
    const Entry *x = &cat.e[*(const int *)a], *y = &cat.e[*(const int *)b];
    int c = strcmp(y->date, x->date);
    return c ? c : strcasecmp(x->title, y->title);
}

static int cmp_rnd(const void *a, const void *b)
{
    unsigned x = cat.e[*(const int *)a].rnd, y = cat.e[*(const int *)b].rnd;
    return x < y ? -1 : x > y;
}

static int cur_entry(void) { return nview && sel >= 0 && sel < nview ? view[sel] : -1; }

static void rebuild_view(void)
{
    int keep = cur_entry();
    const char *keep_slug = keep >= 0 ? cat.e[keep].slug : NULL;
    char slugbuf[160] = "";
    if (keep_slug) snprintf(slugbuf, sizeof slugbuf, "%s", keep_slug);

    free(view);
    view = malloc(sizeof(int) * (cat.n ? cat.n : 1));
    nview = 0;
    for (int i = 0; i < cat.n; i++) {
        Entry *e = &cat.e[i];
        if (tab > 0 && e->plat != tab - 1) continue;
        if (typef != TF_ALL) {
            int want = typef == TF_GAMES ? T_GAME : typef == TF_DEMOS ? T_DEMO : typef == TF_MUSIC ? T_MUSIC : T_TOOL;
            if (e->type != want) continue;
        }
        if (*query && !ci_contains(e->title, query) && !ci_contains(e->dev, query) && !ci_contains(e->tags, query))
            continue;
        view[nview++] = i;
    }
    qsort(view, nview, sizeof(int), sortm == SORT_AZ ? cmp_az : sortm == SORT_NEW ? cmp_new : cmp_rnd);

    sel = 0;
    if (*slugbuf)
        for (int i = 0; i < nview; i++)
            if (!strcmp(cat.e[view[i]].slug, slugbuf)) { sel = i; break; }
    top = sel - ROWS / 2;
    if (top > nview - ROWS) top = nview - ROWS;
    if (top < 0) top = 0;
    sel_changed_at = now_ms;
    dirty = 1;
}

static void move_sel(int d)
{
    if (!nview) return;
    int ns = sel + d;
    if (ns < 0) ns = 0;
    if (ns >= nview) ns = nview - 1;
    if (ns == sel) return;
    sel = ns;
    if (sel < top) top = sel;
    if (sel >= top + ROWS) top = sel - ROWS + 1;
    sel_changed_at = now_ms;
    dirty = 1;
}

/* ---------- catalog lifecycle ---------- */
static void scan_installed(void)
{
    char p[1024];
    for (int i = 0; i < cat.n; i++) {
        rom_path(&cat.e[i], p, sizeof p);
        cat.e[i].installed = file_exists(p);
    }
}

static void prime_shot_states(void)
{
    char p[512];
    for (int i = 0; i < cat.n; i++)
        for (int k = 0; k < cat.e[i].nshots; k++) {
            shot_cache_path(&cat.e[i], k, p, sizeof p);
            cat.e[i].shot_state[k] = file_exists(p) ? 2 : 0;
        }
}

static int load_catalog_file(const char *path)
{
    Catalog nc;
    char err[120];
    if (catalog_load(&nc, path, err, sizeof err)) {
        snprintf(load_err, sizeof load_err, "%s", err);
        return -1;
    }
    /* keep the selection across a reload */
    char keep[160] = "";
    int ce = cur_entry();
    if (ce >= 0) snprintf(keep, sizeof keep, "%s", cat.e[ce].slug);

    job_cancel(&jshot);
    jshot_e = -1;
    if (jinst.pid) { job_cancel(&jinst); jinst_e = -1; }
    icache_clear();
    catalog_free(&cat);
    cat = nc;
    srand((unsigned)time(NULL));
    for (int i = 0; i < cat.n; i++) cat.e[i].rnd = (unsigned)rand();
    scan_installed();
    prime_shot_states();
    nview = 0;
    rebuild_view();
    if (*keep)
        for (int i = 0; i < nview; i++)
            if (!strcmp(cat.e[view[i]].slug, keep)) { sel = i; break; }
    if (sel >= top + ROWS || sel < top) top = sel - ROWS / 2 < 0 ? 0 : sel - ROWS / 2;
    return 0;
}

static void start_catalog_fetch(void)
{
    if (job_busy(&jcat)) return;
    load_err[0] = 0;
    job_start(&jcat, CATALOG_URL, "cache/catalog.new");
    dirty = 1;
}

/* ---------- installs ---------- */
static void write_box_art(int ei)
{
    Entry *e = &cat.e[ei];
    if (!e->nshots || e->shot_state[0] != 2) return;
    char src[512], dst[1024];
    shot_cache_path(e, 0, src, sizeof src);
    art_path(e, dst, sizeof dst);
    if (file_exists(dst)) return;
    char dir[1024];
    snprintf(dir, sizeof dir, "%s", dst);
    *strrchr(dir, '/') = 0;
    mkdirs(dir);
    Image *im = img_load_fit(src, 250, 250, 0xFF000000);
    if (im) {
        img_save_png(im, dst);
        img_free(im);
    }
}

static void start_install(int ei)
{
    Entry *e = &cat.e[ei];
    if (e->installed) { toast("Already installed", NULL); return; }
    if (job_busy(&jinst)) { toast("Wait for the current download to finish", NULL); return; }
    char dst[1024], dir[1024], url[1200];
    rom_path(e, dst, sizeof dst);
    snprintf(dir, sizeof dir, "%s", dst);
    *strrchr(dir, '/') = 0;
    mkdirs(dir);
    entry_url(e, e->file, url, sizeof url);
    if (job_start(&jinst, url, dst)) { toast("Couldn't start download", NULL); return; }
    jinst_e = ei;
    dirty = 1;
}

static void remove_game(int ei)
{
    Entry *e = &cat.e[ei];
    char p[1024];
    rom_path(e, p, sizeof p);
    unlink(p);
    art_path(e, p, sizeof p);
    unlink(p);
    invalidate_rom_cache(e->plat);
    e->installed = 0;
    toast("Removed %s", e->title);
}

/* ---------- background work ---------- */
static void want_shot(int ei, int k)
{
    if (ei < 0 || job_busy(&jshot)) return;
    Entry *e = &cat.e[ei];
    if (k >= e->nshots || e->shot_state[k] != 0) return;
    char url[1200], dst[512];
    entry_url(e, e->shots[k], url, sizeof url);
    shot_cache_path(e, k, dst, sizeof dst);
    if (job_start(&jshot, url, dst) == 0) {
        e->shot_state[k] = 1;
        jshot_e = ei, jshot_k = k;
    }
}

static void update_jobs(void)
{
    if (job_busy(&jcat)) {
        int r = job_poll(&jcat);
        dirty = 1;
        if (r == JOB_OK) {
            int first = cat.n == 0;
            if (!first && job_busy(&jinst)) {
                pending_reload = 1; /* swap catalogs once the install finishes */
            } else if (load_catalog_file("cache/catalog.new") == 0) {
                rename("cache/catalog.new", "cache/catalog.json");
                cat_from_cache = 0;
                if (first) screen = SCR_BROWSE;
                else toast("Catalog updated", NULL);
            } else if (first) {
                screen = SCR_LOADING;
            }
        } else if (r == JOB_FAIL) {
            if (cat.n == 0) snprintf(load_err, sizeof load_err, "%s", net_error(jcat.exit_code));
            /* with a cached catalog, a failed refresh is silent */
        }
    }
    if (job_busy(&jshot)) {
        int r = job_poll(&jshot);
        if (r != JOB_RUNNING && jshot_e >= 0 && jshot_e < cat.n) {
            Entry *e = &cat.e[jshot_e];
            e->shot_state[jshot_k] = r == JOB_OK ? 2 : 3;
            if (r == JOB_OK && jshot_k == 0 && e->installed) write_box_art(jshot_e);
            jshot_e = -1;
            dirty = 1;
        }
    }
    if (job_busy(&jinst)) {
        int r = job_poll(&jinst);
        dirty = 1;
        if (r != JOB_RUNNING && jinst_e >= 0) {
            Entry *e = &cat.e[jinst_e];
            if (r == JOB_OK) {
                e->installed = 1;
                write_box_art(jinst_e);
                invalidate_rom_cache(e->plat);
                char msg[120];
                snprintf(msg, sizeof msg, "Installed to Roms/%s", plat_dir(e->plat));
                toast("%s", msg);
            } else {
                toast("%s", net_error(jinst.exit_code));
            }
            jinst_e = -1;
        }
    }
    if (pending_reload && !job_busy(&jinst)) {
        pending_reload = 0;
        if (load_catalog_file("cache/catalog.new") == 0) rename("cache/catalog.new", "cache/catalog.json");
        dirty = 1;
    }

    /* fetch artwork: the selection first, then its neighbours */
    if (screen == SCR_BROWSE && nview && now_ms - sel_changed_at > 180 && !job_busy(&jshot)) {
        static const int order[] = {0, 1, -1, 2, 3, -2, 4, 5};
        for (int i = 0; i < 8; i++) {
            int s = sel + order[i];
            if (s < 0 || s >= nview) continue;
            Entry *e = &cat.e[view[s]];
            if (e->nshots && e->shot_state[0] == 0) { want_shot(view[s], 0); break; }
        }
    } else if (screen == SCR_DETAIL && !job_busy(&jshot)) {
        int ei = cur_entry();
        if (ei >= 0) {
            Entry *e = &cat.e[ei];
            if (det_shot < e->nshots && e->shot_state[det_shot] == 0) want_shot(ei, det_shot);
            else
                for (int k = 0; k < e->nshots; k++)
                    if (e->shot_state[k] == 0) { want_shot(ei, k); break; }
        }
    }
}

/* ---------- drawing helpers ---------- */
static void spinner(float cx, float cy, float r, uint32_t col)
{
    float ph = (now_ms % 1000) / 1000.0f;
    for (int i = 0; i < 8; i++) {
        float a = (i / 8.0f + ph) * 6.2831853f;
        unsigned alpha = 40 + 215 * i / 7;
        gfx_circle(cx + cosf(a) * r, cy + sinf(a) * r, r * 0.22f, (col & 0xFFFFFF) | (alpha << 24));
    }
}

static void check_badge(int cx, int cy, int r)
{
    gfx_circle(cx, cy, r, C_OK);
    gfx_line(cx - r * 0.45f, cy + 0.5f, cx - r * 0.1f, cy + r * 0.38f, r * 0.3f, 0xFF0F1018);
    gfx_line(cx - r * 0.1f, cy + r * 0.38f, cx + r * 0.5f, cy - r * 0.38f, r * 0.3f, 0xFF0F1018);
}

static int plat_badge(int x, int y, int plat)
{
    const char *n = plat_name(plat);
    int w = gfx_text_w(F_PIXEL, 8, n) + 10;
    gfx_rrect(x, y, w, 16, 4, plat_col[plat]);
    gfx_text(F_PIXEL, 8, x + 5, y + 4, 0xFF0F1018, n);
    return w;
}

static int pill(int x, int y, const char *label, uint32_t bg, uint32_t fg)
{
    int w = gfx_text_w(F_BOLD, 13, label) + 16;
    gfx_rrect(x, y, w, 22, 11, bg);
    gfx_text(F_BOLD, 13, x + 8, y + 3, fg, label);
    return w;
}

/* footer hint: button glyph + label; returns width used */
static int hint(int x, int y, const char *btn, const char *label)
{
    int bw = gfx_text_w(F_BOLD, 12, btn);
    int gw = bw + 12 < 20 ? 20 : bw + 12;
    gfx_rrect(x, y, gw, 20, 10, C_PANEL2);
    gfx_text(F_BOLD, 12, x + (gw - bw) / 2, y + 3, C_TEXT, btn);
    int lw = gfx_text(F_REG, 14, x + gw + 5, y + 2, C_DIM, label);
    return gw + 5 + lw + 14;
}

static void draw_footer(const char **pairs, int n)
{
    gfx_rect(0, SCR_H - 28, SCR_W, 28, C_PANEL);
    int total = 0;
    /* measure, then center */
    for (int i = 0; i < n; i++) {
        int bw = gfx_text_w(F_BOLD, 12, pairs[i * 2]);
        int gw = bw + 12 < 20 ? 20 : bw + 12;
        total += gw + 5 + gfx_text_w(F_REG, 14, pairs[i * 2 + 1]) + 14;
    }
    int x = (SCR_W - total + 14) / 2;
    if (x < 8) x = 8;
    for (int i = 0; i < n; i++) x += hint(x, SCR_H - 24, pairs[i * 2], pairs[i * 2 + 1]);
}

static void draw_logo(int x, int y)
{
    gfx_rrect(x, y + 1, 22, 18, 3, C_GOLD);
    gfx_rect(x + 4, y + 5, 14, 7, 0xFF0F1018);
    gfx_rect(x + 6, y + 14, 3, 3, 0xFF0F1018);
    gfx_rect(x + 13, y + 14, 3, 3, 0xFF0F1018);
    gfx_text(F_PIXEL, 8, x + 30, y + 2, C_GOLD, "HOMEBREW");
    gfx_text(F_PIXEL, 8, x + 30, y + 12, C_TEXT, "STORE");
}

static void draw_header(void)
{
    gfx_rect(0, 0, SCR_W, 44, C_PANEL);
    draw_logo(14, 12);
    /* platform tabs */
    int w[5], total = 0;
    for (int i = 0; i < 5; i++) w[i] = gfx_text_w(F_PIXEL, 8, tab_names[i]) + 22, total += w[i] + 4;
    int x = SCR_W - 14 - total - 54;
    gfx_text(F_BOLD, 12, x, 15, C_FAINT, "L1");
    x += 24;
    for (int i = 0; i < 5; i++) {
        uint32_t bg = i == tab ? (i ? plat_col[i - 1] : C_GOLD) : C_PANEL2;
        uint32_t fg = i == tab ? 0xFF0F1018 : C_DIM;
        gfx_rrect(x, 10, w[i], 24, 12, bg);
        gfx_text(F_PIXEL, 8, x + 11, 18, fg, tab_names[i]);
        x += w[i] + 4;
    }
    gfx_text(F_BOLD, 12, x + 6, 15, C_FAINT, "R1");
}

static void draw_toast(void)
{
    if (now_ms >= toast_until || !*toast_msg) return;
    int w = gfx_text_w(F_BOLD, 15, toast_msg) + 32;
    if (w > SCR_W - 40) w = SCR_W - 40;
    int x = (SCR_W - w) / 2, y = SCR_H - 76;
    gfx_rrect(x, y, w, 34, 17, 0xF02C3150);
    gfx_text_fit(F_BOLD, 15, x + 16, y + 7, w - 32, C_TEXT, toast_msg);
}

static void cover_box(int ei, int k, int x, int y, int w, int h)
{
    gfx_rrect(x, y, w, h, 8, C_PANEL2);
    if (ei < 0) return;
    Entry *e = &cat.e[ei];
    Image *im = k < e->nshots ? get_shot(ei, k, w - 8, h - 8) : NULL;
    if (im) {
        gfx_image(im, x + (w - im->w) / 2, y + (h - im->h) / 2);
    } else if (k < e->nshots && e->shot_state[k] <= 1) {
        spinner(x + w / 2.0f, y + h / 2.0f, 14, C_DIM);
    } else {
        const char *n = "NO IMAGE";
        gfx_text(F_PIXEL, 8, x + (w - gfx_text_w(F_PIXEL, 8, n)) / 2, y + h / 2 - 4, C_FAINT, n);
    }
}

static void progress_bar(int x, int y, int w, int h, long bytes)
{
    gfx_rrect(x, y, w, h, h / 2, C_PANEL2);
    /* ROM sizes are unknown up front: show an indeterminate sweep */
    int seg = w / 3;
    int off = (int)((now_ms % 1200) * (w + seg) / 1200) - seg;
    gfx_clip(x, y, w, h);
    gfx_rrect(x + off, y, seg, h, h / 2, C_GOLD);
    gfx_noclip();
    (void)bytes;
}

/* ---------- screens ---------- */
static void draw_loading(void)
{
    gfx_clear(C_BG);
    int cx = SCR_W / 2;
    gfx_rrect(cx - 44, 120, 88, 70, 10, C_GOLD);
    gfx_rect(cx - 30, 134, 60, 30, 0xFF0F1018);
    gfx_rect(cx - 22, 172, 10, 10, 0xFF0F1018);
    gfx_rect(cx + 12, 172, 10, 10, 0xFF0F1018);
    const char *t = "HOMEBREW STORE";
    gfx_text(F_PIXEL, 16, cx - gfx_text_w(F_PIXEL, 16, t) / 2, 214, C_TEXT, t);
    const char *s = "Free & legal games from Homebrew Hub";
    gfx_text(F_REG, 16, cx - gfx_text_w(F_REG, 16, s) / 2, 244, C_DIM, s);

    if (job_busy(&jcat)) {
        spinner(cx, 312, 14, C_GOLD);
        char m[64];
        if (jcat.bytes > 0) snprintf(m, sizeof m, "Downloading catalog...  %ld KB", jcat.bytes / 1024);
        else snprintf(m, sizeof m, "Connecting...");
        gfx_text(F_REG, 15, cx - gfx_text_w(F_REG, 15, m) / 2, 340, C_DIM, m);
    } else if (*load_err) {
        gfx_text(F_BOLD, 17, cx - gfx_text_w(F_BOLD, 17, load_err) / 2, 300, C_ERR, load_err);
        const char *h = "Turn on Wi-Fi in Onion's settings, then try again.";
        gfx_text(F_REG, 14, cx - gfx_text_w(F_REG, 14, h) / 2, 330, C_DIM, h);
        const char *f[] = {"A", "Retry", "B", "Quit"};
        draw_footer(f, 2);
    }
}

static void draw_browse(void)
{
    gfx_clear(C_BG);
    draw_header();

    /* sub-header */
    char sub[160];
    snprintf(sub, sizeof sub, "%s  ·  %s  ·  %d title%s", tf_names[typef], sort_names[sortm], nview, nview == 1 ? "" : "s");
    gfx_text(F_BOLD, 15, LIST_X + 4, 52, C_DIM, sub);
    if (*query) {
        char q[80];
        snprintf(q, sizeof q, "\"%s\"", query);
        int w = gfx_text_w(F_BOLD, 13, q) + 16;
        pill(PANEL_X + PANEL_W - w, 50, q, C_SEL, C_GOLD);
    } else if (job_busy(&jcat)) {
        spinner(PANEL_X + PANEL_W - 10, 61, 6, C_DIM);
    }

    /* list */
    if (!nview) {
        const char *m = *query ? "No matches. Press Y to change the search." : "Nothing here yet.";
        gfx_text(F_REG, 16, LIST_X + 6, LIST_Y + 12, C_DIM, m);
    }
    for (int r = 0; r < ROWS && top + r < nview; r++) {
        int i = top + r, ei = view[i];
        Entry *e = &cat.e[ei];
        int y = LIST_Y + r * ROW_H;
        if (i == sel) {
            gfx_rrect(LIST_X, y, LIST_W, ROW_H - 2, 8, C_SEL);
            gfx_rrect(LIST_X, y + 7, 4, ROW_H - 16, 2, C_GOLD);
        }
        int bx = LIST_X + 10;
        plat_badge(bx, y + 8, e->plat);
        int tx = bx + 46;
        int right = LIST_X + LIST_W - 10;
        if (ei == jinst_e) {
            spinner(right - 8, y + 16, 7, C_GOLD);
            right -= 24;
        } else if (e->installed) {
            check_badge(right - 8, y + 16, 8);
            right -= 24;
        }
        gfx_text_fit(i == sel ? F_BOLD : F_REG, 16, tx, y + 6, right - tx, i == sel ? C_TEXT : 0xFFC9CCDA, e->title);
    }
    /* scrollbar */
    if (nview > ROWS) {
        int th = ROWS * ROW_H - 2;
        int bh = th * ROWS / nview;
        if (bh < 18) bh = 18;
        int by = LIST_Y + (th - bh) * top / (nview - ROWS);
        gfx_rrect(LIST_X + LIST_W + 3, LIST_Y, 3, th, 1, C_PANEL);
        gfx_rrect(LIST_X + LIST_W + 3, by, 3, bh, 1, C_FAINT);
    }

    /* detail panel */
    int ei = cur_entry();
    gfx_rrect(PANEL_X, LIST_Y, PANEL_W, ROWS * ROW_H - 2, 10, C_PANEL);
    if (ei >= 0) {
        Entry *e = &cat.e[ei];
        int px = PANEL_X + 10, pw = PANEL_W - 20;
        cover_box(ei, 0, px, LIST_Y + 10, COVER_W, COVER_H);
        int y = LIST_Y + 10 + COVER_H + 10;
        int lines = gfx_text_wrap(F_BOLD, 18, px, y, pw, 21, 2, 0, C_TEXT, e->title);
        y += 21 * (lines > 2 ? 2 : lines) + 2;
        if (*e->dev) {
            gfx_text_fit(F_REG, 14, px, y, pw, C_DIM, e->dev);
            y += 19;
        }
        int x = px;
        x += plat_badge(x, y + 3, e->plat) + 6;
        char meta[64];
        snprintf(meta, sizeof meta, "%s%s%.4s", type_name(e->type), *e->date ? "  ·  " : "", e->date);
        gfx_text(F_REG, 14, x, y + 1, C_DIM, meta);
        y += 24;
        int bottom = LIST_Y + ROWS * ROW_H - 10;
        int maxl = (bottom - y) / 18;
        if (maxl > 0) gfx_text_wrap(F_REG, 14, px, y, pw, 18, maxl, 0, 0xFFBFC3D3, *e->desc ? e->desc : "No description.");
    }

    const char *f[] = {"A", "Details", "X", "Install", "Y", "Search", "SEL", "Sort", "L2/R2", "Type", "B", "Quit"};
    draw_footer(f, 6);
}

static void draw_detail(void)
{
    gfx_clear(C_BG);
    int ei = cur_entry();
    if (ei < 0) return;
    Entry *e = &cat.e[ei];

    gfx_rect(0, 0, SCR_W, 44, C_PANEL);
    draw_logo(14, 12);
    char pos[48];
    snprintf(pos, sizeof pos, "%d / %d", sel + 1, nview);
    gfx_text(F_REG, 14, SCR_W - 16 - gfx_text_w(F_REG, 14, pos), 14, C_FAINT, pos);

    /* screenshot */
    int sx = 12, sy = 56, sw = 330, sh = 296;
    cover_box(ei, det_shot, sx, sy, sw, sh);
    if (e->nshots > 1) {
        int dots = e->nshots, dx = sx + sw / 2 - (dots * 14) / 2 + 7;
        for (int k = 0; k < dots; k++) gfx_circle(dx + k * 14, sy + sh + 12, 4, k == det_shot ? C_GOLD : C_FAINT);
        gfx_text(F_BOLD, 12, sx + 4, sy + sh + 5, C_FAINT, "L1");
        gfx_text(F_BOLD, 12, sx + sw - 20, sy + sh + 5, C_FAINT, "R1");
    }

    /* install box */
    int iy = sy + sh + 28, ih = 384 + 64 - iy;
    gfx_rrect(sx, iy, sw, ih, 10, C_PANEL);
    char dest[200];
    snprintf(dest, sizeof dest, "Roms/%s/%s", plat_dir(e->plat), e->localname);
    if (ei == jinst_e) {
        char m[64];
        snprintf(m, sizeof m, "Downloading...  %ld KB", jinst.bytes / 1024);
        gfx_text(F_BOLD, 16, sx + 14, iy + 10, C_TEXT, m);
        progress_bar(sx + 14, iy + 38, sw - 28, 10, jinst.bytes);
    } else if (e->installed) {
        check_badge(sx + 24, iy + 21, 10);
        gfx_text(F_BOLD, 16, sx + 42, iy + 10, C_OK, "Installed");
        gfx_text_fit(F_REG, 13, sx + 14, iy + 36, sw - 28, C_DIM, dest);
    } else {
        int w = pill(sx + 14, iy + 10, "A  Install", C_GOLD, 0xFF0F1018);
        (void)w;
        gfx_text_fit(F_REG, 13, sx + 14, iy + 38, sw - 28, C_DIM, dest);
    }

    /* info column */
    int ix = 356, iw = SCR_W - ix - 14;
    int y = 56;
    int tl = gfx_text_wrap(F_BOLD, 22, ix, y, iw, 26, 3, 0, C_TEXT, e->title);
    y += 26 * (tl > 3 ? 3 : tl) + 2;
    if (*e->dev) {
        int dl = gfx_text_wrap(F_REG, 15, ix, y, iw, 19, 2, 0, C_DIM, e->dev);
        y += 19 * (dl > 2 ? 2 : dl) + 4;
    }
    int x = ix;
    x += plat_badge(x, y + 3, e->plat) + 6;
    char meta[64];
    snprintf(meta, sizeof meta, "%s%s%s", type_name(e->type), *e->date ? "  ·  " : "", e->date);
    gfx_text(F_REG, 14, x, y + 1, C_DIM, meta);
    y += 24;
    if (*e->license) {
        char l[96];
        snprintf(l, sizeof l, "License: %s", e->license);
        gfx_text_fit(F_REG, 13, ix, y, iw, C_FAINT, l);
        y += 18;
    }
    if (*e->tags) {
        gfx_text_fit(F_REG, 13, ix, y, iw, C_FAINT, e->tags);
        y += 18;
    }
    y += 6;
    gfx_rect(ix, y, iw, 1, C_PANEL2);
    y += 8;
    int bottom = SCR_H - 34;
    int maxl = (bottom - y) / 19;
    const char *desc = *e->desc ? e->desc : "No description.";
    gfx_clip(ix, y, iw + 8, bottom - y);
    det_lines = gfx_text_wrap(F_REG, 15, ix, y, iw, 19, maxl, det_scroll, 0xFFCDD0DE, desc);
    gfx_noclip();
    if (det_lines > maxl) {
        int th = bottom - y, bh = th * maxl / det_lines;
        int by = y + (th - bh) * det_scroll / (det_lines - maxl);
        gfx_rrect(SCR_W - 8, by, 3, bh, 1, C_FAINT);
    }
    if (det_scroll > det_lines - maxl) det_scroll = det_lines - maxl < 0 ? 0 : det_lines - maxl;

    if (e->installed) {
        const char *f[] = {"Y", "Remove", "L1/R1", "Screens", "UP/DN", "Scroll", "B", "Back"};
        draw_footer(f, 4);
    } else {
        const char *f[] = {"A", "Install", "L1/R1", "Screens", "UP/DN", "Scroll", "B", "Back"};
        draw_footer(f, 4);
    }

    if (modal == 1) {
        gfx_rect(0, 0, SCR_W, SCR_H, C_SHADE);
        int w = 420, h = 150, mx = (SCR_W - w) / 2, my = (SCR_H - h) / 2;
        gfx_rrect(mx, my, w, h, 14, C_PANEL);
        gfx_text(F_BOLD, 19, mx + 20, my + 18, C_TEXT, "Remove this game?");
        gfx_text_fit(F_REG, 15, mx + 20, my + 50, w - 40, C_DIM, dest);
        gfx_text(F_REG, 14, mx + 20, my + 72, C_FAINT, "Your save files are kept.");
        pill(mx + 20, my + 106, "A  Remove", C_ERR, 0xFF0F1018);
        pill(mx + 140, my + 106, "B  Cancel", C_PANEL2, C_TEXT);
    }
}

/* ---------- on-screen keyboard ---------- */
typedef struct {
    int row, col, span;
    const char *label;
    char ch; /* 0 = special */
} Key;

static Key keys[64];
static int nkeys;

static void kb_build(void)
{
    const char *rows[] = {"ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ0123", "456789-'!&"};
    static char labels[40][2];
    nkeys = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 10; c++) {
            int li = r * 10 + c;
            labels[li][0] = rows[r][c];
            labels[li][1] = 0;
            keys[nkeys++] = (Key){r, c, 1, labels[li], rows[r][c]};
        }
    keys[nkeys++] = (Key){4, 0, 3, "SPACE", 0};
    keys[nkeys++] = (Key){4, 3, 2, "DEL", 0};
    keys[nkeys++] = (Key){4, 5, 2, "CLEAR", 0};
    keys[nkeys++] = (Key){4, 7, 3, "DONE", 0};
}

static Key *kb_at(int row, int col)
{
    for (int i = 0; i < nkeys; i++)
        if (keys[i].row == row && col >= keys[i].col && col < keys[i].col + keys[i].span) return &keys[i];
    return NULL;
}

static void query_changed(void)
{
    rebuild_view();
    sel = 0, top = 0;
}

static void kb_press(Key *k)
{
    size_t n = strlen(query);
    if (k->ch) {
        if (n < sizeof query - 1) query[n] = (char)tolower((unsigned char)k->ch), query[n + 1] = 0;
        query_changed();
    } else if (!strcmp(k->label, "SPACE")) {
        if (n && n < sizeof query - 1 && query[n - 1] != ' ') query[n] = ' ', query[n + 1] = 0;
    } else if (!strcmp(k->label, "DEL")) {
        if (n) query[n - 1] = 0, query_changed();
    } else if (!strcmp(k->label, "CLEAR")) {
        query[0] = 0, query_changed();
    } else if (!strcmp(k->label, "DONE")) {
        searching = 0;
    }
    dirty = 1;
}

static void draw_keyboard(void)
{
    gfx_rect(0, 0, SCR_W, SCR_H, C_SHADE);
    int w = 520, h = 330, x0 = (SCR_W - w) / 2, y0 = 70;
    gfx_rrect(x0, y0, w, h, 14, C_PANEL);
    gfx_text(F_BOLD, 18, x0 + 20, y0 + 16, C_TEXT, "Search");
    char cnt[48];
    snprintf(cnt, sizeof cnt, "%d match%s", nview, nview == 1 ? "" : "es");
    gfx_text(F_REG, 14, x0 + w - 20 - gfx_text_w(F_REG, 14, cnt), y0 + 19, C_DIM, cnt);
    gfx_rrect(x0 + 20, y0 + 46, w - 40, 36, 8, C_BG);
    int qw = gfx_text_fit(F_BOLD, 18, x0 + 32, y0 + 52, w - 70, C_TEXT, query);
    if ((now_ms / 500) % 2) gfx_rect(x0 + 34 + qw, y0 + 54, 2, 20, C_GOLD);
    if (!*query) gfx_text(F_REG, 16, x0 + 32, y0 + 54, C_FAINT, "title, developer or tag");

    int kx = x0 + 20, ky = y0 + 96, kw = 46, kh = 38, gap = 2;
    Key *cur = kb_at(kb_row, kb_col);
    for (int i = 0; i < nkeys; i++) {
        Key *k = &keys[i];
        int x = kx + k->col * (kw + gap), y = ky + k->row * (kh + gap);
        int ww = k->span * (kw + gap) - gap;
        int on = k == cur;
        gfx_rrect(x, y, ww, kh, 7, on ? C_GOLD : C_PANEL2);
        int font = k->ch ? F_BOLD : F_PIXEL, size = k->ch ? 18 : 8;
        int tw = gfx_text_w(font, size, k->label);
        gfx_text(font, size, x + (ww - tw) / 2, y + (k->ch ? 9 : 15), on ? 0xFF0F1018 : C_TEXT, k->label);
    }
    const char *f[] = {"A", "Type", "B", "Delete", "X", "Space", "START", "Done"};
    draw_footer(f, 4);
}

static void render(void)
{
    if (screen == SCR_LOADING) draw_loading();
    else if (screen == SCR_BROWSE) draw_browse();
    else draw_detail();
    if (searching) draw_keyboard();
    draw_toast();
}

/* ---------- input ---------- */
static void on_button(int b)
{
    dirty = 1;
    if (b == BTN_MENU) { running = 0; return; }

    if (screen == SCR_LOADING) {
        if (b == BTN_A && !job_busy(&jcat)) start_catalog_fetch();
        if (b == BTN_B) running = 0;
        return;
    }

    if (searching) {
        Key *k = kb_at(kb_row, kb_col);
        switch (b) {
        case BTN_UP: if (kb_row > 0) kb_row--; break;
        case BTN_DOWN: if (kb_row < 4) kb_row++; break;
        case BTN_LEFT:
            if (k && k->col > 0) kb_col = k->col - 1;
            else kb_col = 9;
            if (kb_row == 4 && (k = kb_at(kb_row, kb_col))) kb_col = k->col;
            break;
        case BTN_RIGHT:
            if (k && k->col + k->span < 10) kb_col = k->col + k->span;
            else kb_col = 0;
            break;
        case BTN_A: if (k) kb_press(k); break;
        case BTN_B:
            if (*query) query[strlen(query) - 1] = 0, query_changed();
            else searching = 0;
            break;
        case BTN_X: {
            Key sp = {0, 0, 0, "SPACE", 0};
            kb_press(&sp);
            break;
        }
        case BTN_SELECT: query[0] = 0, query_changed(); break;
        case BTN_START: case BTN_Y: searching = 0; break;
        }
        return;
    }

    if (screen == SCR_DETAIL) {
        int ei = cur_entry();
        Entry *e = ei >= 0 ? &cat.e[ei] : NULL;
        if (modal == 1) {
            if (b == BTN_A && e) remove_game(ei);
            if (b == BTN_A || b == BTN_B) modal = 0;
            return;
        }
        switch (b) {
        case BTN_B: screen = SCR_BROWSE; break;
        case BTN_A: if (e) start_install(ei); break;
        case BTN_X: if (e) start_install(ei); break;
        case BTN_Y: if (e && e->installed && ei != jinst_e) modal = 1; break;
        case BTN_L1: case BTN_LEFT:
            if (e && e->nshots) det_shot = (det_shot + e->nshots - 1) % e->nshots;
            break;
        case BTN_R1: case BTN_RIGHT:
            if (e && e->nshots) det_shot = (det_shot + 1) % e->nshots;
            break;
        case BTN_UP: if (det_scroll > 0) det_scroll--; break;
        case BTN_DOWN: det_scroll++; break;
        case BTN_L2: case BTN_R2: {
            /* jump to previous/next game without leaving the page */
            int s = sel + (b == BTN_R2 ? 1 : -1);
            if (s >= 0 && s < nview) { move_sel(s - sel); det_shot = 0, det_scroll = 0; }
            break;
        }
        }
        return;
    }

    /* browse */
    switch (b) {
    case BTN_UP: move_sel(-1); break;
    case BTN_DOWN: move_sel(1); break;
    case BTN_LEFT: move_sel(-ROWS); break;
    case BTN_RIGHT: move_sel(ROWS); break;
    case BTN_L1: tab = (tab + 4) % 5; rebuild_view(); break;
    case BTN_R1: tab = (tab + 1) % 5; rebuild_view(); break;
    case BTN_L2: typef = (typef + TF_COUNT - 1) % TF_COUNT; rebuild_view(); break;
    case BTN_R2: typef = (typef + 1) % TF_COUNT; rebuild_view(); break;
    case BTN_SELECT:
        sortm = (sortm + 1) % SORT_COUNT;
        if (sortm == SORT_RANDOM)
            for (int i = 0; i < cat.n; i++) cat.e[i].rnd = (unsigned)rand();
        rebuild_view();
        sel = 0, top = 0;
        toast("Sorted: %s", sort_names[sortm]);
        break;
    case BTN_A:
        if (nview) { screen = SCR_DETAIL; det_shot = 0; det_scroll = 0; }
        break;
    case BTN_X: if (nview) start_install(view[sel]); break;
    case BTN_Y: searching = 1; kb_row = 0; kb_col = 0; break;
    case BTN_START:
        if (!job_busy(&jcat)) { start_catalog_fetch(); toast("Refreshing catalog...", NULL); }
        break;
    case BTN_B: running = 0; break;
    }
}

/* ---------- scripted runs (desktop testing / screenshots) ---------- */
static const char *script;
static uint32_t script_wait;

static int btn_by_name(const char *n, int len)
{
    static const char *names[] = {"up", "down", "left", "right", "a", "b", "x", "y", "l1", "r1",
                                  "l2", "r2", "select", "start", "menu"};
    for (int i = 0; i < BTN_COUNT; i++)
        if ((int)strlen(names[i]) == len && !strncmp(names[i], n, len)) return i;
    return -1;
}

static void script_step(void)
{
    if (!script || now_ms < script_wait) return;
    while (*script == ',' || *script == ' ') script++;
    if (!*script) { running = 0; return; }
    const char *end = strchr(script, ',');
    int len = end ? (int)(end - script) : (int)strlen(script);
    char tok[256];
    snprintf(tok, sizeof tok, "%.*s", len, script);
    script += len;
    if (!strncmp(tok, "wait:", 5)) {
        script_wait = now_ms + atoi(tok + 5);
    } else if (!strcmp(tok, "idle")) {
        /* wait until downloads settle */
        if (job_busy(&jcat) || job_busy(&jshot) || job_busy(&jinst)) script -= len;
        script_wait = now_ms + 100;
    } else if (!strncmp(tok, "shot:", 5)) {
        render();
        canvas_save_png(tok + 5);
        printf("saved %s\n", tok + 5);
    } else if (!strncmp(tok, "type:", 5)) {
        snprintf(query, sizeof query, "%s", tok + 5);
        query_changed();
    } else {
        int b = btn_by_name(tok, len);
        if (b >= 0) on_button(b);
        else fprintf(stderr, "script: unknown token %s\n", tok);
        script_wait = now_ms + 60;
    }
}

static void on_crash(int sig)
{
    char m[64];
    int n = snprintf(m, sizeof m, "[main] crashed with signal %d\n", sig);
    write(2, m, n);
    _exit(128 + sig);
}

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
    setvbuf(stderr, NULL, _IONBF, 0);
    signal(SIGSEGV, on_crash);
    signal(SIGBUS, on_crash);
    signal(SIGILL, on_crash);
    signal(SIGFPE, on_crash);
    signal(SIGPIPE, SIG_IGN);
#ifdef __linux__
    /* run from the binary's own folder so res/ and cache/ are found */
    char exe[512];
    ssize_t el = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (el > 0 && access("res/Atkinson-Regular.ttf", F_OK) != 0) {
        exe[el] = 0;
        char *sl = strrchr(exe, '/');
        if (sl && sl != exe) { *sl = 0; if (chdir(exe)) {} }
    }
#endif
    char cwd[512];
    fprintf(stderr, "[main] Homebrew Store starting, cwd=%s\n", getcwd(cwd, sizeof cwd) ? cwd : "?");
    const char *rr = getenv("HBS_ROMS");
    if (rr) snprintf(roms_root, sizeof roms_root, "%s", rr);
    else if (file_exists("/mnt/SDCARD/Roms")) snprintf(roms_root, sizeof roms_root, "/mnt/SDCARD/Roms");
    else snprintf(roms_root, sizeof roms_root, "sdcard/Roms");

    if (gfx_init("res")) {
        fprintf(stderr, "Couldn't load fonts from res/ (run from the app folder)\n");
        return 1;
    }
    mkdir("cache", 0777);
    kb_build();
    script = getenv("HBS_SCRIPT");
    int headless = script != NULL;

    fprintf(stderr, "[main] fonts loaded, roms=%s\n", roms_root);
    if (!headless && plat_init()) {
        fprintf(stderr, "[main] display init failed, exiting\n");
        return 1;
    }
    fprintf(stderr, "[main] display ready\n");
    now_ms = ticks();

    if (net_init()) {
        snprintf(load_err, sizeof load_err, "curl not found (this app needs Onion OS)");
    }
    fprintf(stderr, "[main] curl=%s\n", *net_curl_path() ? net_curl_path() : "(missing)");
    if (load_catalog_file("cache/catalog.json") == 0) {
        screen = SCR_BROWSE;
        cat_from_cache = 1;
        struct stat st;
        /* refresh in the background if the cached catalog is over a day old */
        if (*net_curl_path() && stat("cache/catalog.json", &st) == 0 && time(NULL) - st.st_mtime > 24 * 3600)
            start_catalog_fetch();
    } else if (*net_curl_path()) {
        load_err[0] = 0;
        start_catalog_fetch();
    }

    int held[BTN_COUNT] = {0};
    uint32_t next_rep[BTN_COUNT] = {0};
    uint32_t last_frame = 0;

    while (running) {
        now_ms = ticks();
        int b, down;
        if (!headless)
            while (plat_poll(&b, &down)) {
                if (b < 0) { running = 0; break; }
                held[b] = down;
                if (down) {
                    next_rep[b] = now_ms + 320;
                    on_button(b);
                }
            }
        /* key repeat for navigation */
        for (int k = BTN_UP; k <= BTN_RIGHT; k++)
            if (held[k] && now_ms >= next_rep[k]) {
                next_rep[k] = now_ms + 55;
                on_button(k);
            }
        script_step();
        update_jobs();

        int animating = job_busy(&jcat) || job_busy(&jshot) || job_busy(&jinst) || searching ||
                        (toast_until && now_ms < toast_until + 50);
        if (toast_until && now_ms >= toast_until && now_ms < toast_until + 100) dirty = 1;
        if (!headless && (dirty || (animating && now_ms - last_frame > 50))) {
            render();
            plat_present(g_px);
            dirty = 0;
            last_frame = now_ms;
        }
        usleep(headless ? 5000 : 12000);
    }

    job_cancel(&jshot);
    job_cancel(&jinst);
    job_cancel(&jcat);
    if (!headless) plat_quit();
    return 0;
}
