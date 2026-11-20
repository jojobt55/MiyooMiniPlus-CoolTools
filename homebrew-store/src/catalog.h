#pragma once

#define MAX_SHOTS 8
#define HUB_BASE "https://hh3.gbdev.io"
#define CATALOG_URL HUB_BASE "/api/search?results=5000&sort=title&order=asc"

enum { P_GB, P_GBC, P_GBA, P_NES, P_COUNT };
enum { T_GAME, T_DEMO, T_MUSIC, T_TOOL, T_OTHER };

typedef struct {
    char *slug, *title, *dev, *desc, *date, *tags, *license, *basepath;
    char *file;                 /* ROM path inside the entry, e.g. "files/helmet.nes" */
    char *ext;                  /* extension used on the SD card ("gb", "gbc", ...) */
    char *shots[MAX_SHOTS];
    int nshots;
    int plat, type;
    char *localname;            /* "Title.gbc" */
    int installed;
    signed char shot_state[MAX_SHOTS]; /* 0 unknown, 1 downloading, 2 cached, 3 failed */
    unsigned rnd;
} Entry;

typedef struct {
    Entry *e;
    int n;
} Catalog;

int catalog_load(Catalog *c, const char *path, char *err, int errlen);
void catalog_free(Catalog *c);

const char *plat_name(int p);   /* GB, GBC, GBA, NES */
const char *plat_dir(int p);    /* Onion Roms/ folder: GB, GBC, GBA, FC */
const char *type_name(int t);
