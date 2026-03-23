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

#ifdef __cplusplus
}
#endif

#endif /* PC_PATHS_H */
