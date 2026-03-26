#ifndef PC_SETTINGS_H
#define PC_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int window_width;
    int window_height;
    int fullscreen;       /* 0=windowed, 1=fullscreen, 2=borderless */
    int vsync;            /* 0=off, 1=on */
    int msaa;             /* 0=off, 2/4/8=samples */
    int preload_textures; /* 0=off (load on demand), 1=on (load all at startup), 2=on + cache file */
    /* 0 = GameCube-style (0.5x per frame at ~60Hz, default; stable actor motion).
     * 1 = full step per frame (snappier; set in settings.ini if desired). */
    int physics_native_60hz;
    /* 1 = cap to ~60fps (default), 0 = uncapped (--no-framelimit / F3). */
    int framerate_cap;
} PCSettings;

extern PCSettings g_pc_settings;

void pc_settings_load(void);
void pc_settings_save(void);
void pc_settings_apply(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_SETTINGS_H */
