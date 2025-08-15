#include <verilated.h>

extern "C" void halt(){
    vl_finish(__FILE__, __LINE__, "");
}