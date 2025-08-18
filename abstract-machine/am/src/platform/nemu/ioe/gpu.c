#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  uint32_t gpu_t = inl(VGACTL_ADDR);
  int i;
  int w = gpu_t >> 16;
  int h = gpu_t & 0xFFFF;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i++) fb[i] = i;
  outl(SYNC_ADDR, 1);
}
// 读出屏幕大小信息
void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t gpu_temp = inl(VGACTL_ADDR);
  int wei = gpu_temp >> 16;
  int hei = gpu_temp & 0xFFFF;
  *cfg = (AM_GPU_CONFIG_T){
      .present = true,
      .has_accel = false,
      .width = wei,
      .height = hei,
      .vmemsz = wei * hei * sizeof(uint32_t)};//4 also can do it
}
// 写入绘图信息
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
