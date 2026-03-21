# CMake toolchain file for Linux i686 (32-bit) cross-compilation
# Typical Debian/Ubuntu packages: gcc-i686-linux-gnu, g++-i686-linux-gnu, libsdl2-dev:i386
#
# If CMake cannot find 32-bit SDL2 via pkg-config, point it at the i386 .pc files, e.g.:
#   export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

if(NOT DEFINED ENV{PKG_CONFIG_LIBDIR} OR "$ENV{PKG_CONFIG_LIBDIR}" STREQUAL "")
    if(EXISTS "/usr/lib/i386-linux-gnu/pkgconfig/sdl2.pc")
        set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/i386-linux-gnu/pkgconfig")
    endif()
endif()

set(CMAKE_C_COMPILER i686-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER i686-linux-gnu-g++)
set(CMAKE_C_FLAGS "-m32" CACHE STRING "")
set(CMAKE_CXX_FLAGS "-m32" CACHE STRING "")
