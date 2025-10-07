import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
import "DPI-C" function void pmem_write(input  int waddr, input int  wdata, input byte wmask,input int pc);
module MEM(
  input clk,
  input [31:0] ifu_raddr,
  input inst_valid,
  input raddr_ready,
  input load_wait,
  input state_wait,
  input lsu_valid,
  
  input [31:0] lsu_addr,
  input        lsu_wen,
  input [31:0] lsu_wdata,
  input [ 7:0] lsu_wmask,
  output reg [31:0] lsu_rdata,

  output reg [31:0] ifu_rdata,
  output reg reg_load_wait
);
  reg state_fan = !state_wait;
  always @(posedge clk) begin
    if(!state_wait) begin
      ifu_rdata <= pmem_read(ifu_raddr,ifu_raddr,0,0);
      reg_load_wait <= 1'b0;
    end else if (load_wait) begin
      reg_load_wait <= 1'b1;
    end else begin
      ifu_rdata <= 0;
      reg_load_wait <= 1'b0;
    end

    if(lsu_wen && lsu_valid) begin
      pmem_write(lsu_addr, lsu_wdata, lsu_wmask,ifu_raddr);
      //$display("MEM write: addr=0x%08x data=0x%08x mask=0x%02x pc=0x%08x", lsu_addr, lsu_wdata, lsu_wmask,ifu_raddr);
    end
    if(lsu_valid && !lsu_wen) begin
      lsu_rdata <= pmem_read(lsu_addr,ifu_raddr,{31'b0, lsu_valid},{31'b0, lsu_wen});
      //$display("MEM read: addr=0x%08x data=0x%08x pc=0x%08x", lsu_addr, lsu_rdata,ifu_raddr);
    end
    //$display("MEM ifu_rdata = 0x%08x",ifu_rdata);
  end
endmodule