/* pc_paths.h - Cross-platform executable directory and file checks (Windows + Linux) */
#ifndef PC_PATHS_H
#define PC_PATHS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Non-zero if path exists and is readable. */
int pc_path_is_readable(const char* path);

/* Directory containing the executable (no trailing slash). Returns 1 on success, 0 if unknown. */
int pc_get_executable_directory(char* out, size_t out_sz);

/* chdir() to the executable's directory. Returns 1 on success. */
int pc_chdir_to_executable_directory(void);

/* Resolve settings.ini / keybindings.ini: native config dir (XDG / %APPDATA%), then
 * directory of the executable, then current working directory. Returns 1 if a file
 * was found at out_path.
 * Set PC_PORTABLE_CONFIG=1 (or true/yes) to skip XDG/AppData and use exe/cwd only. */
int pc_paths_find_config_file(const char* basename, char* out_path, size_t out_path_sz);

/* Path for a new config file when none exists: prefer native config (dirs created on
 * Linux; Windows creates AnimalCrossing under APPDATA). Falls back to exe dir, then basename.
 * Respects PC_PORTABLE_CONFIG same as pc_paths_find_config_file. */
int pc_paths_default_config_file(const char* basename, char* out_path, size_t out_path_sz);

#ifdef __cplusplus
}
#endif

#endif /* PC_PATHS_H */
