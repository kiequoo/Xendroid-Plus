/*
 * Xenia Nexus — launcher entry point.
 *
 * Creates the dashboard window and hands control to dash_run().
 * When a game-path argument is passed the AppRun script routes directly to
 * xenia_edge, so this binary only ever sees the no-argument case.
 *
 * Modelled on the dashboard block of xbased's native/app/win_main.c:1779-1800.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "ui/dash.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Resolve the directory containing the running executable.
 * Returns a heap-allocated string ending in '/', or NULL on failure. */
static char *exe_dir(void)
{
    char *base = SDL_GetBasePath();
    if (!base) {
        return NULL;
    }
    return base;   /* SDL_GetBasePath already ends with the separator */
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        fprintf(stderr, "xenia-nexus: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* Full-screen by default on handhelds; windowed when NEXUS_WINDOWED is set */
    const bool fullscreen = !SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "NEXUS_WINDOWED");
    SDL_WindowFlags wflags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (fullscreen) {
        wflags |= SDL_WINDOW_FULLSCREEN;
    }

    SDL_Window *win = SDL_CreateWindow("Xenia Nexus", 1280, 720, wflags);
    if (!win) {
        fprintf(stderr, "xenia-nexus: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    char *dir = exe_dir();
    const int r = dash_run(win, dir ? dir : "");
    SDL_free(dir);

    SDL_DestroyWindow(win);
    SDL_Quit();
    return r;
}
