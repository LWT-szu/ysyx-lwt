/*索引抽象寄存器，让不同架构都能识别对应的外设
ioe.c相当于一个路由器，决定请求应该交给哪个设备处理。
每个具体设备的.c文件(audio.c、disk.c、gpu.c)等实现了和该设备相关的全部动作,供ioe.c分发调用。
这样整个IOE层就做到了“上层统一、底层分工”。

 * 功能概述：
 * 1. 该文件实现了IOE（I/O Extension）设备访问的分发机制。
 * 2. 通过一个函数指针查找表(lut)，把所有抽象寄存器编号和具体设备处理函数关联起来，
 *    从而统一不同外设的读写操作入口。
 * 3. 上层通过ioe_read/ioe_write接口访问设备时，只需传入抽象寄存器编号和数据缓冲区，
 *    IOE自动将请求派发到正确的设备处理函数。
 */

#include <am.h>
#include <klib-macros.h>

// 声明各个设备的初始化和寄存器处理函数

void __am_timer_init();
void __am_gpu_init();
void __am_audio_init();
void __am_input_keybrd(AM_INPUT_KEYBRD_T *);
void __am_timer_rtc(AM_TIMER_RTC_T *);
void __am_timer_uptime(AM_TIMER_UPTIME_T *);
void __am_gpu_config(AM_GPU_CONFIG_T *);
void __am_gpu_status(AM_GPU_STATUS_T *);
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *);
void __am_audio_config(AM_AUDIO_CONFIG_T *);
void __am_audio_ctrl(AM_AUDIO_CTRL_T *);
void __am_audio_status(AM_AUDIO_STATUS_T *);
void __am_audio_play(AM_AUDIO_PLAY_T *);
void __am_disk_config(AM_DISK_CONFIG_T *cfg);
void __am_disk_status(AM_DISK_STATUS_T *stat);
void __am_disk_blkio(AM_DISK_BLKIO_T *io);

static void __am_timer_config(AM_TIMER_CONFIG_T *cfg) { cfg->present = true; cfg->has_rtc = true; }
static void __am_input_config(AM_INPUT_CONFIG_T *cfg) { cfg->present = true;  }
static void __am_uart_config(AM_UART_CONFIG_T *cfg)   { cfg->present = false; }
static void __am_net_config (AM_NET_CONFIG_T *cfg)    { cfg->present = false; }

// handler_t 是所有设备处理函数的统一类型
typedef void (*handler_t)(void *buf);
// 查找表：reg编号 -> 设备处理函数
static void *lut[128] = {
  [AM_TIMER_CONFIG] = __am_timer_config,
  [AM_TIMER_RTC   ] = __am_timer_rtc,
  [AM_TIMER_UPTIME] = __am_timer_uptime,
  [AM_INPUT_CONFIG] = __am_input_config,
  [AM_INPUT_KEYBRD] = __am_input_keybrd,
  [AM_GPU_CONFIG  ] = __am_gpu_config,
  [AM_GPU_FBDRAW  ] = __am_gpu_fbdraw,
  [AM_GPU_STATUS  ] = __am_gpu_status,
  [AM_UART_CONFIG ] = __am_uart_config,
  
  [AM_AUDIO_CONFIG] = __am_audio_config,
  [AM_AUDIO_CTRL  ] = __am_audio_ctrl,
  [AM_AUDIO_STATUS] = __am_audio_status,
  [AM_AUDIO_PLAY  ] = __am_audio_play,
  [AM_DISK_CONFIG ] = __am_disk_config,
  [AM_DISK_STATUS ] = __am_disk_status,
  [AM_DISK_BLKIO  ] = __am_disk_blkio,
  [AM_NET_CONFIG  ] = __am_net_config,
};

// 默认处理：访问了不存在的寄存器编号时报错
static void fail(void *buf) { panic("access nonexist register"); }

// 初始化IOE，补全查找表，执行各设备的初始化
bool ioe_init() {
  for (int i = 0; i < LENGTH(lut); i++)
    if (!lut[i]) lut[i] = fail;
  __am_gpu_init();
  __am_timer_init();
  __am_audio_init();
  return true;
}

/*IOE层的统一设备访问接口——它们都会通过 lut[reg] 找到对应的设备处理函数，
然后把 buf 传进去，完成设备的读/写操作*/

//  两个函数都是通过抽象寄存器的编号索引到一个处理函数, 
//  然后调用它. 处理函数的具体功能和寄存器编号相关

//  这里的reg寄存器并不是上文讨论的设备寄存器, 
//  因为设备寄存器的编号是架构相关的，这个reg其实是一个【功能编号】, 
//  我们约定在不同的架构中, 同一个功能编号的含义也是相同的

// 统一的设备读入口：通过抽象寄存器编号和数据缓冲区访问设备
// 从编号为reg的寄存器中读出内容到缓冲区buf中
void ioe_read (int reg, void *buf) { ((handler_t)lut[reg])(buf); }

// 统一的设备写入口
// 向编号为reg的寄存器写入缓冲区buf中的内容
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }

//klib中提供了io_read()和io_write()这两个宏, 
//它们分别对ioe_read()和ioe_write()这两个API进行了进一步的封装
