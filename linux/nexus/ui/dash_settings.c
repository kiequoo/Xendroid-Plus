/*
 * Xenia Nexus — stub dash_settings.c / dash_memory.c.
 *
 * xbased's dash_settings.c implements settings, controls, mods, cheats and
 * save-states screens, plus several helpers used by dash.c's main loop
 * (dash_take_folder, dash_play_file_later) and by the in-game overlay
 * (load_bmp, load_thumb, states_forget).  Those helpers depend on the xbased
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
#include "ui.h"

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

/* ---------------------------------------------------------------- helpers called from dash.c main loop */

/* dash_play_file_later / dash_take_folder: drag-and-drop game launch.
 * Not needed for Phase 1 (Odin 3 has no mouse/file-drop); stubs keep the
 * main loop compile-able without the full xbased kernel. */
void dash_play_file_later(const char *path) { (void)path; }
void dash_take_folder(void) {}

/* ---------------------------------------------------------------- in-game overlay helpers */

/* load_bmp / load_thumb / states_forget: used by dash_ingame() in dash.c
 * for the save-state overlay UI.  In Nexus the overlay path
 * (ui_is_overlay()) is always false, so these are only called when a user
 * reaches the in-game menu from within xenia's overlay — which Nexus does
 * not support in Phase 1.  Return NULL / no-op so it links and degrades
 * gracefully. */
UiTex *load_bmp(const char *path)        { (void)path; return NULL; }
UiTex *load_thumb(const char *dir, int slot) { (void)dir; (void)slot; return NULL; }
void   states_forget(void)               {}
