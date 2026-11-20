/*
 * Xenia Nexus — stub dash_settings.c / dash_memory.c.
 *
 * xbased's dash_settings.c implements settings, controls, mods, cheats and
 * save-states screens, plus dash_ingame.  Many of those depend on the xbased
 * Xbox kernel (xcheat.h, mod_fit.h, win_mods.h) which are not present in
 * Xenia Nexus.
 *
 * Phase 1: all screen functions are stubs that immediately pop the stack so
 * the dashboard gracefully returns to the previous screen.  A full settings UI
 * is planned for Phase 3.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "dash.h"
#include "dash_int.h"

/* ---------------------------------------------------------------- screens */

void scr_settings(void)      { dash_pop(); }
void scr_game_settings(void) { dash_pop(); }
void scr_mods(void)          { dash_pop(); }
void scr_states(void)        { dash_pop(); }
void scr_controls(void)      { dash_pop(); }
void scr_remap(void)         { dash_pop(); }
void scr_sticks(void)        { dash_pop(); }
void scr_triggers(void)      { dash_pop(); }
void scr_online(void)        { dash_pop(); }

/* ---------------------------------------------------------------- memory screens (dash_memory.c stub) */

void scr_memory(void)        { dash_pop(); }
void scr_memory_title(void)  { dash_pop(); }

/* ---------------------------------------------------------------- in-game menu stub */

int dash_ingame(SDL_Window *win, const char *key, const char *title,
                const char *frame_bmp, int *slot,
                void (*live)(const struct Config *global, const struct Config *game))
{
    (void)win; (void)key; (void)title; (void)frame_bmp; (void)slot; (void)live;
    return DASH_INGAME_RESUME;
}
