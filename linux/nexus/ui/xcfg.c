/*
 * Xenia Nexus — xenia-edge config backend.
 *
 * Line-oriented read-modify-write for xenia's TOML config file.
 * See xcfg.h for the public API and design rationale.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "xcfg.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <sys/stat.h>
#endif

/* ---------------------------------------------------------------- paths */

static void storage_root(char *out, size_t n)
{
    /* Mirror xenia-edge's GetUserFolder()/Xenia resolution exactly:
     *   $XDG_DATA_HOME/Xenia   if XDG_DATA_HOME is set and non-empty
     *   $HOME/.local/share/Xenia  otherwise
     * (src/xenia/base/filesystem_posix.cc:81 + src/xenia/config.cc SetupConfig) */
    const char *xdg = getenv("XDG_DATA_HOME");
    if (xdg && xdg[0]) {
        snprintf(out, n, "%s/Xenia", xdg);
    } else {
        const char *home = getenv("HOME");
        snprintf(out, n, "%s/.local/share/Xenia", home ? home : "");
    }
}

void xcfg_path(char *out, size_t n)
{
    char root[512];
    storage_root(root, sizeof(root));
    snprintf(out, n, "%s/xenia-edge.config.toml", root);
}

void xcfg_game_path(uint32_t title_id, char *out, size_t n)
{
    char root[512];
    storage_root(root, sizeof(root));
    snprintf(out, n, "%s/config/%08X.config.toml", root, title_id);
}

/* ---------------------------------------------------------------- helpers */

/* Strip leading/trailing ASCII whitespace in-place. */
static void trim(char *s)
{
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    int i = (int)strlen(s) - 1;
    while (i >= 0 && isspace((unsigned char)s[i])) s[i--] = '\0';
}

/* Unquote a TOML string value (strip surrounding double-quotes if present). */
static void unquote(const char *src, char *dst, size_t n)
{
    size_t len = strlen(src);
    if (len >= 2 && src[0] == '"' && src[len - 1] == '"') {
        size_t inner = len - 2;
        if (inner >= n) inner = n - 1;
        memcpy(dst, src + 1, inner);
        dst[inner] = '\0';
    } else {
        snprintf(dst, n, "%s", src);
    }
}

/* Find the index of "Section.key" in c, or -1. */
static int find_key(const XCfg *c, const char *key)
{
    for (int i = 0; i < c->n; i++) {
        if (!strcmp(c->key[i], key)) return i;
    }
    return -1;
}

/* Infer type from a bare TOML value string. */
static XCfgType infer_type(const char *v)
{
    if (!strcmp(v, "true") || !strcmp(v, "false")) return XCFG_BOOL;
    if (v[0] == '"') return XCFG_STRING;
    /* integer or double */
    bool has_dot = (strchr(v, '.') != NULL);
    return has_dot ? XCFG_DOUBLE : XCFG_INT;
}

/* Format a value line.  Returns bytes written (not incl. NUL). */
static int fmt_line(char *out, size_t n, const char *cvar, const char *val, XCfgType type)
{
    if (type == XCFG_STRING) {
        /* Only add quotes if not already present. */
        if (val[0] == '"') {
            return snprintf(out, n, "%s = %s\n", cvar, val);
        }
        return snprintf(out, n, "%s = \"%s\"\n", cvar, val);
    }
    return snprintf(out, n, "%s = %s\n", cvar, val);
}

/* ---------------------------------------------------------------- load */

void xcfg_load(XCfg *c, const char *path)
{
    memset(c, 0, sizeof(*c));
    snprintf(c->path, sizeof(c->path), "%s", path ? path : "");

    FILE *f = path ? fopen(path, "r") : NULL;
    if (!f) return;   /* new file — start empty */

    char line[512];
    char section[64] = "";
    while (fgets(line, sizeof(line), f)) {
        char l[512];
        snprintf(l, sizeof(l), "%s", line);
        trim(l);

        /* Section header */
        if (l[0] == '[' && l[strlen(l) - 1] == ']') {
            size_t len = strlen(l) - 2;
            if (len >= sizeof(section)) len = sizeof(section) - 1;
            memcpy(section, l + 1, len);
            section[len] = '\0';
            trim(section);
            continue;
        }

        /* Skip comments and blank lines */
        if (!l[0] || l[0] == '#') continue;

        /* key = value */
        char *eq = strchr(l, '=');
        if (!eq) continue;

        char cvar[64], rawval[256];
        size_t klen = (size_t)(eq - l);
        if (klen >= sizeof(cvar)) klen = sizeof(cvar) - 1;
        memcpy(cvar, l, klen);
        cvar[klen] = '\0';
        trim(cvar);

        snprintf(rawval, sizeof(rawval), "%s", eq + 1);
        trim(rawval);

        /* Strip inline comment */
        if (rawval[0] != '"') {
            char *hash = strchr(rawval, '#');
            if (hash) { *hash = '\0'; trim(rawval); }
        }

        if (!cvar[0]) continue;

        char flat[128];
        if (section[0]) {
            snprintf(flat, sizeof(flat), "%s.%s", section, cvar);
        } else {
            snprintf(flat, sizeof(flat), "%s", cvar);
        }

        /* Avoid duplicate keys (keep first occurrence) */
        if (find_key(c, flat) >= 0) continue;
        if (c->n >= XCFG_MAX) continue;

        int i = c->n++;
        snprintf(c->key[i], sizeof(c->key[i]), "%s", flat);

        /* Store unquoted value */
        char uqval[256];
        unquote(rawval, uqval, sizeof(uqval));
        snprintf(c->val[i], sizeof(c->val[i]), "%s", uqval);
        c->type[i] = infer_type(rawval);
    }
    fclose(f);
}

/* ---------------------------------------------------------------- save */

/*
 * Line-oriented read-modify-write:
 *   Pass 1: stream the file, updating lines for managed dirty keys.
 *   Pass 2: append any new keys grouped by section.
 */
bool xcfg_save(XCfg *c, const char *path)
{
    if (!path || !path[0]) return false;

    /* Mark which keys have been written during pass 1. */
    bool written[XCFG_MAX] = {false};

    /* Read the existing file into memory. */
    FILE *rf = fopen(path, "r");
    char *fbuf = NULL;
    size_t flen = 0;
    if (rf) {
        fseek(rf, 0, SEEK_END);
        flen = (size_t)ftell(rf);
        rewind(rf);
        fbuf = malloc(flen + 1);
        if (fbuf) {
            flen = fread(fbuf, 1, flen, rf);
            fbuf[flen] = '\0';
        }
        fclose(rf);
    }

    /* Ensure the config directory exists. */
#ifdef __linux__
    {
        char dir[1024];
        snprintf(dir, sizeof(dir), "%s", path);
        char *slash = strrchr(dir, '/');
        if (slash) {
            *slash = '\0';
            /* Create all directories in the path */
            for (char *p = dir + 1; *p; p++) {
                if (*p == '/') {
                    *p = '\0';
                    mkdir(dir, 0755);
                    *p = '/';
                }
            }
            mkdir(dir, 0755);
        }
    }
#endif

    /* Write to a temp file, then rename atomically. */
    char tmppath[1100];
    snprintf(tmppath, sizeof(tmppath), "%s.nexustmp", path);
    FILE *wf = fopen(tmppath, "w");
    if (!wf) { free(fbuf); return false; }

    if (fbuf) {
        /* Pass 1: stream lines, rewriting managed values. */
        char section[64] = "";
        char *p = fbuf;
        while (*p) {
            char *nl = strchr(p, '\n');
            size_t llen = nl ? (size_t)(nl - p + 1) : strlen(p);
            char line[512];
            size_t copy = llen < sizeof(line) - 1 ? llen : sizeof(line) - 2;
            memcpy(line, p, copy);
            line[copy] = '\0';

            char trimmed[512];
            snprintf(trimmed, sizeof(trimmed), "%s", line);
            /* trim trailing newline for analysis */
            {
                int ti = (int)strlen(trimmed) - 1;
                while (ti >= 0 && (trimmed[ti] == '\n' || trimmed[ti] == '\r')) trimmed[ti--] = '\0';
            }
            trim(trimmed);

            /* Section header? */
            if (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']') {
                size_t slen = strlen(trimmed) - 2;
                if (slen >= sizeof(section)) slen = sizeof(section) - 1;
                memcpy(section, trimmed + 1, slen);
                section[slen] = '\0';
                trim(section);
                fwrite(p, 1, llen, wf);
                p += llen;
                continue;
            }

            /* key = value line? */
            char *eq = strchr(trimmed, '=');
            if (eq && trimmed[0] != '#' && trimmed[0] != '\0') {
                char cvar[64];
                size_t klen = (size_t)(eq - trimmed);
                if (klen >= sizeof(cvar)) klen = sizeof(cvar) - 1;
                memcpy(cvar, trimmed, klen);
                cvar[klen] = '\0';
                trim(cvar);

                char flat[128];
                if (section[0]) snprintf(flat, sizeof(flat), "%s.%s", section, cvar);
                else snprintf(flat, sizeof(flat), "%s", cvar);

                int idx = find_key(c, flat);
                if (idx >= 0) {
                    written[idx] = true;
                    /* Write updated line */
                    char newline[512];
                    fmt_line(newline, sizeof(newline), cvar, c->val[idx], c->type[idx]);
                    fputs(newline, wf);
                    p += llen;
                    continue;
                }
            }

            /* Pass through verbatim */
            fwrite(p, 1, llen, wf);
            p += llen;
        }
    }

    /* Pass 2: append new keys (not yet written) grouped by section. */
    {
        /* Collect unique sections needed. */
        char new_sections[32][64];
        int ns = 0;
        for (int i = 0; i < c->n; i++) {
            if (written[i]) continue;
            /* Extract section from "Section.key" */
            const char *dot = strchr(c->key[i], '.');
            if (!dot) continue; /* no section — skip (shouldn't happen) */
            char sec[64];
            size_t slen = (size_t)(dot - c->key[i]);
            if (slen >= sizeof(sec)) slen = sizeof(sec) - 1;
            memcpy(sec, c->key[i], slen);
            sec[slen] = '\0';
            /* Already in our list? */
            bool found = false;
            for (int j = 0; j < ns; j++) {
                if (!strcmp(new_sections[j], sec)) { found = true; break; }
            }
            if (!found && ns < 32) {
                snprintf(new_sections[ns++], 64, "%s", sec);
            }
        }
        /* Append per section */
        for (int s = 0; s < ns; s++) {
            fprintf(wf, "\n[%s]\n", new_sections[s]);
            for (int i = 0; i < c->n; i++) {
                if (written[i]) continue;
                const char *dot = strchr(c->key[i], '.');
                if (!dot) continue;
                char sec[64];
                size_t slen = (size_t)(dot - c->key[i]);
                if (slen >= sizeof(sec)) slen = sizeof(sec) - 1;
                memcpy(sec, c->key[i], slen);
                sec[slen] = '\0';
                if (strcmp(sec, new_sections[s])) continue;
                char newline[512];
                fmt_line(newline, sizeof(newline), dot + 1, c->val[i], c->type[i]);
                fputs(newline, wf);
                written[i] = true;
            }
        }
    }

    fclose(wf);
    free(fbuf);

    /* Atomic rename */
    if (rename(tmppath, path) != 0) {
        remove(tmppath);
        return false;
    }
    c->dirty = false;
    return true;
}

/* ---------------------------------------------------------------- accessors */

const char *xcfg_get(const XCfg *c, const char *key)
{
    int i = find_key(c, key);
    return i >= 0 ? c->val[i] : NULL;
}

void xcfg_set(XCfg *c, const char *key, const char *val, XCfgType type)
{
    int i = find_key(c, key);
    if (i < 0) {
        if (c->n >= XCFG_MAX) return;
        i = c->n++;
        snprintf(c->key[i], sizeof(c->key[i]), "%s", key);
        c->type[i] = type;
    }
    snprintf(c->val[i], sizeof(c->val[i]), "%s", val ? val : "");
    c->dirty = true;
}

void xcfg_unset(XCfg *c, const char *key)
{
    int i = find_key(c, key);
    if (i < 0) return;
    /* Shift remaining entries down */
    for (int j = i; j < c->n - 1; j++) {
        memcpy(c->key[j], c->key[j + 1], sizeof(c->key[j]));
        memcpy(c->val[j], c->val[j + 1], sizeof(c->val[j]));
        c->type[j] = c->type[j + 1];
    }
    c->n--;
    c->dirty = true;
}

bool xcfg_bool(const XCfg *c, const char *key, bool def)
{
    const char *v = xcfg_get(c, key);
    if (!v) return def;
    return !strcmp(v, "true") || !strcmp(v, "1");
}

int xcfg_int(const XCfg *c, const char *key, int def)
{
    const char *v = xcfg_get(c, key);
    return v ? atoi(v) : def;
}

double xcfg_double(const XCfg *c, const char *key, double def)
{
    const char *v = xcfg_get(c, key);
    return v ? atof(v) : def;
}

void xcfg_set_bool(XCfg *c, const char *key, bool v)
{
    xcfg_set(c, key, v ? "true" : "false", XCFG_BOOL);
}

void xcfg_set_int(XCfg *c, const char *key, int v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", v);
    xcfg_set(c, key, buf, XCFG_INT);
}

void xcfg_set_double(XCfg *c, const char *key, double v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%g", v);
    xcfg_set(c, key, buf, XCFG_DOUBLE);
}
