/* pc_settings.c - runtime settings loaded from settings.ini */
#include "pc_settings.h"
#include "pc_platform.h"
#include "pc_paths.h"

#include <stdio.h>
#include <string.h>

PCSettings g_pc_settings = {
    .window_width  = PC_SCREEN_WIDTH,
    .window_height = PC_SCREEN_HEIGHT,
    .fullscreen    = 0,
    .vsync         = 0,
    .msaa          = 4,
    .preload_textures = 0,
    /* Default 0: matches console integration (Actor_position_* uses 0.5 step).
     * physics_native_60hz=1 is snappier but many actors still assume 0.5-scale
     * motion, which reads as jitter, overshoot, or odd spins. */
    .physics_native_60hz = 0,
    .framerate_cap = 1,
};

static char g_settings_ini_path[512];

static const char* DEFAULT_SETTINGS =
    "[Graphics]\n"
    "# Window size (ignored in fullscreen)\n"
    "window_width = 640\n"
    "window_height = 480\n"
    "\n"
    "# 0 = windowed, 1 = fullscreen, 2 = borderless fullscreen\n"
    "fullscreen = 0\n"
    "\n"
    "# Vertical sync: 0 = off, 1 = on\n"
    "vsync = 0\n"
    "\n"
    "# Anti-aliasing samples: 0 = off, 2, 4, or 8\n"
    "msaa = 4\n"
    "\n"
    "[Enhancements]\n"
    "# Preload HD textures at startup: 0 = off (load on demand), 1 = preload, 2 = preload + cache file (fastest)\n"
    "preload_textures = 0\n"
    "\n"
    "[Gameplay]\n"
    "# 0 = original pacing (half physics step per frame at ~60Hz)\n"
    "# 1 = full physics step every frame (snappier; ~2x movement vs 0)\n"
    "physics_native_60hz = 0\n"
    "\n"
    "[Performance]\n"
    "# 1 = limit framerate (~60fps), 0 = uncapped\n"
    "framerate_cap = 1\n";

static const char* skip_ws(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static void trim_end(char* s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
                       s[len-1] == '\r' || s[len-1] == '\n')) {
        s[--len] = '\0';
    }
}

static void apply_setting(const char* key, const char* value) {
    int val = atoi(value);
    enum { WIN_W_MAX = 7680, WIN_H_MAX = 4320 };

    if (strcmp(key, "window_width") == 0) {
        if (val >= 640 && val <= WIN_W_MAX) g_pc_settings.window_width = val;
    } else if (strcmp(key, "window_height") == 0) {
        if (val >= 480 && val <= WIN_H_MAX) g_pc_settings.window_height = val;
    } else if (strcmp(key, "fullscreen") == 0) {
        if (val >= 0 && val <= 2) g_pc_settings.fullscreen = val;
    } else if (strcmp(key, "vsync") == 0) {
        if (val == 0 || val == 1) g_pc_settings.vsync = val;
    } else if (strcmp(key, "msaa") == 0) {
        if (val == 0 || val == 2 || val == 4 || val == 8)
            g_pc_settings.msaa = val;
    } else if (strcmp(key, "preload_textures") == 0) {
        if (val >= 0 && val <= 2) g_pc_settings.preload_textures = val;
    } else if (strcmp(key, "physics_native_60hz") == 0) {
        if (val == 0 || val == 1) g_pc_settings.physics_native_60hz = val;
    } else if (strcmp(key, "framerate_cap") == 0) {
        if (val == 0 || val == 1) g_pc_settings.framerate_cap = val;
    }
}

static void write_defaults(const char* path) {
    FILE* f = fopen(path, "w");
    if (f) {
        fputs(DEFAULT_SETTINGS, f);
        fclose(f);
    } else {
        fprintf(stderr, "[Settings] Cannot write default %s (check permissions)\n", path);
    }
}

void pc_settings_save(void) {
    const char* path = g_settings_ini_path[0] != '\0' ? g_settings_ini_path : "settings.ini";
    FILE* f = fopen(path, "w");
    if (!f) {
        printf("[Settings] Failed to write %s\n", path);
        return;
    }
    fprintf(f, "[Graphics]\n");
    fprintf(f, "# Window size (ignored in fullscreen)\n");
    fprintf(f, "window_width = %d\n", g_pc_settings.window_width);
    fprintf(f, "window_height = %d\n", g_pc_settings.window_height);
    fprintf(f, "\n");
    fprintf(f, "# 0 = windowed, 1 = fullscreen, 2 = borderless fullscreen\n");
    fprintf(f, "fullscreen = %d\n", g_pc_settings.fullscreen);
    fprintf(f, "\n");
    fprintf(f, "# Vertical sync: 0 = off, 1 = on\n");
    fprintf(f, "vsync = %d\n", g_pc_settings.vsync);
    fprintf(f, "\n");
    fprintf(f, "# Anti-aliasing samples: 0 = off, 2, 4, or 8\n");
    fprintf(f, "msaa = %d\n", g_pc_settings.msaa);
    fprintf(f, "\n");
    fprintf(f, "[Enhancements]\n");
    fprintf(f, "# Preload HD textures at startup: 0 = off (load on demand), 1 = preload, 2 = preload + cache file (fastest)\n");
    fprintf(f, "preload_textures = %d\n", g_pc_settings.preload_textures);
    fprintf(f, "\n");
    fprintf(f, "[Gameplay]\n");
    fprintf(f, "# 0 = GameCube-style half-step physics (default)\n");
    fprintf(f, "# 1 = full step each frame (snappier; can mismatch actor logic)\n");
    fprintf(f, "physics_native_60hz = %d\n", g_pc_settings.physics_native_60hz);
    fprintf(f, "\n");
    fprintf(f, "[Performance]\n");
    fprintf(f, "# 1 = cap ~60fps, 0 = uncapped\n");
    fprintf(f, "framerate_cap = %d\n", g_pc_settings.framerate_cap);
    fclose(f);
    printf("[Settings] Saved %s\n", path);
}

void pc_settings_apply(void) {
    if (!g_pc_window) return;

    if (g_pc_settings.fullscreen == 1) {
        SDL_SetWindowFullscreen(g_pc_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (g_pc_settings.fullscreen == 2) {
        SDL_SetWindowFullscreen(g_pc_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(g_pc_window, 0);
        SDL_SetWindowSize(g_pc_window, g_pc_settings.window_width, g_pc_settings.window_height);
        SDL_SetWindowPosition(g_pc_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    SDL_GL_SetSwapInterval(g_pc_settings.vsync);
    pc_platform_update_window_size();

    printf("[Settings] Applied: %dx%d fullscreen=%d vsync=%d msaa=%d\n",
           g_pc_settings.window_width, g_pc_settings.window_height,
           g_pc_settings.fullscreen, g_pc_settings.vsync, g_pc_settings.msaa);
}

void pc_settings_load(void) {
    g_settings_ini_path[0] = '\0';

    if (!pc_paths_find_config_file("settings.ini", g_settings_ini_path, sizeof(g_settings_ini_path))) {
        pc_paths_default_config_file("settings.ini", g_settings_ini_path, sizeof(g_settings_ini_path));
        write_defaults(g_settings_ini_path);
        printf("[Settings] Created default %s\n", g_settings_ini_path);
        return;
    }

    FILE* f = fopen(g_settings_ini_path, "r");
    if (!f) {
        fprintf(stderr, "[Settings] Cannot open %s\n", g_settings_ini_path);
        g_settings_ini_path[0] = '\0';
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        const char* p = skip_ws(line);

        if (*p == '#' || *p == ';' || *p == '\0' || *p == '\n' || *p == '[')
            continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = (char*)skip_ws(line);
        trim_end(key);
        char* value = (char*)skip_ws(eq + 1);
        trim_end(value);

        if (*key && *value) {
            apply_setting(key, value);
        }
    }
    fclose(f);
    printf("[Settings] Loaded %s: %dx%d fullscreen=%d vsync=%d msaa=%d preload=%d physics=%d framerate_cap=%d\n",
           g_settings_ini_path, g_pc_settings.window_width, g_pc_settings.window_height,
           g_pc_settings.fullscreen, g_pc_settings.vsync, g_pc_settings.msaa,
           g_pc_settings.preload_textures, g_pc_settings.physics_native_60hz, g_pc_settings.framerate_cap);
}
