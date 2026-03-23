/* pc_main.c - PC entry point: SDL2/GL init, crash protection, boot sequence */
#include "pc_platform.h"
#include "pc_paths.h"
#include "pc_gx_internal.h"
#include "pc_texture_pack.h"
#include "pc_settings.h"
#include "pc_keybindings.h"
#include "pc_assets.h"
#include "pc_disc.h"

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#endif

/* prefer discrete GPU on laptops */
#ifdef _WIN32
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

SDL_Window*   g_pc_window = NULL;
SDL_GLContext  g_pc_gl_context = NULL;
int           g_pc_running = 1;
int           g_pc_no_framelimit = 0;
int           g_pc_verbose = 0;
int           g_pc_time_override = -1; /* -1=system clock, 0-23=override hour */
int           g_pc_window_w = PC_SCREEN_WIDTH;
int           g_pc_window_h = PC_SCREEN_HEIGHT;
int           g_pc_widescreen_stretch = 0;

/* exe image range — used by seg2k0 to distinguish pointers from segment addresses */
unsigned int pc_image_base = 0;
unsigned int pc_image_end  = 0;

static jmp_buf* pc_active_jmpbuf = NULL;
static volatile unsigned int pc_last_crash_addr = 0;

static volatile unsigned int pc_last_crash_data_addr = 0;

#ifdef _WIN32
/* longjmp from VEH is technically UB, but works on x86 MinGW (no SEH to corrupt).
 * GCC doesn't have __try/__except and checking every pointer in emu64 is impractical. */
static LONG WINAPI pc_veh_handler(PEXCEPTION_POINTERS ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (pc_active_jmpbuf != NULL &&
        (code == EXCEPTION_ACCESS_VIOLATION ||
         code == EXCEPTION_ILLEGAL_INSTRUCTION ||
         code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
         code == EXCEPTION_PRIV_INSTRUCTION)) {
        pc_last_crash_addr = (unsigned int)(uintptr_t)ep->ExceptionRecord->ExceptionAddress;
        if (code == EXCEPTION_ACCESS_VIOLATION)
            pc_last_crash_data_addr = (unsigned int)(uintptr_t)ep->ExceptionRecord->ExceptionInformation[1];
        else
            pc_last_crash_data_addr = 0;
        jmp_buf* buf = pc_active_jmpbuf;
        pc_active_jmpbuf = NULL;
        longjmp(*buf, 1);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
/* POSIX equivalent of VEH — longjmp from signal handler (POSIX-defined for program faults) */
static void pc_signal_handler(int sig, siginfo_t* info, void* ucontext) {
    (void)ucontext;
    if (pc_active_jmpbuf != NULL) {
        pc_last_crash_addr = (unsigned int)(uintptr_t)info->si_addr;
        pc_last_crash_data_addr = (sig == SIGSEGV) ?
            (unsigned int)(uintptr_t)info->si_addr : 0;
        jmp_buf* buf = pc_active_jmpbuf;
        pc_active_jmpbuf = NULL;
        longjmp(*buf, 1);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

unsigned int pc_crash_get_data_addr(void) {
    return pc_last_crash_data_addr;
}

void pc_crash_protection_init(void) {
    static int installed = 0;
    if (!installed) {
#ifdef _WIN32
        AddVectoredExceptionHandler(1, pc_veh_handler);
#else
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = pc_signal_handler;
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGILL, &sa, NULL);
        sigaction(SIGFPE, &sa, NULL);
#endif
        installed = 1;
    }
}

void pc_crash_set_jmpbuf(jmp_buf* buf) {
    pc_active_jmpbuf = buf;
}

unsigned int pc_crash_get_addr(void) {
    return pc_last_crash_addr;
}

#if defined(__linux__)
/* Optional: dlopen libLLVM with RTLD_GLOBAL before SDL so Mesa's nested dlopen may reuse one
 * LLVM copy. On some stacks (e.g. Ubuntu 25.10 i386) the *first* dlopen of libLLVM SIGSEGVs
 * inside LLVM static init — enabling this only makes the crash happen here instead of in
 * GLX/EGL. Default is off; set PC_LLVM_PRELOAD=1 to try it on distros where it helps. */
static void pc_linux_preload_llvm_rtld_global(void) {
    static const char* const candidates[] = {
        "libLLVM.so.20.1",
        "libLLVM.so.19.1",
        "libLLVM-20.so",
        "libLLVM-20.so.1",
        "/lib/i386-linux-gnu/libLLVM.so.20.1",
        "/usr/lib/i386-linux-gnu/libLLVM.so.20.1",
        "libLLVM.so.18.1",
        NULL,
    };
    const char* opt = getenv("PC_LLVM_PRELOAD");
    if (opt == NULL || strcmp(opt, "1") != 0)
        return;
    const char* last_err = NULL;
    for (int i = 0; candidates[i] != NULL; i++) {
        void* h = dlopen(candidates[i], RTLD_NOW | RTLD_GLOBAL);
        if (h != NULL) {
            fprintf(stderr, "[PC] Preloaded %s (RTLD_GLOBAL) before SDL_Init (PC_LLVM_PRELOAD=1)\n",
                    candidates[i]);
            fflush(stderr);
            return;
        }
        last_err = dlerror();
    }
    fprintf(stderr, "[PC] PC_LLVM_PRELOAD=1 but no libLLVM opened; last dlerror: %s\n",
            last_err ? last_err : "(none)");
    fflush(stderr);
}

/* If 32-bit proprietary NVIDIA GLX is installed, prefer it over Mesa so we never load
 * Mesa's i386 libLLVM (known to SIGSEGV during static init on some Ubuntu 25.x setups).
 * libglvnd reads __GLX_VENDOR_LIBRARY_NAME. User can set the var themselves; we only
 * fill it when unset. Opt out: PC_SKIP_NVIDIA_GLX_VENDOR=1 */
static void pc_linux_try_nvidia_glx_vendor(void) {
    if (sizeof(void*) != 4)
        return;
    if (getenv("__GLX_VENDOR_LIBRARY_NAME") != NULL)
        return;
    if (getenv("PC_SKIP_NVIDIA_GLX_VENDOR") != NULL)
        return;
    static const char* const glx_paths[] = {
        "/usr/lib/i386-linux-gnu/libGLX_nvidia.so.0",
        "/usr/lib32/libGLX_nvidia.so.0",
        NULL,
    };
    for (int i = 0; glx_paths[i] != NULL; i++) {
        if (access(glx_paths[i], R_OK) == 0) {
            setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
            if (g_pc_verbose)
                fprintf(stderr, "[PC] Using NVIDIA GLX vendor (found %s); avoids Mesa i386 LLVM path\n",
                        glx_paths[i]);
            return;
        }
    }
}
#endif

void pc_platform_init(void) {
#ifdef _WIN32
    SetProcessDPIAware();
#elif defined(__linux__)
    pc_linux_try_nvidia_glx_vendor();
    /* Hybrid graphics: prefer discrete GPU (like Windows exports). Skip on 32-bit builds:
     * DRI_PRIME often pulls AMDGPU's i386 GLX/LLVM stack from /opt/amdgpu and segfaults
     * on glXChooseVisual; 64-bit is unaffected. Set PC_USE_DISCRETE_GPU=1 to force these
     * hints even for i686 if your setup needs them. */
    if (sizeof(void*) > 4 || getenv("PC_USE_DISCRETE_GPU") != NULL) {
        setenv("DRI_PRIME", "1", 0);
        setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);
    }
    /* 32-bit SDL on Wayland is unreliable on several distros; default to X11 unless the
     * user chose a driver (SDL_VIDEODRIVER) or opted into Wayland (PC_SDL_USE_WAYLAND). */
    if (getenv("PC_SDL_USE_WAYLAND") == NULL && getenv("SDL_VIDEODRIVER") == NULL) {
        SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "x11", SDL_HINT_DEFAULT);
    }
    /* Questing-class crashes: glXChooseVisual -> dlopen LLVM. EGL on X11 can avoid that path.
     * Opt-in: PC_X11_EGL=1. (Forced EGL breaks some AMDGPU i386 setups — see run-pc-linux-safegl.sh.) */
    if (getenv("PC_X11_EGL") != NULL && getenv("SDL_VIDEO_X11_FORCE_EGL") == NULL) {
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_X11_FORCE_EGL, "1", SDL_HINT_OVERRIDE);
    }
    pc_linux_preload_llvm_rtld_global();
#endif
    /* VIDEO first on all platforms: defers gamepad/audio/timer init until after GL context
     * (matches Linux stability path; harmless on Windows). */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef PC_ENHANCEMENTS
    {
        int gl_msaa_samples = g_pc_settings.msaa;
#if defined(__linux__)
        /* i386 GLX multisample often segfaults before first frame; plain ./AnimalCrossing must stay safe.
         * Set PC_MSAA=1 to request multisample from settings.ini (same as other platforms). */
        if (getenv("PC_MSAA") == NULL)
            gl_msaa_samples = 0;
#endif
        if (gl_msaa_samples > 0) {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gl_msaa_samples);
        }
    }
#endif

    {
        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        int win_w = g_pc_settings.window_width;
        int win_h = g_pc_settings.window_height;
        if (g_pc_settings.fullscreen == 1) {
            flags |= SDL_WINDOW_FULLSCREEN;
        } else if (g_pc_settings.fullscreen == 2) {
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
        g_pc_window = SDL_CreateWindow(
            PC_WINDOW_TITLE,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            win_w, win_h, flags
        );
    }
    if (!g_pc_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    g_pc_gl_context = SDL_GL_CreateContext(g_pc_window);
    if (!g_pc_gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_pc_window);
        SDL_Quit();
        exit(1);
    }

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "gladLoadGL failed\n");
        SDL_GL_DeleteContext(g_pc_gl_context);
        SDL_DestroyWindow(g_pc_window);
        SDL_Quit();
        exit(1);
    }

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_InitSubSystem failed: %s\n", SDL_GetError());
        SDL_GL_DeleteContext(g_pc_gl_context);
        SDL_DestroyWindow(g_pc_window);
        SDL_Quit();
        exit(1);
    }

    SDL_GL_SetSwapInterval(g_pc_settings.vsync);

    pc_platform_update_window_size();

#ifdef PC_ENHANCEMENTS
#if defined(__linux__)
    if (g_pc_settings.msaa > 0 && getenv("PC_MSAA") != NULL)
#else
    if (g_pc_settings.msaa > 0)
#endif
    {
        glEnable(GL_MULTISAMPLE);
    }
#endif

    pc_gx_init();
    pc_texture_pack_init();
#ifdef PC_ENHANCEMENTS
    if (g_pc_settings.preload_textures) {
        pc_texture_pack_preload_all();
    }
#endif
}

extern void PADCleanup(void);

void pc_platform_shutdown(void) {
    pc_audio_shutdown();
    pc_audio_mq_shutdown();
    PADCleanup();
    pc_texture_pack_shutdown();
    pc_gx_shutdown();

    if (g_pc_gl_context) {
        SDL_GL_DeleteContext(g_pc_gl_context);
        g_pc_gl_context = NULL;
    }
    if (g_pc_window) {
        SDL_DestroyWindow(g_pc_window);
        g_pc_window = NULL;
    }
    SDL_Quit();
}

void pc_platform_update_window_size(void) {
    SDL_GL_GetDrawableSize(g_pc_window, &g_pc_window_w, &g_pc_window_h);
    if (g_pc_window_w <= 0) g_pc_window_w = PC_SCREEN_WIDTH;
    if (g_pc_window_h <= 0) g_pc_window_h = PC_SCREEN_HEIGHT;
}

void pc_platform_swap_buffers(void) {
    SDL_GL_SwapWindow(g_pc_window);
}

static int pc_confirm_quit(void) {
    const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Quit" },
    };
    const SDL_MessageBoxData data = {
        SDL_MESSAGEBOX_INFORMATION, g_pc_window,
        "Animal Crossing", "Are you sure you want to quit?",
        2, buttons, NULL
    };
    int button = 0;
    if (SDL_ShowMessageBox(&data, &button) < 0)
        return 1; /* on error, just quit */
    return button == 1;
}

int pc_platform_poll_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                if (pc_confirm_quit()) {
                    g_pc_running = 0;
                    return 0;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    pc_platform_update_window_size();
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (pc_confirm_quit()) {
                        g_pc_running = 0;
                        return 0;
                    }
                }
                if (event.key.keysym.sym == SDLK_F3 && !event.key.repeat) {
                    g_pc_no_framelimit ^= 1;
                    printf("[PC] Frame limiter %s\n", g_pc_no_framelimit ? "OFF" : "ON");
                }
                break;
        }
    }
    return 1;
}

/* game's main() renamed to ac_entry via -Dmain=ac_entry, boot.c's to boot_main */
extern void ac_entry(void);
extern int boot_main(int argc, const char** argv);

/* Fallback when cwd has no disc image: dev trees often keep rom/orig at repo root while the
 * binary lives under pc/build32/bin. Packaged installs colocate disc + exe, so disc init often
 * succeeds on the first try without chdir. */
static void pc_chdir_to_exe_dir(void) {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, sizeof(path));
    if (n == 0 || n >= sizeof(path))
        return;
    char* slash = strrchr(path, '\\');
    if (!slash)
        slash = strrchr(path, '/');
    if (slash) {
        *slash = '\0';
        if (!SetCurrentDirectoryA(path) && g_pc_verbose)
            fprintf(stderr, "[PC] SetCurrentDirectoryA failed: %s\n", path);
    }
#elif defined(__linux__)
    char path[512];
    ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n < 0 || (size_t)n >= sizeof(path) - 1)
        return;
    path[n] = '\0';
    char* slash = strrchr(path, '/');
    if (slash) {
        *slash = '\0';
        if (chdir(path) != 0 && g_pc_verbose)
            fprintf(stderr, "[PC] chdir to exe dir failed: %s\n", path);
    }
#endif
}

static int pc_probe_preextracted_rom(void) {
    FILE* f = fopen("orig/GAFE01_00/sys/main.dol", "rb");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: AnimalCrossing [options]\n");
            printf("  --verbose, -v       Enable diagnostic output\n");
            printf("  --no-framelimit     Disable frame limiter\n");
            printf("  --model-viewer [N]  Launch model viewer (optional start index)\n");
            printf("  --time HOUR         Override in-game hour (0-23)\n");
            printf("  --help, -h          Show this help message\n");
            return 0;
        } else if (strcmp(argv[i], "--no-framelimit") == 0) {
            g_pc_no_framelimit = 1;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            g_pc_verbose = 1;
        } else if (strcmp(argv[i], "--model-viewer") == 0) {
            g_pc_model_viewer = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                g_pc_model_viewer_start = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "--time") == 0 && i + 1 < argc) {
            g_pc_time_override = atoi(argv[i + 1]);
            if (g_pc_time_override < 0 || g_pc_time_override > 23) g_pc_time_override = -1;
            i++;
        }
    }

    /* Redirect stdout/stderr to NUL unless verbose — unbuffered terminal writes
     * are extremely slow on Windows and tank FPS. */
    if (!g_pc_verbose) {
#ifdef _WIN32
        /* Unbuffered Win32 console I/O tanks FPS; Linux terminals are cheap enough to keep stdio. */
        freopen("NUL", "w", stdout);
        freopen("NUL", "w", stderr);
#endif
    } else {
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    /* exe image range for seg2k0 — BSS can overlap N64 segment addresses */
#ifdef _WIN32
    {
        HMODULE exe = GetModuleHandle(NULL);
        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)exe;
        IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((char*)exe + dos->e_lfanew);
        pc_image_base = (unsigned int)(uintptr_t)exe;
        pc_image_end = pc_image_base + nt->OptionalHeader.SizeOfImage;
    }
#else
    {
        Dl_info dl;
        if (dladdr((void*)main, &dl) && dl.dli_fbase) {
            pc_image_base = (unsigned int)(uintptr_t)dl.dli_fbase;
            Elf32_Ehdr* ehdr = (Elf32_Ehdr*)dl.dli_fbase;
            Elf32_Phdr* phdr = (Elf32_Phdr*)((char*)dl.dli_fbase + ehdr->e_phoff);
            unsigned int max_end = 0;
            for (int i = 0; i < ehdr->e_phnum; i++) {
                if (phdr[i].p_type == PT_LOAD) {
                    unsigned int seg_end = phdr[i].p_vaddr + phdr[i].p_memsz;
                    if (seg_end > max_end) max_end = seg_end;
                }
            }
            /* ET_EXEC: p_vaddr is absolute. ET_DYN (PIE): relative to load address. */
            if (ehdr->e_type == ET_DYN) {
                pc_image_end = pc_image_base + max_end;
            } else {
                pc_image_end = max_end;
            }
        }
    }
#endif

    SDL_SetMainReady();
    pc_settings_load();
    pc_keybindings_load();
    pc_platform_init();
    /* Disc search uses . / orig / rom relative to cwd. Prefer cwd when it already has game data
     * (rom next to repo, or orig/ tree) so we do not break that layout by jumping to build/bin. */
    if (!pc_disc_init() && !pc_probe_preextracted_rom()) {
        pc_chdir_to_exe_dir();
        if (!pc_disc_init() && g_pc_verbose)
            fprintf(stderr,
                    "[PC] No .ciso/.iso/.gcm in cwd or executable directory (./ orig/ rom/)\n");
    }
    pc_assets_init();

    ac_entry();                         /* sets HotStartEntry = &entry */
    boot_main(argc, (const char**)argv); /* full init → HotStartEntry → game loop */

    pc_disc_shutdown();
    pc_platform_shutdown();
    return 0;
}
