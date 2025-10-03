import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
module MEM (
  input        clk,
  input        rst,
  input  [31:0] ifu_raddr,
  output reg [31:0] ifu_rdata
);
  // 存储器本体，可用DPI或者内存数组
  // 用DPI-C方式

  reg [31:0] addr_reg;
  always @(posedge clk) begin
    if (rst) begin
      addr_reg <= 32'h80000000;
      ifu_rdata <= 32'b0;
    end else begin
      // 先保存当前周期请求地址
      addr_reg <= ifu_raddr;
      // 用上一周期地址读出数据
      ifu_rdata <= pmem_read(addr_reg,addr_reg,0,0);
    end
  end
endmodule
/*  MEM MEM_init(  //访存
    .clk(clk),
    .addr(infu_raddr),//读地址
    .inst_valid(inst_valid),

    .data_out(ifu_data)
  );
  */