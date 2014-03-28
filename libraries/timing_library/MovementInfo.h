#ifndef __INCLUDED_MOVEMENTINFO__

#include "BKProtocol.h"

typedef struct
{
  __int32 id;
  u_int32 numFrames;
  u_int32 usecPerFrame;
  u_int32 usecTotal;
  double  beatsPerMinute;
  double  usecPerBeat;
} MovementInfoT;

extern bool ReadMovementFile(char *fname);
extern ClipID GetFirstClipID();
extern ClipID GetFirstClipIDInTheShow();
extern ClipID GetLastClipID();
extern ClipID GetNextClipID(ClipID clipID);
extern MovementInfoT *GetMovementInfoForClipID(ClipID clipID);

extern __int32 GetNumClips();
extern MovementInfoT *GetMovementInfoAtIndex(__int32 index);

#define __INCLUDED_MOVEMENTINFO__
#endif
