/*
 * Xenia Nexus — settings screens.
 *
 * Implements the full Settings UI (global + per-game) using xbased's existing
 * rendering framework (tab_bar, opt_choice, opt_bool, opt_int from dash.c) but
 * pointing every row at xenia-edge's cvars via the xcfg TOML backend rather
 * than at xbased's original Xbox cvars.
 *
 * Tab layout (global settings — D.editing_game == false):
 *   0  Video / Display  — resolution scale, vsync, framerate, letterbox, …
 *   1  Graphics         — GPU backend, render path, postprocess, …
 *   2  Audio            — backend, volume, XMA decoder, …
 *   3  Input            — vibration, deadzones, keyboard passthrough, …
 *   4  System           — language, country, video standard, misc, …
 *   5  Interface        — game folders (launcher-local, written to xbased INI)
 *
 * Per-game settings (scr_game_settings) render tabs 0-4 against D.xenia_game,
 * loaded from <storage_root>/config/<TITLEID>.config.toml.
 *
 * The two config domains are kept strictly separate:
 *   - Launcher-local prefs (game folders, UI lang) → D.global (xbased INI,
 *     config.c / cfg_* API)
 *   - xenia-edge settings → D.xenia_global / D.xenia_game (xcfg.c)
 *
 * Framework pieces copied verbatim from xbased's dash_settings.c:
 *   shoulder_key(), tab_bar()   (lines 130-186 of the original)
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "dash_int.h"

/* from library.c */
bool lib_has_sgdb_key(void);
void lib_reset_fetch_tried(void);
#include "ui.h"
#include "xcfg.h"
#include "config.h"
#include "input_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================ xcfg state
 *
 * We keep one global XCfg (loaded on first open, saved when the user leaves
 * the settings screen) and one per-game XCfg (loaded when scr_game_settings
 * opens a game's config).
 */
static XCfg  g_xcfg_global;
static bool  g_xcfg_global_loaded = false;

static XCfg  g_xcfg_game;
static bool  g_xcfg_game_loaded   = false;
static uint32_t g_xcfg_game_tid   = 0;  /* title_id of the loaded per-game cfg */

static XCfg *xcfg_edited(void)
{
    return D.editing_game ? &g_xcfg_game : &g_xcfg_global;
}

/* Load (or reload) the global xenia config. */
static void xcfg_ensure_global(void)
{
    if (g_xcfg_global_loaded) return;
    char path[1024];
    xcfg_path(path, sizeof(path));
    xcfg_load(&g_xcfg_global, path);
    g_xcfg_global_loaded = true;
}

/* Save whichever config is currently being edited, if dirty. */
static void xcfg_flush(void)
{
    if (!D.editing_game && g_xcfg_global_loaded && g_xcfg_global.dirty) {
        char path[1024];
        xcfg_path(path, sizeof(path));
        xcfg_save(&g_xcfg_global, path);
    }
    if (D.editing_game && g_xcfg_game_loaded && g_xcfg_game.dirty) {
        char path[1024];
        xcfg_game_path(g_xcfg_game_tid, path, sizeof(path));
        xcfg_save(&g_xcfg_game, path);
    }
}

/* ================================================================ tab framework
 * Copied from xbased dash_settings.c:130-186 (GPL-3.0).
 */
static float shoulder_key(float x, const char *name)
{
    const float w = ui_text_width(name, 17, UI_FONT_BOLD) + 22;
    ui_round(x, 131, w, 30, 15, ui_alpha((UiColor){0x30, 0x34, 0x30, 0xFF}, 0.95f));
    ui_round_outline(x, 131, w, 30, 15, 1, UI_GREEN_DIM);
    ui_text(x + w / 2, 135, name, 17, UI_FONT_BOLD, UI_WHITE, UI_ALIGN_CENTER);
    return w;
}

static void tab_bar(const enum UiStr *tabs, int n)
{
    const float W = ui_w();
    const float sk = D.in.pad ? ui_text_width("LB", 17, UI_FONT_BOLD) + ui_text_width("RB", 17, UI_FONT_BOLD) + 68 : 0;
    float names = 0;
    for (int i = 0; i < n; i++) {
        names += ui_text_width(T(tabs[i]), 24, UI_FONT_BOLD);
    }
    const float room = W - 120 - sk, full = names + n * 44 + (n - 1) * 12;
    const float k = full > room ? room / full : 1;
    const float fs = 24 * (k > 0.7f ? k : 0.7f), pad = 44 * k, gap = 12 * k;
    float x = 60;
    if (D.in.pad) {
        x += shoulder_key(x, "LB") + 12;
    }
    for (int i = 0; i < n; i++) {
        const char *t = T(tabs[i]);
        const float w = ui_text_width(t, fs, UI_FONT_BOLD) + pad;
        const bool s = D.tab == i;
        const bool over = D.in.mx >= x && D.in.mx < x + w && D.in.my >= 124 && D.in.my < 168;
        if (over && D.in.click) {
            D.tab = i;
            D.sel[dash_top()] = 0;
        }
        ui_round(x, 124, w, 44, 22, s ? (UiColor){0x3C, 0x9A, 0x12, 0xFF} : (UiColor){0x10, 0x2C, 0x07, 0xE0});
        ui_round_outline(x, 124, w, 44, 22, s ? 2 : 1, s ? UI_GREEN_HI : UI_GREEN_DIM);
        ui_text(x + w / 2, 146 - fs * 0.58f, t, fs, UI_FONT_BOLD, s ? UI_WHITE : UI_GREY, UI_ALIGN_CENTER);
        x += w + gap;
    }
    x += 12 - gap;
    if (D.in.pad) {
        x += shoulder_key(x, "RB") + 12;
    }
    if (x < W - 200) {
        draw_button_glyph(x + 8, 132, 0, 0);
    }
    if (D.in.lb) {
        D.tab = (D.tab + n - 1) % n;
        D.sel[dash_top()] = 0;
    }
    if (D.in.rb) {
        D.tab = (D.tab + 1) % n;
        D.sel[dash_top()] = 0;
    }
}

/* ================================================================ xcfg option rows
 *
 * These parallel opt_choice / opt_bool / opt_int from dash.c but read from /
 * write to the XCfg backend instead of the xbased Config INI.
 *
 * For per-game editing (D.editing_game): showing "global" before the first
 * choice mirrors xbased's per-game opt_choice behaviour.
 */

static void xopt_choice(List *l, enum UiStr label, const char *key,
                        const char *const *vals, const char *const *names,
                        int n, const char *def)
{
    xcfg_ensure_global();
    XCfg *c = xcfg_edited();
    const char *v  = xcfg_get(c, key);
    const char *gv = D.editing_game ? xcfg_get(&g_xcfg_global, key) : NULL;

    /* Build display text */
    const char *cur = D.editing_game ? v : (v ? v : def);
    const char *show = cur;
    for (int i = 0; i < n; i++) {
        if (cur && !strcmp(cur, vals[i])) { show = names[i]; break; }
    }
    char text[256];
    if (D.editing_game && !v) {
        /* Show "global (X)" */
        const char *gs = gv ? gv : def;
        const char *gname = gs;
        for (int i = 0; i < n; i++) {
            if (gs && !strcmp(gs, vals[i])) { gname = names[i]; break; }
        }
        snprintf(text, sizeof(text), "%s (%s)", T(S_DEFAULT_GLOBAL), gname ? gname : "");
    } else {
        snprintf(text, sizeof(text), "%s", show ? show : "");
    }

    int delta = 0;
    const bool chosen = list_row(l, T(label), text, &delta);
    if (chosen) delta = 1;
    if (!delta) return;

    /* Step through values */
    int cur_idx = -1;
    const char *now = D.editing_game ? v : (v ? v : def);
    for (int i = 0; now && i < n; i++) {
        if (!strcmp(now, vals[i])) { cur_idx = i; break; }
    }
    const int slots = D.editing_game ? n + 1 : n;
    int pos = D.editing_game ? cur_idx + 1 : (cur_idx < 0 ? 0 : cur_idx);
    pos = (pos + delta + slots) % slots;
    if (D.editing_game && pos == 0) {
        xcfg_unset(c, key);
    } else {
        xcfg_set(c, key, vals[D.editing_game ? pos - 1 : pos], XCFG_STRING);
    }
}

static void xopt_bool(List *l, enum UiStr label, const char *key, bool def)
{
    xcfg_ensure_global();
    XCfg *c = xcfg_edited();
    const char *v  = xcfg_get(c, key);
    const char *gv = D.editing_game ? xcfg_get(&g_xcfg_global, key) : NULL;

    bool on;
    char text[256];
    if (D.editing_game && !v) {
        on = gv ? (!strcmp(gv, "true") || !strcmp(gv, "1")) : def;
        snprintf(text, sizeof(text), "%s (%s)", T(S_DEFAULT_GLOBAL), T(on ? S_ON : S_OFF));
    } else {
        on = v ? (!strcmp(v, "true") || !strcmp(v, "1")) : (gv ? (!strcmp(gv, "true") || !strcmp(gv, "1")) : def);
        snprintf(text, sizeof(text), "%s", T(on ? S_ON : S_OFF));
    }

    int delta = 0;
    const bool chosen = list_row(l, T(label), text, &delta);
    if (chosen) delta = 1;
    if (D.in.repeat) delta = 0;  /* booleans don't auto-repeat */
    if (!delta) return;

    if (D.editing_game && v) {
        /* on → off → global */
        if (on == (delta > 0)) {
            xcfg_unset(c, key);
            return;
        }
    }
    xcfg_set_bool(c, key, !on);
}

static void xopt_int(List *l, enum UiStr label, const char *key,
                     int lo, int hi, int step, int def, const char *unit)
{
    xcfg_ensure_global();
    XCfg *c = xcfg_edited();
    const char *v  = xcfg_get(c, key);
    const char *gv = D.editing_game ? xcfg_get(&g_xcfg_global, key) : NULL;

    int cur = v ? atoi(v) : (gv ? atoi(gv) : def);
    char text[256];
    if (D.editing_game && !v) {
        snprintf(text, sizeof(text), "%s (%d%s)", T(S_DEFAULT_GLOBAL), cur, unit ? unit : "");
    } else {
        snprintf(text, sizeof(text), "%d%s", cur, unit ? unit : "");
    }

    int delta = 0;
    list_row(l, T(label), text, &delta);
    if (delta) {
        int g = gv ? atoi(gv) : def;
        int x = v ? atoi(v) : g;
        x += delta * step;
        x = x < lo ? lo : x > hi ? hi : x;
        if (D.editing_game && x == g) {
            xcfg_unset(c, key);
        } else {
            xcfg_set_int(c, key, x);
        }
    }
}

/* Integer-enum option: values are integer strings, displayed as names. */
static void xopt_int_enum(List *l, enum UiStr label, const char *key,
                          const char *const *vals, const char *const *names,
                          int n, const char *def)
{
    xopt_choice(l, label, key, vals, names, n, def);
}

/* ================================================================ tab content */

/* ---- Video / Display ---- */
static void rows_xenia_video(List *l)
{
    /* Resolution scale 1-8 (draw_resolution_scale_x and _y edited together) */
    static const char *const sv[] = {"1","2","3","4","5","6","7","8"};
    static const char *const sn[] = {
        "1x  (640×480)",   "2x  (1280×960)",  "3x  (1920×1440)", "4x  (2560×1920)",
        "5x  (3200×2400)", "6x  (3840×2880)", "7x  (4480×3360)", "8x  (5120×3840)"
    };
    /* When we set scale_x we also set scale_y to match */
    xcfg_ensure_global();
    {
        XCfg *c = xcfg_edited();
        const char *v = xcfg_get(c, "GPU.draw_resolution_scale_x");
        const char *gv = D.editing_game ? xcfg_get(&g_xcfg_global, "GPU.draw_resolution_scale_x") : NULL;
        const char *cur = D.editing_game ? v : (v ? v : "1");
        const char *show = cur;
        for (int i = 0; i < 8; i++) {
            if (cur && !strcmp(cur, sv[i])) { show = sn[i]; break; }
        }
        char text[256];
        if (D.editing_game && !v) {
            const char *gs = gv ? gv : "1";
            const char *gname = gs;
            for (int i = 0; i < 8; i++) {
                if (!strcmp(gs, sv[i])) { gname = sn[i]; break; }
            }
            snprintf(text, sizeof(text), "%s (%s)", T(S_DEFAULT_GLOBAL), gname);
        } else {
            snprintf(text, sizeof(text), "%s", show ? show : "1x");
        }
        int delta = 0;
        const bool chosen = list_row(l, T(S_RENDER_SCALE), text, &delta);
        if (list_selected(l)) dash_help(T(S_H_SCALE), HELP_NOW);
        if (chosen) delta = 1;
        if (delta) {
            int cur_idx = -1;
            const char *now = D.editing_game ? v : (v ? v : "1");
            for (int i = 0; i < 8; i++) {
                if (now && !strcmp(now, sv[i])) { cur_idx = i; break; }
            }
            const int slots = D.editing_game ? 9 : 8;
            int pos = D.editing_game ? cur_idx + 1 : (cur_idx < 0 ? 0 : cur_idx);
            pos = (pos + delta + slots) % slots;
            if (D.editing_game && pos == 0) {
                xcfg_unset(c, "GPU.draw_resolution_scale_x");
                xcfg_unset(c, "GPU.draw_resolution_scale_y");
            } else {
                const char *nv = sv[D.editing_game ? pos - 1 : pos];
                xcfg_set(c, "GPU.draw_resolution_scale_x", nv, XCFG_INT);
                xcfg_set(c, "GPU.draw_resolution_scale_y", nv, XCFG_INT);
            }
        }
    }

    xopt_bool(l, S_WIDESCREEN,  "Console.widescreen", false);
    if (list_selected(l)) dash_help(T(S_H_WIDESCREEN), HELP_NOW);
    xopt_bool(l, S_FULLSCREEN,  "Display.fullscreen", true);
    if (list_selected(l)) dash_help(T(S_H_FULLSCREEN), HELP_NOW);
    xopt_bool(l, S_VSYNC,       "GPU.guest_display_refresh_cap", true);
    if (list_selected(l)) dash_help(T(S_H_VSYNC), HELP_NOW);

    /* Framerate limit */
    {
        static const char *const fv[] = {"0","60","120","144","240"};
        static const char *const fn[] = {"Unlimited","60 fps","120 fps","144 fps","240 fps"};
        xopt_choice(l, S_FRAMERATE, "GPU.framerate_limit", fv, fn, 5, "60");
    }

    /* Letterbox */
    xopt_bool(l, S_ASPECT, "Display.present_letterbox", true);

    /* 50 Hz mode */
    xopt_bool(l, S_PAL_GAMES, "Console.use_50Hz_mode", false);
}

/* ---- Graphics backend ---- */
static void rows_xenia_graphics(List *l)
{
    /* GPU backend */
    {
        static const char *const gv[] = {"vulkan","null"};
        static const char *const gn[] = {"Vulkan","Null (no rendering)"};
        xopt_choice(l, S_SHADERS_MODE, "GPU.gpu", gv, gn, 2, "vulkan");
    }

    /* Render target path */
    {
        static const char *const rv[] = {"performance","accuracy"};
        static const char *const rn[] = {"Performance (faster)","Accuracy (compatible)"};
        xopt_choice(l, S_PRECISION, "GPU.render_target_path", rv, rn, 2, "performance");
        if (list_selected(l)) dash_help(T(S_PREC_HELP), HELP_NOW);
    }

    /* Postprocess anti-aliasing */
    {
        static const char *const av[] = {"off","fxaa","fxaa_extreme"};
        const char *an[] = {T(S_OFF),"FXAA","FXAA Extreme"};
        xopt_choice(l, S_FXAA, "Display.postprocess_antialiasing", av, an, 3, "off");
        if (list_selected(l)) dash_help(T(S_H_FXAA), HELP_NOW);
    }

    /* Postprocess scaling / sharpening */
    {
        static const char *const sv[] = {"none","cas","fsr"};
        const char *sn[] = {T(S_OFF),"AMD CAS","AMD FSR 1"};
        xopt_choice(l, S_UPSCALE, "Display.postprocess_scaling_and_sharpening", sv, sn, 3, "none");
        if (list_selected(l)) dash_help(T(S_H_UPSCALE), HELP_NOW);
    }

    /* Readback resolve */
    xopt_bool(l, S_SHOW_FPS, "GPU.readback_resolve", false);
}

/* ---- Audio ---- */
static void rows_xenia_audio(List *l)
{
    /* APU backend */
    {
        static const char *const av[] = {"sdl","nop"};
        static const char *const an[] = {"SDL","Disabled"};
        xopt_choice(l, S_AUDIO_OUTPUT, "APU.apu", av, an, 2, "sdl");
    }

    /* Volume */
    xopt_int(l, S_VOLUME, "APU.volume", 0, 100, 5, 100, "%");
    if (list_selected(l)) dash_help(T(S_H_VOLUME), HELP_NOW);

    /* XMA decoder */
    {
        static const char *const xv[] = {"new","old","master","fake"};
        static const char *const xn[] = {"New","Old","Master","Fake (silent)"};
        xopt_choice(l, S_AUDIO_OUTPUT, "APU.xma_decoder", xv, xn, 4, "new");
    }

    xopt_bool(l, S_MUTE_BG, "APU.use_dedicated_xma_thread", true);

    /* Queued frames latency */
    xopt_int(l, S_MAXFPS, "APU.apu_max_queued_frames", 2, 64, 2, 8, "");
}

/* ---- Input ---- */
static void rows_xenia_input(List *l)
{
    xopt_bool(l, S_RUMBLE, "HID.vibration", true);
    if (list_selected(l)) dash_help(T(S_H_RUMBLE), HELP_NOW);

    xopt_int(l, S_DEADZONE, "HID.left_stick_deadzone_percentage", 0, 99, 1, 15, "%");
    if (list_selected(l)) dash_help(T(S_H_DEADZONE), HELP_NOW);

    xopt_int(l, S_ANTI_DEADZONE, "HID.right_stick_deadzone_percentage", 0, 99, 1, 15, "%");

    xopt_bool(l, S_KEYBOARD, "HID.keyboard_passthrough", false);
    xopt_bool(l, S_HK_MENU,  "UI.guide_button",          true);
}

/* ---- System ---- */
static void rows_xenia_system(List *l)
{
    /* Console language */
    {
        static const char *const lv[] = {
            "1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"
        };
        static const char *const ln[] = {
            "English","Japanese","German","French","Spanish","Italian",
            "Korean","TChinese","Portuguese","Polish","Russian","Swedish",
            "Turkish","Norwegian","Dutch","SChinese"
        };
        xopt_int_enum(l, S_CONSOLE_LANG, "Console.user_language", lv, ln, 16, "1");
        if (list_selected(l)) dash_help(T(S_H_CONSOLE_LANG), HELP_NOW);
    }

    /* Video standard */
    {
        static const char *const vv[] = {"1","2","3"};
        static const char *const vn[] = {"NTSC","NTSC-J","PAL"};
        xopt_int_enum(l, S_PAL_GAMES, "Console.video_standard", vv, vn, 3, "1");
    }

    /* License mask */
    {
        static const char *const mv[] = {"0","1","-1"};
        static const char *const mn[] = {"None (default)","Full","All"};
        xopt_choice(l, S_ONLINE_MODE, "Content.license_mask", mv, mn, 3, "0");
    }

    /* Apply title updates */
    xopt_bool(l, S_CHECK_UPDATES, "Kernel.apply_title_update", true);

    /* Discord RPC */
    xopt_bool(l, S_COVERS_ONLINE, "General.discord", true);

    /* Early precompilation */
    xopt_bool(l, S_PREBUILD, "CPU.enable_early_precompilation", true);
    if (list_selected(l)) dash_help(T(S_H_PREBUILD), HELP_NOW);
}

/* ---- Interface (launcher-local game folders) ---- */

static void SDLCALL play_file_cb(void *ud, const char *const *files, int filter)
{
    (void)ud;
    (void)filter;
    if (files && files[0]) {
        dash_play_file_later(files[0]);
    }
}

void dash_pick_play_file(void)
{
    dash_browse_file(T(S_PLAY_GAME_FILE), "iso;xiso;cci;chd", play_file_cb, NULL);
}

static char g_unlist[1024], g_unlist_q[1200];
static bool g_unlist_file;

static bool  g_sgdb_editing;
static char  g_sgdb_buf[256];

static void sgdb_edit_begin(void)
{
    g_sgdb_editing = true;
    D.typing = true;
    g_sgdb_buf[0] = '\0';
    SDL_StartTextInput(SDL_GetKeyboardFocus());
}

static void sgdb_edit_end(bool save)
{
    g_sgdb_editing = false;
    D.typing = false;
    SDL_StopTextInput(SDL_GetKeyboardFocus());
    if (!save || !g_sgdb_buf[0]) return;
    char path[1200];
    snprintf(path, sizeof(path), "%ssgdb.key", cfg_data_dir());
    FILE *f = fopen(path, "w");
    if (f) { fputs(g_sgdb_buf, f); fclose(f); }
    /* Reset fetch_tried so the next scan picks up games that need covers */
    lib_reset_fetch_tried();
}

static void unlist_yes(Dash *d)
{
    cfg_remove_value(&d->global, g_unlist_file ? "gamefile" : "gamedir", g_unlist);
    cfg_save(&d->global);
}

static void rows_interface(List *l)
{
    /* UI language */
    static const char *uv[L_COUNT + 1];
    static const char *un[L_COUNT + 1];
    uv[0] = "auto";
    un[0] = T(S_AUTO);
    for (int i = 0; i < L_COUNT; i++) {
        uv[i + 1] = i18n_code((enum UiLang)i);
        un[i + 1] = i18n_name((enum UiLang)i);
    }
    const char *before = cfg_get(&D.global, "ui.lang");
    char was[16];
    snprintf(was, sizeof(was), "%s", before ? before : "auto");
    opt_choice(l, S_UI_LANG, "ui.lang", uv, un, L_COUNT + 1, "auto");
    const char *now = cfg_get(&D.global, "ui.lang");
    if (now && strcmp(now, was)) {
        i18n_set(now);
    }
    if (list_selected(l)) dash_help(T(S_H_UI_LANG), HELP_NOW);

    opt_bool(l, S_UI_SOUNDS, "ui.sounds", true, false);
    if (list_selected(l)) dash_help(T(S_H_UI_SOUNDS), HELP_NOW);

    opt_bool(l, S_ANIMATED_BG, "ui.anim",   true, false);
    opt_bool(l, S_TRANSITIONS, "ui.fx",     true, false);
    opt_bool(l, S_SWAP_AB,     "ui.swapab", false, false);
    opt_bool(l, S_SHOW_HIDDEN, "ui.showhidden", false, false);
    opt_bool(l, S_FAV_FIRST,   "ui.favfirst",   true, false);
    opt_bool(l, S_LAST_FIRST,  "ui.lastfirst",  true, false);
    opt_bool(l, S_CLOCK_12H,   "ui.clock12",    false, false);

    /* Game folders */
    list_heading(l, T(S_GAME_FOLDERS));

    const char *dirs[16];
    const int nd = cfg_all(&D.global, "gamedir", dirs, 16);
    bool removable = false;
    for (int i = 0; i < nd; i++) {
        char label[64];
        snprintf(label, sizeof(label), "%s %d", T(S_GAME_FOLDERS), i + 1);
        const bool chosen = list_row(l, label, dirs[i], NULL);
        if (list_selected(l)) {
            dash_help(T(S_H_FOLDER), HELP_NOW);
            removable = true;
        }
        if (chosen || (list_selected(l) && D.in.x)) {
            snprintf(g_unlist, sizeof(g_unlist), "%s", dirs[i]);
            snprintf(g_unlist_q, sizeof(g_unlist_q), T(S_REMOVE_FOLDER_Q), dirs[i]);
            g_unlist_file = false;
            dash_ask(g_unlist_q, unlist_yes);
            break;
        }
    }
    if (list_row(l, T(S_ADD_GAMES), "+", NULL)) {
        dash_browse_games();
    }
    if (list_selected(l)) {
        dash_help(T(S_H_ADD_GAMES), HELP_NOW);
    }

    const char *files[32];
    const int nf = cfg_all(&D.global, "gamefile", files, 32);
    for (int i = 0; i < nf; i++) {
        char label[64];
        snprintf(label, sizeof(label), "%s %d", T(S_GAME_FILE), i + 1);
        const bool chosen = list_row(l, label, files[i], NULL);
        if (list_selected(l)) {
            dash_help(T(S_H_GAME_FILE), HELP_NOW);
            removable = true;
        }
        if (chosen || (list_selected(l) && D.in.x)) {
            snprintf(g_unlist, sizeof(g_unlist), "%s", files[i]);
            snprintf(g_unlist_q, sizeof(g_unlist_q), T(S_REMOVE_FILE_Q), files[i]);
            g_unlist_file = true;
            dash_ask(g_unlist_q, unlist_yes);
            break;
        }
    }
    if (list_row(l, T(S_PLAY_GAME_FILE), ">", NULL)) {
        dash_pick_play_file();
    }
    if (list_selected(l)) {
        dash_help(T(S_H_PLAY_FILE), HELP_NOW);
    }
    (void)removable;

    /* SteamGridDB API key — editable with the on-screen keyboard */
    list_heading(l, "SteamGridDB");
    {
        char shown[260];
        if (g_sgdb_editing) {
            snprintf(shown, sizeof(shown), "%s%s", g_sgdb_buf,
                     (int)(ui_time() * 2) % 2 ? "_" : "");
        } else if (lib_has_sgdb_key()) {
            snprintf(shown, sizeof(shown), "%s", T(S_CONFIGURED));
        } else {
            snprintf(shown, sizeof(shown), "%s", T(S_NOT_SET));
        }
        if (list_row(l, T(S_SGDB_KEY), shown, NULL) && !g_sgdb_editing) {
            sgdb_edit_begin();
        }
        if (list_selected(l)) dash_help(T(S_H_SGDB_KEY), HELP_NOW);
    }

    /* Show X button hint when a folder row is selected */
    draw_hints(T(S_SELECT), T(S_BACK), T(S_REMOVE), NULL);
}

/* ================================================================ screens */

void scr_settings(void)
{
    xcfg_ensure_global();
    const float W = ui_w();
    draw_header(T(S_SETTINGS), NULL);
    static const enum UiStr tabs[] = {
        S_VIDEO, S_AUDIO, S_SYSTEM, S_INTERFACE, S_CONTROLS
    };
    D.editing_game = false;
    tab_bar(tabs, 5);
    List l;
    list_begin(&l, 74, 190, W - 148, dash_list_h(190, 58), 58);
    switch (D.tab) {
    case 0: rows_xenia_video(&l);    break;
    case 1: rows_xenia_audio(&l);    break;
    case 2: rows_xenia_system(&l);   break;
    case 3: rows_interface(&l);      break;
    case 4:
        /* Controls: push existing screen */
        for (int p = 0; p < 4; p++) {
            char label[48], val[128];
            snprintf(label, sizeof(label), "%s %d", T(S_PLAYER), p + 1);
            im_device_name(&D.global, p, val, sizeof(val));
            int delta = 0;
            list_row(&l, label, val, &delta);
            if (list_selected(&l)) dash_help(T(S_H_PLAYER), HELP_ON_RESUME);
            if (delta) im_cycle_device(&D.global, p, delta);
        }
        /* Controller shortcut combos (EmuDeck-style defaults, Back+button).
         * Save/Load state omitted — those features are not available. */
        list_heading(&l, T(S_CONTROLS));
        {
            static const char *const k_names[IM_HOTKEYS] = {
                "menu", "save", "load", "turbo", "pause", "shot", "full", "quit"
            };
            static const bool k_show[IM_HOTKEYS] = {
                true,  /* menu  */
                false, /* save  — removed */
                false, /* load  — removed */
                true,  /* turbo */
                true,  /* pause */
                true,  /* shot  */
                true,  /* full  */
                true,  /* quit  */
            };
            const int n = im_hkpad_count();
            for (int h = 0; h < IM_HOTKEYS; h++) {
                if (!k_show[h]) continue;
                char key[32], label[96];
                snprintf(key, sizeof(key), "hkpad.%s", k_names[h]);
                snprintf(label, sizeof(label), "%s  (%s)",
                         T(im_hotkey_label(h)), T(S_HK_PAD));
                const char *v = cfg_get(&D.global, key);
                if (!v) v = im_hkpad_default(h);
                int cur = 0;
                for (int i = 0; i < n; i++) {
                    if (!SDL_strcasecmp(v, im_hkpad_value(i))) { cur = i; break; }
                }
                int delta = 0;
                if (list_row(&l, label, im_hkpad_label(cur), &delta)) delta = 1;
                if (list_selected(&l)) dash_help(T(S_H_HOTKEY_PAD), HELP_ON_RESUME);
                if (delta) {
                    cfg_set(&D.global, key,
                            im_hkpad_value((cur + (delta > 0 ? 1 : n - 1)) % n));
                    cfg_save(&D.global);
                }
            }
        }
        break;
    }
    list_end(&l);

    /* On-screen keyboard for SGDB key entry */
    if (g_sgdb_editing) {
        In in = D.in;
        if (D.in.text) {
            dash_text_type(g_sgdb_buf, sizeof(g_sgdb_buf), D.in.text);
        }
        if (D.in.key == SDLK_V && (D.in.mod & SDL_KMOD_CTRL)) {
            char *clip = SDL_GetClipboardText();
            if (clip) {
                /* Strip whitespace — API keys never contain spaces/newlines */
                char clean[256];
                size_t o = 0;
                for (const char *p = clip; *p && o < sizeof(clean) - 1; p++) {
                    if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
                        clean[o++] = *p;
                }
                clean[o] = '\0';
                snprintf(g_sgdb_buf, sizeof(g_sgdb_buf), "%s", clean);
                SDL_free(clip);
            }
        }
        if (D.in.key == SDLK_BACKSPACE || D.in.key == SDLK_DELETE || D.in.x) {
            dash_text_rub_out(g_sgdb_buf);
        }
        if (D.in.back || D.in.key == SDLK_ESCAPE) {
            sgdb_edit_end(false);
        }
        memset(&D.in, 0, sizeof(D.in));
        D.in.mx = D.in.my = -1;
        D.in.pad = in.pad;
        const float ks = 52;
        const float kx = (ui_w() - ks * 10) * 0.5f;
        const float ky = ui_h() - ks * 5 - 20;
        if (dash_osk(kx, ky, ks, &in, g_sgdb_buf, sizeof(g_sgdb_buf))) {
            sgdb_edit_end(true);
        }
    }

    dash_help_draw();

    /* Save on B / back (not while editing — B cancels the OSK there) */
    if (!g_sgdb_editing && D.in.back) {
        xcfg_flush();
        dash_pop();
    }
}

void scr_game_settings(void)
{
    xcfg_ensure_global();
    if (!g_xcfg_game_loaded && D.cur) {
        /* title_id not available in Phase 1 — show global for now */
        g_xcfg_game = g_xcfg_global;
        g_xcfg_game_loaded = true;
        g_xcfg_game_tid = 0;
    }

    const float W = ui_w();
    draw_header(T(S_GAME_SETTINGS), D.cur ? D.cur->g.title : "");
    static const enum UiStr tabs[] = {S_VIDEO, S_AUDIO, S_SYSTEM};
    D.editing_game = g_xcfg_game_tid != 0;   /* only real per-game if TID known */
    tab_bar(tabs, 3);
    List l;
    list_begin(&l, 74, 190, W - 148, dash_list_h(190, 58), 58);
    switch (D.tab) {
    case 0: rows_xenia_video(&l);  break;
    case 1: rows_xenia_audio(&l);  break;
    case 2: rows_xenia_system(&l); break;
    }
    list_end(&l);
    dash_help_draw();
    if (D.in.back) {
        xcfg_flush();
        D.editing_game = false;
        dash_pop();
    }
}

/* ================================================================ stub screens */

void scr_mods(void)        { dash_pop(); }
void scr_states(void)      { dash_pop(); }
void scr_online(void)      { dash_pop(); }
void scr_memory(void)      { dash_pop(); }
void scr_memory_title(void){ dash_pop(); }

/* Controls and remap screens remain live (implemented in input_map.c helpers) */
void scr_controls(void)
{
    /* Controls detail screen: stub — pop back to settings.
     * The player-device selection rows are rendered inline in scr_settings
     * tab 4; this screen is reached when the user presses A on a row.
     * Full binding UI (scr_remap/scr_sticks/scr_triggers) is out of scope
     * for Phase 1. */
    dash_pop();
}
void scr_remap(void)   { dash_pop(); }
void scr_sticks(void)  { dash_pop(); }
void scr_triggers(void){ dash_pop(); }

/* ================================================================ helpers called from dash.c */

static char g_play_file[1024];
static SDL_AtomicInt g_play_file_set;

void dash_play_file_later(const char *path)
{
    if (path && path[0] && !SDL_GetAtomicInt(&g_play_file_set)) {
        snprintf(g_play_file, sizeof(g_play_file), "%s", path);
        SDL_SetAtomicInt(&g_play_file_set, 1);
    }
}

void dash_take_folder(void)
{
    if (SDL_GetAtomicInt(&g_play_file_set)) {
        static LibGame once;
        memset(&once, 0, sizeof(once));
        if (xd_read_game(g_play_file, &once.g)) {
            xd_free_game(&once.g);
            launch_game(&once, NULL);
        } else {
            xd_free_game(&once.g);
            dash_toast(T(S_NOT_A_GAME));
        }
        SDL_SetAtomicInt(&g_play_file_set, 0);
    }
}

/* ================================================================ in-game overlay stubs */

UiTex *load_bmp(const char *path)        { (void)path; return NULL; }
UiTex *load_thumb(const char *dir, int slot) { (void)dir; (void)slot; return NULL; }
void   states_forget(void)               {}
