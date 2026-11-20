/*
 * Xenia Nexus — stub for update.h (xbased self-updater).
 * Nexus delegates updates to the GitHub Actions CI release; no built-in
 * self-updater is provided.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef NATIVE_APP_UI_UPDATE_H
#define NATIVE_APP_UI_UPDATE_H

#include <stdbool.h>

typedef enum UpdState {
    UPD_IDLE, UPD_CHECKING, UPD_AVAILABLE, UPD_INSTALLING, UPD_INSTALLED, UPD_FAILED,
} UpdState;

static inline void      upd_cleanup(void)                       {}
static inline void      upd_check_start(const char *v)         { (void)v; }
static inline UpdState  upd_state(void)                        { return UPD_IDLE; }
static inline const char *upd_version(void)                    { return ""; }
static inline const char *upd_page(void)                       { return "https://github.com/kiequoo/Xendroid-Plus/releases"; }
static inline bool      upd_can_install(void)                   { return false; }

#endif /* NATIVE_APP_UI_UPDATE_H */
