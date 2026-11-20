/*
 * Xenia Nexus — stub for console_cfg.h (xbased EEPROM sync).
 * Nexus has no Xbox EEPROM; cc_sync is a no-op.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef NATIVE_APP_UI_CONSOLE_CFG_H
#define NATIVE_APP_UI_CONSOLE_CFG_H

#include <stdbool.h>
#include "config.h"

#define CC_VIDEO_USER    0x001F0000u
#define CC_VIDEO_DEFAULT 0x00080000u

static inline int      cc_lang(const Config *c)         { (void)c; return 1; }
static inline unsigned cc_video(const Config *c)        { (void)c; return CC_VIDEO_DEFAULT; }
static inline bool     cc_sync(Config *c)               { (void)c; return false; }

#endif /* NATIVE_APP_UI_CONSOLE_CFG_H */
