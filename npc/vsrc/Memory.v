import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
module MEM(
  input clk,
  input [31:0] ifu_raddr,
  input inst_valid,
  input raddr_ready,
  input load_wait,
  input state_wait,
  output reg [31:0] ifu_rdata,
  output reg reg_load_wait
);
  reg state_fan = !state_wait;
  always @(posedge clk) begin
    if(!state_wait) begin
      ifu_rdata <= pmem_read(ifu_raddr,ifu_raddr,0,0);
      reg_load_wait <= 1'b0;
    end else if (load_wait) begin
      ifu_rdata <= pmem_read(ifu_raddr,ifu_raddr,1,0);//第三周期也要取出这个指令
      reg_load_wait <= 1'b1;
    end else begin
      ifu_rdata <= 0;
      reg_load_wait <= 1'b0;
    end
    //$display("MEM ifu_rdata = 0x%08x",ifu_rdata);
  end
endmodule