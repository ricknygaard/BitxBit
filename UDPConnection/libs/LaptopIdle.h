#ifndef __INCLUDED_LAPTOPIDLE__

typedef enum
{
  KEYCODE_NONE = 0,
  KEYCODE_BEGIN,
  KEYCODE_NEXT,
  KEYCODE_STOP,
  KEYCODE_RESET,
  KEYCODE_TEMPO
} KeyCode;

extern void LaptopInit();
extern void LaptopIdleFunction(KeyCode key);

#define __INCLUDED_LAPTOPIDLE__
#endif
