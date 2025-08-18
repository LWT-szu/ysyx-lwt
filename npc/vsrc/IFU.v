//取指
<<<<<<< HEAD
import "DPI-C" function int pmem_read(input int raddr,input int pc);
=======
import "DPI-C" function int pmem_read(input int raddr);
>>>>>>> d34687c96b7473e4c27729b0dcd29d99cb48dda7

module IFU (
  input clk,
  input [31:0]pc,//或者直接用 pc!!!????!!!????
  //input [31:0]inst_in,
  output reg [31:0]inst_out
  //注意去掉逗号！！！！！！！！！！！！！！
);
<<<<<<< HEAD
  assign inst_out = pmem_read(pc,pc);
  always @(*) begin//
    $display("pc=%08x",pc);
=======
  always @(posedge clk ) begin//多加了个clk不知道行不行
    inst_out = pmem_read(pc);
    $display("pc=0x%08x", pc);
>>>>>>> d34687c96b7473e4c27729b0dcd29d99cb48dda7
  end

endmodule
