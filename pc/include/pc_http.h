/* pc_http.h - libcurl-backed HTTP client (PC port) */
#ifndef PC_HTTP_H
#define PC_HTTP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void pc_http_init(void);
void pc_http_shutdown(void);

/* Synchronous GET. On success returns 0, sets *out_data to malloc'd bytes (caller free()s),
 * *out_size to body length, *out_http_code to HTTP status (200, 404, ...).
 * Response is NUL-terminated at out_data[out_size] for convenience (extra byte past body).
 * out_http_code may be set even on failure (e.g. 404 with body). */
int pc_http_get(const char* url, void** out_data, size_t* out_size, long* out_http_code);

/* POST with optional Content-Type (NULL -> application/octet-stream). */
int pc_http_post(const char* url, const void* body, size_t body_len, const char* content_type,
                 void** out_data, size_t* out_size, long* out_http_code);

#ifdef __cplusplus
}
#endif

#endif /* PC_HTTP_H */
