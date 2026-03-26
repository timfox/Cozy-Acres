#include "libc64/math64.h"
#include "MSL_C/w_math.h"

f32 fatan2(f32 x, f32 y) {
#ifdef TARGET_PC
    return atan2f(x, y);
#else
    return (f32)atan2((double)x, (double)y);
#endif
}

#if defined(TARGET_PC) && defined(__linux__)
f32 m64_fsqrt(f32 x) {
#else
f32 fsqrt(f32 x) {
#endif
    return sqrtf(x);
}

f32 facos(f32 x) {
    return acos(x);
}
