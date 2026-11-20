/*
 * Xenia Nexus — stub for shortcut.h (xbased desktop shortcut maker).
 * Not implemented in the initial release.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef NATIVE_APP_UI_SHORTCUT_H
#define NATIVE_APP_UI_SHORTCUT_H

#include <stdbool.h>
#include <stddef.h>

static inline bool sc_make(const char *name, const char *game,
                            const char *icon_png, const char *icon_ico,
                            char *where, size_t n)
{
    (void)name; (void)game; (void)icon_png; (void)icon_ico; (void)where; (void)n;
    return false;
}

#endif /* NATIVE_APP_UI_SHORTCUT_H */
