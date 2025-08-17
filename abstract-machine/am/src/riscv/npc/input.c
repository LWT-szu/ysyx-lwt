#include <am.h>
// 键盘/输入设备适配（可能拆分出去让 ioe.c 更干净)
void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  kbd->keydown = 0;
  kbd->keycode = AM_KEY_NONE;
}
