#include "catalog.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../vendor/cJSON.h"

const char *plat_name(int p)
{
    static const char *n[] = {"GB", "GBC", "GBA", "NES"};
    return p >= 0 && p < P_COUNT ? n[p] : "?";
}

const char *plat_dir(int p)
{
    static const char *n[] = {"GB", "GBC", "GBA", "FC"};
    return p >= 0 && p < P_COUNT ? n[p] : "GB";
}

const char *type_name(int t)
{
    static const char *n[] = {"Game", "Demo", "Music", "Tool", "Other"};
    return t >= 0 && t <= T_OTHER ? n[t] : "Other";
}

static char *dupstr(const char *s) { return strdup(s ? s : ""); }

static const char *jstr(const cJSON *o, const char *k)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k);
    return cJSON_IsString(v) ? v->valuestring : NULL;
}

/* developer can be a string, an object {name}, or a list of either */
static char *dev_string(const cJSON *d)
{
    if (cJSON_IsString(d)) return dupstr(d->valuestring);
    if (cJSON_IsObject(d)) return dupstr(jstr(d, "name"));
    if (cJSON_IsArray(d)) {
        char buf[256] = "";
        const cJSON *it;
        cJSON_ArrayForEach(it, d)
        {
            const char *n = cJSON_IsString(it) ? it->valuestring : cJSON_IsObject(it) ? jstr(it, "name") : NULL;
            if (!n || !*n) continue;
            if (*buf) strncat(buf, ", ", sizeof buf - strlen(buf) - 1);
            strncat(buf, n, sizeof buf - strlen(buf) - 1);
        }
        return dupstr(buf);
    }
    return dupstr("");
}

static const char *ext_of(const char *f)
{
    const char *d = strrchr(f, '.');
    return d ? d + 1 : "";
}

static int rom_plat(const char *ext, const char *platform, const char *basepath)
{
    if (!strcasecmp(ext, "gba")) return P_GBA;
    if (!strcasecmp(ext, "nes")) return P_NES;
    if (!strcasecmp(ext, "gbc") || !strcasecmp(ext, "cgb")) return P_GBC;
    if (!strcasecmp(ext, "gb") || !strcasecmp(ext, "dmg")) {
        if (platform && !strcasecmp(platform, "GBC")) return P_GBC;
        return P_GB;
    }
    (void)basepath;
    return -1;
}

static int type_of(const char *t)
{
    if (!t) return T_OTHER;
    if (!strcmp(t, "game")) return T_GAME;
    if (!strcmp(t, "demo")) return T_DEMO;
    if (!strcmp(t, "music")) return T_MUSIC;
    if (!strcmp(t, "tool") || !strcmp(t, "tools")) return T_TOOL;
    return T_OTHER;
}

/* Filesystem-safe ASCII name for the SD card (FAT32, Onion's font). */
static void safe_name(char *out, int outlen, const char *title, const char *slug)
{
    int o = 0, sp = 0;
    for (const unsigned char *p = (const unsigned char *)title; *p && o < outlen - 1 && o < 80; p++) {
        unsigned c = *p;
        if (c >= 0x80 || c < 0x20 || strchr("/\\:*?\"<>|", c)) {
            if (c >= 0x80) continue; /* drop non-ASCII */
            c = ' ';
        }
        if (c == ' ') {
            if (sp || o == 0) continue;
            sp = 1;
        } else sp = 0;
        out[o++] = (char)c;
    }
    while (o > 0 && (out[o - 1] == ' ' || out[o - 1] == '.')) o--;
    out[o] = 0;
    if (!o) snprintf(out, outlen, "%s", slug);
}

static void trim(char *s)
{
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = 0;
}

static char *trim_desc(const char *s)
{
    if (!s) return dupstr("");
    char *d = malloc(strlen(s) + 1), *o = d;
    for (; *s; s++)
        if (*s != '\r') *o++ = *s;
    *o = 0;
    while (o > d && (o[-1] == '\n' || o[-1] == ' ')) *--o = 0;
    return d;
}

int catalog_load(Catalog *c, const char *path, char *err, int errlen)
{
    memset(c, 0, sizeof *c);
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(err, errlen, "No catalog yet"); return -1; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf || fread(buf, 1, len, f) != (size_t)len) {
        fclose(f);
        free(buf);
        snprintf(err, errlen, "Couldn't read catalog");
        return -1;
    }
    fclose(f);
    buf[len] = 0;
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    const cJSON *entries = root ? cJSON_GetObjectItemCaseSensitive(root, "entries") : NULL;
    if (!cJSON_IsArray(entries)) {
        cJSON_Delete(root);
        snprintf(err, errlen, "Catalog data looks broken");
        return -1;
    }
    int cap = cJSON_GetArraySize(entries);
    c->e = calloc(cap ? cap : 1, sizeof(Entry));
    const cJSON *it;
    cJSON_ArrayForEach(it, entries)
    {
        const char *slug = jstr(it, "slug");
        const char *basepath = jstr(it, "basepath");
        if (!slug || !basepath) continue;

        /* pick the ROM: default playable file with a known extension */
        const cJSON *files = cJSON_GetObjectItemCaseSensitive(it, "files"), *fi;
        const char *rom = NULL;
        int plat = -1;
        cJSON_ArrayForEach(fi, files)
        {
            const char *fn = jstr(fi, "filename");
            if (!fn) continue;
            int p = rom_plat(ext_of(fn), jstr(it, "platform"), basepath);
            if (p < 0) continue;
            int def = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(fi, "default"));
            if (!rom || def) rom = fn, plat = p;
            if (def) break;
        }
        if (!rom) continue;

        Entry *e = &c->e[c->n++];
        e->slug = dupstr(slug);
        e->basepath = dupstr(basepath);
        e->file = dupstr(rom);
        const char *ext = ext_of(rom);
        e->ext = dupstr(!strcasecmp(ext, "cgb") ? "gbc" : !strcasecmp(ext, "dmg") ? "gb" : ext);
        for (char *p = e->ext; *p; p++) *p = (char)tolower((unsigned char)*p);
        e->plat = plat;
        e->title = dupstr(jstr(it, "title") ? jstr(it, "title") : slug);
        trim(e->title);
        if (!*e->title) { free(e->title); e->title = dupstr(slug); }
        e->dev = dev_string(cJSON_GetObjectItemCaseSensitive(it, "developer"));
        e->desc = trim_desc(jstr(it, "description"));
        e->type = type_of(jstr(it, "typetag"));
        e->license = dupstr(jstr(it, "license"));
        const char *date = jstr(it, "date");
        const char *added = jstr(it, "firstadded_date");
        e->date = dupstr(date ? date : added ? added : "");
        if (strlen(e->date) > 10) e->date[10] = 0;

        char tags[160] = "";
        const cJSON *tg;
        cJSON_ArrayForEach(tg, cJSON_GetObjectItemCaseSensitive(it, "tags"))
        {
            if (!cJSON_IsString(tg)) continue;
            if (*tags) strncat(tags, ", ", sizeof tags - strlen(tags) - 1);
            strncat(tags, tg->valuestring, sizeof tags - strlen(tags) - 1);
        }
        e->tags = dupstr(tags);

        const cJSON *sh;
        cJSON_ArrayForEach(sh, cJSON_GetObjectItemCaseSensitive(it, "screenshots"))
        {
            if (e->nshots >= MAX_SHOTS || !cJSON_IsString(sh)) continue;
            e->shots[e->nshots++] = dupstr(sh->valuestring);
        }

        char name[128];
        safe_name(name, sizeof name, e->title, slug);
        e->localname = malloc(strlen(name) + strlen(e->ext) + 2);
        sprintf(e->localname, "%s.%s", name, e->ext);
    }
    cJSON_Delete(root);

    /* two different entries with the same file name in one folder: add the slug */
    for (int i = 0; i < c->n; i++)
        for (int j = i + 1; j < c->n; j++) {
            Entry *a = &c->e[i], *b = &c->e[j];
            if (a->plat != b->plat || strcasecmp(a->localname, b->localname)) continue;
            char *nn = malloc(strlen(b->localname) + strlen(b->slug) + 8);
            char *dot = strrchr(b->localname, '.');
            *dot = 0;
            sprintf(nn, "%s (%s).%s", b->localname, b->slug, dot + 1);
            free(b->localname);
            b->localname = nn;
        }

    if (!c->n) { snprintf(err, errlen, "Catalog is empty"); return -1; }
    return 0;
}

void catalog_free(Catalog *c)
{
    for (int i = 0; i < c->n; i++) {
        Entry *e = &c->e[i];
        free(e->slug), free(e->title), free(e->dev), free(e->desc), free(e->date), free(e->tags);
        free(e->license), free(e->basepath), free(e->file), free(e->ext), free(e->localname);
        for (int k = 0; k < e->nshots; k++) free(e->shots[k]);
    }
    free(c->e);
    memset(c, 0, sizeof *c);
}
