import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
module MEM(
  input clk,
  input [31:0] ifu_raddr,
  input inst_valid,
  input raddr_ready,
  output reg [31:0] ifu_rdata
);
  always @(posedge clk) begin
    if(raddr_ready) ifu_rdata <= pmem_read(ifu_raddr,ifu_raddr,0,0);
  end
endmodule