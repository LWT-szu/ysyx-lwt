import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
import "DPI-C" function void pmem_write(input  int waddr, input int  wdata, input byte wmask,input int pc);
module ysyx_25080201_MEM(
  input clock,
  input reset,
  input [31:0] io_ifu_addr,
  input io_ifu_reqValid,
  
  input [31:0] io_lsu_addr,
  input        io_lsu_wen,
  input [31:0] io_lsu_wdata,
  input [ 3:0] io_lsu_wmask,
  input io_lsu_reqValid,//来自CPU (LSU)      告诉 CPU：“我现在真的有一个读/写请求了，请你处理！”
  output reg io_lsu_respValid,//返回给 CPU (LSU)      告诉 CPU：“我已经把你要的数据准备好了，现在你可以用 rdata/结果了！”
  output reg [31:0] io_lsu_rdata,

  output reg [31:0] io_ifu_rdata,
  output reg io_ifu_respValid
);
  //reg state_fan = !state_wait;
  always @(posedge clock) begin
    if(reset) begin
      io_lsu_respValid <= 0;
      io_lsu_rdata <= 32'b0;
      io_ifu_rdata <= 32'b0;
      io_ifu_respValid <= 1'b0;
      //$display("reset mem = %d",reset);
      //$display("reset io_ifu_addr = 0x%08x",io_ifu_addr);
    end else if(io_ifu_reqValid) begin
      io_ifu_rdata <= pmem_read(io_ifu_addr,io_ifu_addr,0,0);  // IDLE请求译码
      io_ifu_respValid <= 1'b1;

    end else begin                    //一般情况
      //io_ifu_rdata <= 0;
      io_ifu_respValid <= 1'b0;
    end

    if(io_lsu_wen && io_lsu_reqValid) begin
      pmem_write(io_lsu_addr, io_lsu_wdata, {4'b0,io_lsu_wmask},io_ifu_addr);
      //$display("MEM write: addr=0x%08x data=0x%08x mask=0x%02x pc=0x%08x", io_lsu_addr, io_lsu_wdata, io_lsu_wmask,io_ifu_addr);
    end
    if(io_lsu_reqValid && !io_lsu_wen) begin
      io_lsu_rdata <= pmem_read(io_lsu_addr,io_ifu_addr,{31'b0, io_lsu_reqValid},{31'b0, io_lsu_wen});
      io_lsu_respValid <= io_lsu_reqValid;
      //$display("MEM read: addr=0x%08x data=0x%08x pc=0x%08x", io_lsu_addr, io_lsu_rdata,io_ifu_addr);
    end else begin
      io_lsu_respValid <= 0;
      io_lsu_rdata <= 32'b0;
    end
  end
endmodule