/*
 * Xenia Nexus — stub for hw/xbox/hle/vk/swapchain.h.
 *
 * ui.c supports two rendering paths: (1) an SDL_Renderer on the launcher
 * window, and (2) a Vulkan overlay composited by the game's presenter.
 * The Nexus launcher only ever uses path (1) (ui_init, never ui_init_overlay),
 * so the overlay branch is dead code at run time.  The types and constants
 * below satisfy the compiler without pulling in any xenia/xbased backend.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef NEXUS_SWAPCHAIN_STUB_H
#define NEXUS_SWAPCHAIN_STUB_H

#include <stdint.h>
#include <string.h>

/* blend mode enum values (HLE_OV_* in swapchain.h) */
enum {
    HLE_OV_BLEND  = 0,
    HLE_OV_ADD    = 1,
    HLE_OV_PREMUL = 2,
};

/* special texture-handle sentinels (scene / scene-blur) */
enum {
    HLE_OV_SCENE      = 0xFFFFFFFEu,
    HLE_OV_SCENE_BLUR = 0xFFFFFFFDu,
    HLE_OV_COVER      = 1,     /* flag bit passed to hle_present_set_overlay */
};

/* overlay vertex: position, UV, packed RGBA */
typedef struct {
    float x, y, u, v;
    uint32_t col;
} HleOvVert;

/* overlay draw command: texture handle, blend, clip rect, vertex range */
typedef struct {
    uint32_t tex;
    uint32_t blend;
    int32_t  clip[4];    /* scissor rect (x, y, w, h), in pixels */
    uint32_t first;
    uint32_t count;
} HleOvCmd;

/* no-op stubs for the overlay presenter functions —
 * only reached when U.ov is true (ui_init_overlay path),
 * which never happens in the Nexus standalone launcher */
static inline uint32_t hle_ov_tex_new(uint32_t w, uint32_t h)
{
    (void)w; (void)h;
    return 0;
}
static inline void hle_ov_tex_put(uint32_t handle, uint32_t x, uint32_t y,
                                   uint32_t w, uint32_t h,
                                   const void *px, uint32_t pitch)
{
    (void)handle; (void)x; (void)y; (void)w; (void)h; (void)px; (void)pitch;
}
static inline void hle_ov_tex_free(uint32_t handle) { (void)handle; }

static inline void hle_present_set_overlay(
    const HleOvVert *v, uint32_t nv, const HleOvCmd *c, uint32_t nc, int flags)
{
    (void)v; (void)nv; (void)c; (void)nc; (void)flags;
}
static inline void hle_present_shot(const char *path) { (void)path; }

#endif /* NEXUS_SWAPCHAIN_STUB_H */
