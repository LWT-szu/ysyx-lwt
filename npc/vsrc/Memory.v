import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
import "DPI-C" function void pmem_write(input  int waddr, input int  wdata, input byte wmask,input int pc);
module MEM(
  input clk,
  input rst,
  input [31:0] ifu_raddr,
  input inst_valid,
  input raddr_ready,
  input load_wait,
  input state_wait,
  
  input [31:0] lsu_addr,
  input        lsu_wen,
  input [31:0] lsu_wdata,
  input [ 7:0] lsu_wmask,
  input lsu_reqValid,//来自CPU (LSU)      告诉 CPU：“我现在真的有一个读/写请求了，请你处理！”
  output reg lsu_respValid,//返回给 CPU (LSU)      告诉 CPU：“我已经把你要的数据准备好了，现在你可以用 rdata/结果了！”
  output reg [31:0] lsu_rdata,

  output reg [31:0] ifu_rdata,
  output reg reg_load_wait,
  output reg decode_ready
);
  reg state_fan = !state_wait;
  always @(posedge clk) begin
    if(rst) begin
      lsu_respValid <= 0;
      lsu_rdata <= 32'b0;
      ifu_rdata <= 32'b0;
      reg_load_wait <= 1'b0;
      decode_ready <= 1'b0;
      //$display("rst mem = %d",rst);
      //$display("rst ifu_raddr = 0x%08x",ifu_raddr);
    end else if(!state_wait) begin
      ifu_rdata <= pmem_read(ifu_raddr,ifu_raddr,0,0);  // IDLE请求译码
      reg_load_wait <= 1'b0;
      decode_ready <= 1'b1;

    end else if (load_wait) begin     //WAIT
      reg_load_wait <= 1'b1;
      decode_ready <= 1'b0;
    end else begin                    //一般情况
      ifu_rdata <= 0;
      reg_load_wait <= 1'b0;
      decode_ready <= 1'b0;
    end

    if(lsu_wen && lsu_reqValid) begin
      pmem_write(lsu_addr, lsu_wdata, lsu_wmask,ifu_raddr);
      //$display("MEM write: addr=0x%08x data=0x%08x mask=0x%02x pc=0x%08x", lsu_addr, lsu_wdata, lsu_wmask,ifu_raddr);
    end
    if(lsu_reqValid && !lsu_wen) begin
      lsu_rdata <= pmem_read(lsu_addr,ifu_raddr,{31'b0, lsu_reqValid},{31'b0, lsu_wen});
      lsu_respValid <= lsu_reqValid;
      //$display("MEM read: addr=0x%08x data=0x%08x pc=0x%08x", lsu_addr, lsu_rdata,ifu_raddr);
    end else begin
      lsu_respValid <= 0;
      lsu_rdata <= 32'b0;
    end
  end
endmodule