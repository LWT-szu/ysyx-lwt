//取指
import "DPI-C" function int pmem_read(input int raddr);

module IFU (
  input clk,
  input [31:0]pc,//或者直接用 pc!!!????!!!????
  //input [31:0]inst_in,
  output reg [31:0]inst_out
  //注意去掉逗号！！！！！！！！！！！！！！
);
  always @(posedge clk ) begin//多加了个clk不知道行不行
    inst_out = pmem_read(pc);
    $display("pc=0x%08x", pc);
  end

endmodule
