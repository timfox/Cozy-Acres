/* pc_gx_tev.c - TEV shader: GLSL program loading and uniform upload */
#include "pc_gx_internal.h"

#if defined(__linux__)
#include <unistd.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#endif

/* Directory containing the running executable (no trailing slash), or empty if unknown. */
static void pc_exe_dir(char* dir, size_t dir_sz) {
    dir[0] = '\0';
    if (dir_sz < 2)
        return;
#if defined(_WIN32)
    if (GetModuleFileNameA(NULL, dir, (DWORD)dir_sz) == 0)
        return;
#else
    ssize_t n = readlink("/proc/self/exe", dir, dir_sz - 1);
    if (n < 0 || (size_t)n >= dir_sz - 1)
        return;
    dir[n] = '\0';
#endif
    char* slash = strrchr(dir, '/');
#if defined(_WIN32)
    if (!slash)
        slash = strrchr(dir, '\\');
#endif
    if (slash)
        *slash = '\0';
    else
        dir[0] = '\0';
}

/* --- file I/O --- */

static char* load_text_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len <= 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (read != (size_t)len) {
        fprintf(stderr, "WARNING: Partial read of %s (got %zu of %ld bytes)\n", path, read, len);
        free(buf);
        return NULL;
    }
    buf[read] = '\0';
    return buf;
}

static char* load_shader(const char* filename) {
    char path[512];
    char base[512];
    char* src;

    pc_exe_dir(base, sizeof(base));
    if (base[0] != '\0') {
        snprintf(path, sizeof(path), "%s/shaders/%s", base, filename);
        src = load_text_file(path);
        if (src) {
            printf("[PC/TEV] Loaded shader: %s\n", path);
            return src;
        }
    }

    snprintf(path, sizeof(path), "shaders/%s", filename);
    src = load_text_file(path);
    if (src) {
        printf("[PC/TEV] Loaded shader: %s\n", path);
    } else {
        fprintf(stderr, "FATAL: Could not load shader %s (tried next to executable, then cwd shaders/)\n",
                filename);
    }
    return src;
}

static GLuint default_program = 0;

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "FATAL: Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_program(GLuint vert, GLuint frag) {
    if (!vert || !frag) {
        fprintf(stderr, "FATAL: Cannot link program — shader compilation failed\n");
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);

    glBindAttribLocation(prog, 0, "a_position");
    glBindAttribLocation(prog, 1, "a_normal");
    glBindAttribLocation(prog, 2, "a_color0");
    glBindAttribLocation(prog, 3, "a_texcoord0");

    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "FATAL: Program link error: %s\n", log);
        glDeleteProgram(prog);
        prog = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void pc_gx_tev_init(void) {
    char* vs_src = load_shader("default.vert");
    char* fs_src = load_shader("default.frag");

    if (!vs_src || !fs_src) {
        fprintf(stderr, "FATAL: Shader files missing.\n"
                        "Expected: <exe-dir>/shaders/default.vert and default.frag (or cwd shaders/).\n"
                        "Re-run ./build_pc.sh so CMake copies them into pc/build32/bin/shaders/.\n");
        free(vs_src);
        free(fs_src);
        exit(1);
    }

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    default_program = link_program(vs, fs);

    free(vs_src);
    free(fs_src);
}

void pc_gx_tev_shutdown(void) {
    if (default_program) {
        glDeleteProgram(default_program);
        default_program = 0;
    }

}

GLuint pc_gx_tev_get_shader(PCGXState* state) {
    return default_program;
}
