#include "stdafx.h"

#include <math.h>

#include "MovementInfo.h"

static __int32        numMovements;
static MovementInfoT *pMovements;

///////////////////////////////////////////////////////////////////////////////
ClipID GetFirstClipID()
{
  __int32 id = 1000000;
  for (int i = 0; i < numMovements; ++i)
  {
    if (pMovements[i].id < id)
    {
      id = pMovements[i].id;
    }
  }

  return (ClipID) id;
}

///////////////////////////////////////////////////////////////////////////////
ClipID GetFirstClipIDInTheShow()
{
  __int32 id = 1000000;
  for (int i = 0; i < numMovements; ++i)
  {
    if ((pMovements[i].id >= CLIPID_FIRST_OF_SHOW) && (pMovements[i].id < id))
    {
      id = pMovements[i].id;
    }
  }

  return (ClipID) id;
}

///////////////////////////////////////////////////////////////////////////////
ClipID GetLastClipID()
{
  __int32 id = CLIPID_NONE;
  for (int i = 0; i < numMovements; ++i)
  {
    if (pMovements[i].id > id)
    {
      id = pMovements[i].id;
    }
  }

  return (ClipID) id;
}

///////////////////////////////////////////////////////////////////////////////
ClipID GetNextClipID(ClipID clipID)
{
  const __int32 BIG_NUMBER = 1000000;
  __int32 id = BIG_NUMBER;
  for (int i = 0; i < numMovements; ++i)
  {
    if ((pMovements[i].id > clipID) && (pMovements[i].id < id))
    {
      id = pMovements[i].id;
    }
  }

  if (id >= BIG_NUMBER)
  {
    return CLIPID_NONE;
  }

  return (ClipID) id;
}

///////////////////////////////////////////////////////////////////////////////
MovementInfoT *GetMovementInfoForClipID(ClipID clipID)
{
  for (int i = 0; i < numMovements; ++i)
  {
    if (pMovements[i].id == clipID)
    {
      return &(pMovements[i]);
    }
  }

  return NULL;
} 

///////////////////////////////////////////////////////////////////////////////
__int32 GetNumClips()
{
  return numMovements;
}

///////////////////////////////////////////////////////////////////////////////
MovementInfoT *GetMovementInfoAtIndex(__int32 index)
{
  if ((index >= 0) && (index < numMovements))
  {
    return &(pMovements[index]);
  }

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
bool ReadMovementFile(char *fname)
{
  FILE *fp = fopen(fname, "r");
  if (fp == NULL)
  {
    return false;
  }

  char linebuf[2048];
  char *pLine;
  for (int nPass = 0; nPass < 2; ++nPass, fseek(fp, 0, SEEK_SET))
  {
    numMovements = 0;
    while ((pLine = fgets(linebuf, sizeof(linebuf), fp)) != NULL)
    {
      int index = 0;
      // Skip whitespace
      //
      while (isspace(pLine[index]))
      {
        ++index;
      }

      if (pLine[index] != 'M')
      {
        continue;  // only lines that start with "Movement" are of interest
      }

      // Skip to the space after "Movement"
      //
      while (! isspace(pLine[index]))
      {
        ++index;
      }

      // The code that produces the file writes lines like this:
      //
      //  writer.WriteLine(String.Format(
      //    "Movement {0,2:D2}   {1,4}   {2,7}   {3,6:F2}   {4}",
      //
      __int32           id = CLIPID_NONE;
      __int32    numFrames = 0;
      __int32 usecPerFrame = 0;
      double           bpm = 0.0;

      int count = 0;
      char *pNext = pLine + index;
      if (pNext != NULL) {           id = strtol(pNext, &pNext, 10); ++count; }
      if (pNext != NULL) {    numFrames = strtol(pNext, &pNext, 10); ++count; }
      if (pNext != NULL) { usecPerFrame = strtol(pNext, &pNext, 10); ++count; }
      if (pNext != NULL) {          bpm = strtod(pNext, &pNext);     ++count; }

      if ((id > CLIPID_NONE) && (numFrames > 0))
      {
        if (nPass > 0)
        {
          double usecPerBeat = bpm;
          usecPerBeat /= 60.0;
          usecPerBeat = 1000000 / usecPerBeat;

          // The array has been allocated. Save the info.
          //
          pMovements[numMovements].id             = id;
          pMovements[numMovements].numFrames      = numFrames;
          pMovements[numMovements].usecPerFrame   = usecPerFrame;
          pMovements[numMovements].usecTotal      = numFrames * usecPerFrame;
          pMovements[numMovements].beatsPerMinute = bpm;
          pMovements[numMovements].usecPerBeat    = usecPerBeat;
        }

        ++numMovements;
      }
    }

    if (nPass == 0)
    {
      pMovements =
        (MovementInfoT*) calloc(numMovements + 1, sizeof(MovementInfoT));

      // Mark the last one.
      //
      pMovements[numMovements].id        = CLIPID_NONE;
      pMovements[numMovements].numFrames = 0;
    }
  }

  return true;
}
