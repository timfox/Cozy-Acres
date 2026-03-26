/* pc_paths.c - Executable path resolution: Win32 GetModuleFileNameA, Linux /proc/self/exe */
#include "pc_paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

int pc_path_is_readable(const char* path) {
    if (!path || path[0] == '\0')
        return 0;
#ifdef _WIN32
    return _access(path, 04) == 0;
#else
    return access(path, R_OK) == 0;
#endif
}

int pc_get_executable_directory(char* out, size_t out_sz) {
    if (!out || out_sz < 2)
        return 0;
    out[0] = '\0';

#ifdef _WIN32
    {
        DWORD n = GetModuleFileNameA(NULL, out, (DWORD)out_sz);
        if (n == 0 || n >= out_sz) {
            out[0] = '\0';
            return 0;
        }
    }
#else
    {
        ssize_t n = readlink("/proc/self/exe", out, out_sz - 1);
        if (n < 0 || (size_t)n >= out_sz - 1) {
            out[0] = '\0';
            return 0;
        }
        out[n] = '\0';
    }
#endif

    {
        char* slash = strrchr(out, '/');
#ifdef _WIN32
        if (!slash)
            slash = strrchr(out, '\\');
#endif
        if (!slash) {
            out[0] = '\0';
            return 0;
        }
        *slash = '\0';
    }
    return 1;
}

int pc_chdir_to_executable_directory(void) {
    char path[512];
    if (!pc_get_executable_directory(path, sizeof(path)))
        return 0;
#ifdef _WIN32
    return SetCurrentDirectoryA(path) != 0;
#else
    return chdir(path) == 0;
#endif
}

/* --- Native config: XDG on Linux, %APPDATA%\AnimalCrossing on Windows ---
 * Set PC_PORTABLE_CONFIG=1 to skip those dirs and keep .ini next to the exe (or cwd). */

static int pc_paths_portable_ini(void) {
    const char* p = getenv("PC_PORTABLE_CONFIG");
    if (!p)
        return 0;
    return (strcmp(p, "1") == 0 || strcmp(p, "true") == 0 || strcmp(p, "yes") == 0);
}

static int join_dir_file(const char* dir, const char* file, char* out, size_t out_sz) {
    int n;
#ifdef _WIN32
    n = snprintf(out, out_sz, "%s\\%s", dir, file);
#else
    n = snprintf(out, out_sz, "%s/%s", dir, file);
#endif
    return n > 0 && (size_t)n < out_sz;
}

#ifdef _WIN32
static int native_config_dir_path(char* dir, size_t dir_sz, int create_dir) {
    const char* appdata = getenv("APPDATA");
    if (!appdata || appdata[0] == '\0')
        return 0;
    if (snprintf(dir, dir_sz, "%s\\AnimalCrossing", appdata) >= (int)dir_sz)
        return 0;
    if (create_dir)
        CreateDirectoryA(dir, NULL);
    return 1;
}
#else
static int native_config_dir_path(char* dir, size_t dir_sz, int create_dir) {
    const char* xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        if (snprintf(dir, dir_sz, "%s/animal-crossing", xdg) >= (int)dir_sz)
            return 0;
    } else {
        const char* home = getenv("HOME");
        if (!home || home[0] == '\0')
            return 0;
        if (snprintf(dir, dir_sz, "%s/.config/animal-crossing", home) >= (int)dir_sz)
            return 0;
    }
    if (create_dir) {
        if (mkdir(dir, 0755) != 0 && errno != EEXIST)
            return 0;
    }
    return 1;
}
#endif

static int path_exe_dir_file(const char* basename, char* out, size_t out_sz) {
    char exe_dir[512];
    if (!pc_get_executable_directory(exe_dir, sizeof(exe_dir)))
        return 0;
    return join_dir_file(exe_dir, basename, out, out_sz);
}

int pc_paths_find_config_file(const char* basename, char* out_path, size_t out_path_sz) {
    char dir[512];
    char candidate[768];

    if (!basename || !out_path || out_path_sz < 8)
        return 0;

    if (!pc_paths_portable_ini() && native_config_dir_path(dir, sizeof(dir), 0) &&
        join_dir_file(dir, basename, candidate, sizeof(candidate)) &&
        pc_path_is_readable(candidate)) {
        strncpy(out_path, candidate, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
        return 1;
    }

    if (path_exe_dir_file(basename, candidate, sizeof(candidate)) && pc_path_is_readable(candidate)) {
        strncpy(out_path, candidate, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
        return 1;
    }

    if (pc_path_is_readable(basename)) {
        strncpy(out_path, basename, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
        return 1;
    }

    return 0;
}

int pc_paths_default_config_file(const char* basename, char* out_path, size_t out_path_sz) {
    char dir[512];
    char candidate[768];

    if (!basename || !out_path || out_path_sz < 8)
        return 0;

    if (pc_paths_portable_ini()) {
        if (path_exe_dir_file(basename, candidate, sizeof(candidate))) {
            strncpy(out_path, candidate, out_path_sz - 1);
            out_path[out_path_sz - 1] = '\0';
            return 1;
        }
        strncpy(out_path, basename, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
        return 1;
    }

    if (native_config_dir_path(dir, sizeof(dir), 1) && join_dir_file(dir, basename, candidate, sizeof(candidate))) {
        strncpy(out_path, candidate, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
        return 1;
    }

    if (path_exe_dir_file(basename, candidate, sizeof(candidate))) {
        strncpy(out_path, candidate, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
        return 1;
    }

    strncpy(out_path, basename, out_path_sz - 1);
    out_path[out_path_sz - 1] = '\0';
    return 1;
}
