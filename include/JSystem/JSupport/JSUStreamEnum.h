#ifndef JSUSTREAMENUM_H
#define JSUSTREAMENUM_H

/* Distinct names avoid clashing with stdio SEEK_* / EOF macros on PC (and int→enum warnings). */
enum JSUStreamSeekFrom { JSU_SEEK_SET = 0, JSU_SEEK_CUR = 1, JSU_SEEK_END = 2 };

enum EIoState { JSU_IO_GOOD = 0, JSU_IO_EOF = 1 };

#endif
