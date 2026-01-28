#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include "npc.h"

//实在不看不明白，就直接用最下面的版本吧，这里需要注意时间的初始化位置
// 获取当前时间，单位微秒
static uint64_t boot_time = 0;
static uint64_t get_time_internal()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t us = now.tv_sec * 1000000 + now.tv_usec;
    return us;
}

// 在仿真开始前调用一次
void init_timer()
{
    if (boot_time == 0){
        boot_time = get_time_internal();
    }//基准时间
}

// 给 pmem.cpp 或 monitor 调用的接口
// 返回的是：从仿真开始经过的微秒数
uint64_t get_time_in_us()
{
    return get_time_internal() - boot_time;
}

// uint64_t get_time_in_us()
// {
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     uint64_t now = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec; // 秒转微秒再加上微秒
//     if (boot_time == 0)
//         boot_time = now; // 记录启动时间
//     return now - boot_time;
// }
