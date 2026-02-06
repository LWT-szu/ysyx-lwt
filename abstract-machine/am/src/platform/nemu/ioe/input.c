#include <am.h>
#include <nemu.h>
// 某个键码的高位为1,则表示按键按下状态的掩码
#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  int temp = inl(KBD_ADDR);
  kbd->keydown = (temp & KEYDOWN_MASK) ? 1 : 0;
  kbd->keycode = temp & ~KEYDOWN_MASK;
  if (kbd->keycode == AM_KEY_NONE) kbd->keydown = 0;
}

// 从寄存器中读取的键码包含最高位的按下标志和实际键码
// 通过掩码区分按下和释放状态，并提取实际键码