/*
 * Xenia Nexus — xenia-edge config backend (xcfg.c).
 *
 * Minimal, round-trip-safe TOML reader/writer for xenia-edge's config file.
 * xenia keeps two separate TOML files:
 *   global:   <storage_root>/xenia-edge.config.toml
 *   per-game: <storage_root>/config/<TITLEID_8HEX>.config.toml
 *
 * storage_root mirrors xenia-edge exactly:
 *   $XDG_DATA_HOME/Xenia   (or $HOME/.local/share/Xenia if XDG_DATA_HOME unset)
 *   (portable.txt logic is irrelevant — AppImage mount is read-only)
 *
 * xcfg_save uses a line-oriented read-modify-write so it only touches the
 * managed keys and leaves every other cvar, comment, and [Config] defaults_date
 * untouched — avoiding both data loss and xenia's defaults-migration reset.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef NEXUS_UI_XCFG_H
#define NEXUS_UI_XCFG_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Maximum number of key=value pairs held in one XCfg (managed + any keys
 * we pre-read so xcfg_save can round-trip them).  Sized to cover xenia's
 * full config file (~250 cvars) comfortably. */
#define XCFG_MAX 512

/* Value types — determines how xcfg_save formats the written line. */
typedef enum {
    XCFG_BOOL,      /* true / false (bare) */
    XCFG_INT,       /* signed integer (bare) */
    XCFG_UINT,      /* unsigned integer (bare) */
    XCFG_DOUBLE,    /* floating-point (bare) */
    XCFG_STRING,    /* "quoted" */
} XCfgType;

typedef struct XCfg {
    char path[1024];
    int  n;
    char key[XCFG_MAX][64];     /* "Section.cvar_name" */
    char val[XCFG_MAX][256];    /* raw (unquoted) string value */
    XCfgType type[XCFG_MAX];
    bool dirty;
} XCfg;

/* Resolve the global config path into out (size n). */
void xcfg_path(char *out, size_t n);

/* Resolve a per-game config path for title_id (8 hex digits) into out. */
void xcfg_game_path(uint32_t title_id, char *out, size_t n);

/* Load a xenia TOML config from path (creates an empty XCfg if not found). */
void xcfg_load(XCfg *c, const char *path);

/*
 * Save changes back to path using line-oriented read-modify-write:
 *   – keys already in the file are updated in-place under their [Section]
 *   – new keys are appended (with their [Section] header if needed)
 *   – every other line (other cvars, comments, [Config] defaults_date) is
 *     copied verbatim
 * Returns true on success.
 */
bool xcfg_save(XCfg *c, const char *path);

/* Get the raw (unquoted) value for "Section.key", NULL if absent. */
const char *xcfg_get(const XCfg *c, const char *key);

/* Set a value.  type is used only on new keys (existing keys keep their
 * original type inferred at load time). */
void xcfg_set(XCfg *c, const char *key, const char *val, XCfgType type);

/* Remove a key (makes it fall back to xenia's default). */
void xcfg_unset(XCfg *c, const char *key);

/* Typed getters — convert the stored string. */
bool   xcfg_bool  (const XCfg *c, const char *key, bool def);
int    xcfg_int   (const XCfg *c, const char *key, int def);
double xcfg_double(const XCfg *c, const char *key, double def);

/* Typed setters — format the value and call xcfg_set. */
void xcfg_set_bool  (XCfg *c, const char *key, bool v);
void xcfg_set_int   (XCfg *c, const char *key, int v);
void xcfg_set_double(XCfg *c, const char *key, double v);

#endif /* NEXUS_UI_XCFG_H */
