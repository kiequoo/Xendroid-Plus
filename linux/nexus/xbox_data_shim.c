/*
 * Xenia Nexus — xbox_data shim (xbox_data.h compatible implementation).
 *
 * Replaces xbased's xbox_data.c (XBE certificate + XPR0 disc image parser).
 * For Phase 1 we derive the game title from the filename only; no disc
 * reading, no icon extraction.  Hooks for XEX metadata and cover art are
 * planned for Phase 3.
 *
 * Compatible with the XdGame struct defined in xbox_data.h.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "ui/xbox_data.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* xd_clean_name: strip folders, extension, and (...) / [...] groups.
 * Copied verbatim from the xd_clean_name fallback logic used by xbased
 * when disc parsing fails (the same contract). */
void xd_clean_name(const char *path, char *out, size_t n)
{
    /* base name only */
    const char *base = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    /* copy, stopping at extension */
    size_t i = 0;
    for (; base[i] && base[i] != '.' && i + 1 < n; i++) {
        out[i] = base[i];
    }
    out[i] = '\0';

    /* remove (...) and [...] groups at the end */
    for (;;) {
        /* trim trailing spaces first */
        while (i > 0 && out[i - 1] == ' ') {
            out[--i] = '\0';
        }
        if (i > 0 && (out[i - 1] == ')' || out[i - 1] == ']')) {
            char close = out[i - 1], open = close == ')' ? '(' : '[';
            size_t j = i - 1;
            while (j > 0 && out[j - 1] != open) {
                j--;
            }
            if (j > 0) {
                out[j - 1] = '\0';
                i = j - 1;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

/* xd_read_game: fill XdGame from the filesystem path.
 * Returns false only on hard failure (empty path).
 * title is derived from the filename; disc metadata deferred to Phase 3. */
bool xd_read_game(const char *path, XdGame *g)
{
    if (!path || !path[0]) {
        return false;
    }
    memset(g, 0, sizeof(*g));
    snprintf(g->path, sizeof(g->path), "%s", path);

    xd_clean_name(path, g->title, sizeof(g->title));
    if (!g->title[0]) {
        snprintf(g->title, sizeof(g->title), "Unknown");
    }

    /* Derive a stable key from the path for covers/ and games/ cache dirs.
     * Use a simple djb2 hash of the full path; format matches what
     * xbox_data.c produces for unknown-ID discs: "00000000.0" */
    unsigned long h = 5381;
    for (const char *p = path; *p; p++) {
        h = h * 33 ^ (unsigned char)*p;
    }
    snprintf(g->key, sizeof(g->key), "%08lX.0", h & 0xFFFFFFFFul);

    g->title_id = 0;
    g->version  = 0;
    g->regions  = 0;
    g->year     = 0;
    g->icon     = NULL;
    g->icon_w   = 0;
    g->icon_h   = 0;
    g->size     = 0;
    g->disc_sig = 0;

    return true;
}

void xd_free_game(XdGame *g)
{
    if (g) {
        g->icon = NULL;   /* no heap allocation in this shim */
    }
}

/* UTF-16LE (n code units) to UTF-8 — used by xbased when reading cert name.
 * Nexus doesn't call this for now, but it must link. */
void xd_utf16_to_utf8(const uint16_t *s, size_t n, char *out, size_t cap)
{
    size_t i = 0, o = 0;
    while (i < n && s[i] && o + 1 < cap) {
        uint32_t c = s[i++];
        if (c < 0x80) {
            out[o++] = (char)c;
        } else if (c < 0x800) {
            if (o + 2 >= cap) { break; }
            out[o++] = (char)(0xC0 | (c >> 6));
            out[o++] = (char)(0x80 | (c & 0x3F));
        } else {
            if (o + 3 >= cap) { break; }
            out[o++] = (char)(0xE0 | (c >> 12));
            out[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (c & 0x3F));
        }
    }
    out[o] = '\0';
}

/* xd_xpr_decode: XPR0 texture decoder — not implemented in Phase 1.
 * Returns NULL so callers fall back to filename-based display. */
uint8_t *xd_xpr_decode(const uint8_t *b, size_t n, int *w, int *h)
{
    (void)b; (void)n;
    *w = *h = 0;
    return NULL;
}
