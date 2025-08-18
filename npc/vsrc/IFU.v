//取指
import "DPI-C" function int pmem_read(input int raddr,input int pc);

module IFU (
  input clk,
  input [31:0]pc,//或者直接用 pc!!!????!!!????
  //input [31:0]inst_in,
  output reg [31:0]inst_out
  //注意去掉逗号！！！！！！！！！！！！！！
);
  assign inst_out = pmem_read(pc,pc);
  always @(*) begin//
    $display("pc=%08x",pc);
  end

endmodule
