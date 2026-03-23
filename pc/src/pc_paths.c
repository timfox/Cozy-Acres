/* pc_paths.c - Executable path resolution: Win32 GetModuleFileNameA, Linux /proc/self/exe */
#include "pc_paths.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
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
