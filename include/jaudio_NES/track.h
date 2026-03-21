#ifndef TRACK_H
#define TRACK_H

#include "types.h"
#include "jaudio_NES/audiowork.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void Nas_ReleaseNoteTrack(note* note);
extern void Nas_ReleaseSubTrack(sub* subtrack);
extern void Nas_ReleaseGroup_Force(group* grp);
extern void Nas_ReleaseGroup(group* grp);
extern void Nas_AddList(jal_link* root, jal_link* list);
extern void* Nas_GetList(jal_link* root);
extern void Nas_MySeqMain(u32 frames_left);
extern void Nas_SeqSkip(group* grp);
extern void Nas_InitMySeq(group* grp);
extern void Nas_AssignSubTrack(s32 group_idx);
extern void Nas_InitPlayer(void);

#ifdef __cplusplus
}
#endif

#endif
