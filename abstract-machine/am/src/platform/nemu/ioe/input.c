#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  int temp = inl(KBD_ADDR);
  kbd->keydown = (temp & KEYDOWN_MASK) ? 1 : 0;
  kbd->keycode = temp & ~KEYDOWN_MASK;
  if (kbd->keycode == AM_KEY_NONE) kbd->keydown = 0;
}
