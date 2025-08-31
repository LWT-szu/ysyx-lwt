//取指
import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);

module IFU (
  input clk,
  input [31:0]pc,//或者直接用 pc!!!????!!!????
  //input [31:0]inst_in,
  output reg [31:0]inst_out
  //注意去掉逗号！！！！！！！！！！！！！！
);
  assign inst_out = pmem_read(pc,pc,0,0);
  
  /*
  always @(*) begin
    //assign inst_out = pmem_read(pc,pc);
    $display("pc=%08x",pc);
  end
  */
  
endmodule
