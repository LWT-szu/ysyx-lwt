#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  
  uint32_t gpu_t = inl(VGACTL_ADDR);
  int i;
  int w = gpu_t >> 16;
  int h = gpu_t & 0xFFFF;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i++) fb[i] = i;//彩虹条/渐变色效果
  outl(SYNC_ADDR, 1); // 刷新
  
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
  uint32_t scre = inl(VGACTL_ADDR);
  int scre_w = scre >>16;
  int i,j;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  // 把小矩形里的像素，一行一行地复制到framebuffer的指定位置
  //(x,y) 是小矩形左上角的点
  // ctl->w 绘制的小矩形的宽度
  // (ctl->y + i) 第i行在整屏里的行号
  // (ctl->x + j) 第j列在整屏里的列号
  // px_i 是要拷贝的像素在“小矩形”pixels数组里的下标
  // FBI 是目标像素在大屏幕上的位置
  for(i=0;i<ctl->h;i++){
    for(j=0;j<ctl->w;j++){
      int FBI = (ctl->y + i)*scre_w + (ctl->x + j);
      int px_i = i*ctl->w + j;
      fb[FBI] = ((uint32_t *)ctl->pixels)[px_i];
    }
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
