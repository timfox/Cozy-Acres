/* pc_http.c - HTTP via libcurl */
#include "pc_http.h"

#include <curl/curl.h>
/* CURLOPT_PROTOCOLS_STR replaces deprecated CURLOPT_* since libcurl 7.85 */
#if LIBCURL_VERSION_NUM >= 0x075500
#define PC_HTTP_HAVE_PROTOCOLS_STR 1
#endif
#include <stdlib.h>
#include <string.h>

static int s_curl_inited;

struct pc_membuf {
    char* ptr;
    size_t len;
};

static size_t pc_http_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t chunk = size * nmemb;
    struct pc_membuf* m = (struct pc_membuf*)userdata;
    char* next = (char*)realloc(m->ptr, m->len + chunk + 1);
    if (!next)
        return 0;
    memcpy(next + m->len, ptr, chunk);
    m->ptr = next;
    m->len += chunk;
    m->ptr[m->len] = '\0';
    return chunk;
}

void pc_http_init(void) {
    if (s_curl_inited)
        return;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        return;
    s_curl_inited = 1;
}

void pc_http_shutdown(void) {
    if (!s_curl_inited)
        return;
    curl_global_cleanup();
    s_curl_inited = 0;
}

static int pc_http_request(CURL* curl, struct pc_membuf* buf, long* out_http_code) {
    char errbuf[CURL_ERROR_SIZE];

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, pc_http_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AnimalCrossing-PC/1.0");
#ifdef PC_HTTP_HAVE_PROTOCOLS_STR
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

    CURLcode cr = curl_easy_perform(curl);
    if (cr != CURLE_OK) {
        if (out_http_code)
            *out_http_code = 0;
        return -1;
    }
    if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, out_http_code) != CURLE_OK && out_http_code)
        *out_http_code = 0;
    return 0;
}

int pc_http_get(const char* url, void** out_data, size_t* out_size, long* out_http_code) {
    if (!url || !out_data || !out_size)
        return -1;
    *out_data = NULL;
    *out_size = 0;
    if (out_http_code)
        *out_http_code = 0;

    pc_http_init();
    if (!s_curl_inited)
        return -1;

    struct pc_membuf buf = { NULL, 0 };
    CURL* curl = curl_easy_init();
    if (!curl)
        return -1;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    int err = pc_http_request(curl, &buf, out_http_code);
    curl_easy_cleanup(curl);

    if (err != 0) {
        free(buf.ptr);
        return -1;
    }

    *out_data = buf.ptr;
    *out_size = buf.len;
    return 0;
}

int pc_http_post(const char* url, const void* body, size_t body_len, const char* content_type,
                 void** out_data, size_t* out_size, long* out_http_code) {
    if (!url || !out_data || !out_size)
        return -1;
    *out_data = NULL;
    *out_size = 0;
    if (out_http_code)
        *out_http_code = 0;

    pc_http_init();
    if (!s_curl_inited)
        return -1;

    struct pc_membuf buf = { NULL, 0 };
    CURL* curl = curl_easy_init();
    if (!curl)
        return -1;

    struct curl_slist* hdrs = NULL;
    char ctype_buf[96];
    const char* ct = content_type ? content_type : "application/octet-stream";
    if (snprintf(ctype_buf, sizeof(ctype_buf), "Content-Type: %s", ct) < (int)sizeof(ctype_buf))
        hdrs = curl_slist_append(hdrs, ctype_buf);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body && body_len ? body : "");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
    if (hdrs)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    int err = pc_http_request(curl, &buf, out_http_code);
    if (hdrs)
        curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (err != 0) {
        free(buf.ptr);
        return -1;
    }

    *out_data = buf.ptr;
    *out_size = buf.len;
    return 0;
}
