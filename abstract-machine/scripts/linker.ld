ENTRY(_start)/*//这是指定程序入口点的部分。它告诉链接器程序PC应该从 _start 标签处开始执行。*/
PHDRS { text PT_LOAD; data PT_LOAD; }

/*这部分定义了链接器如何组织不同的段。*/
SECTIONS {
  /* _pmem_start and _entry_offset are defined in LDFLAGS */
  . = _pmem_start + _entry_offset;   /*-defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0*/
  /* 设置当前的*位置计数器 . */
  .text : {
    *(entry)  /*将 entry 部分的内容（程序的入口代码）复制到 .text 节中*/
    *(.text*) /*将所有以 .text 开头的小节内容（通常是程序的代码）复制到 .text 节中*/
  } : text    /*指派该节属于 PHDRS 中定义的 text 段*/
  etext = .;  
  _etext = .; /*定义符号 etext 和 _etext，其值为当前地址，表示.text 节的末尾*/
  .rodata : {
    *(.rodata*) /*放置所有只读数据（常量字符串、const 变量）。*/
  }
  .data : {   /*数据段 放置已初始化的全局变量和静态变量*/
    *(.data*)
  } : data
  edata = .;
  _data = .;
  .bss : {    /*未初始化数据段*/
	_bss_start = .; /*记录 BSS 段的开始，用于启动代码执行 memset 清零操作*/
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x1000);/*当前地址按 4KB (0x1000) 向上对齐，并将对齐后的地址赋给 _stack_top。这是栈底（低地址端）*/
  . = _stack_top + 0x8000;
  /*栈的大小是 32KB*/
  _stack_pointer = .; /*定义 _stack_pointer 符号指向当前位置（栈的高地址端）*/
  end = .;            /*标记程序使用的静态内存（代码+数据+栈）的总结束位置*/
  _end = .;
  _heap_start = ALIGN(0x1000);/*定义堆（Heap）的起始地址 _heap_start*/
}
