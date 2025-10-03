import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
module IFU (
  input clk,
  input rst,
  input [31:0]pc,
  //nput [31:0]ifu_data,//存储器发送的数据

  //output reg [31:0]infu_raddr,//读地址
  output reg [31:0]inst_out,//输出指令
  output reg inst_valid
  //注意去掉逗号！！！！！！！！！！！！！！
);
  //reg [31:0] req_addr; // 读请求地址

  typedef enum reg [0:0] {IDLE=1'b0, WAIT=1'b1} state_t;
  state_t state;
  always @(posedge clk) begin
    if (rst) begin
      inst_valid <= 1'b0;
      //req_addr  <= 32'h80000000;
      //infu_raddr <= 32'h80000000;
      inst_out <= 32'b0;
      state <= WAIT;
      //$display("IFU Reset: pc=%08x rst=%d", pc,rst);
    end else begin
      case (state)
        IDLE: begin
          //infu_raddr <= pc;// 发起读请求
          //req_addr  <= pc;      // 发起读请求
          inst_valid <= 1'b0;
          state <= WAIT;
          inst_out <= 32'b0;
          //$display("state = %d", state);
        end
        WAIT: begin
          inst_out <= pmem_read(pc,pc,0,0);
          //inst_out <= ifu_data;// 收到数据才允许执行
          //$display("IFU: pc=%08x, inst=%08x", pc, inst_out);
          inst_valid <= 1'b1;
          state <= IDLE;
          //$display("state = %d", state);
        end
      endcase
    end
    
  end
endmodule

/*
  assign ifu_data = pmem_read(pc,pc,0,0);
  always @(*) begin
    //assign inst_out = pmem_read(pc,pc);
    $display("pc=%08x",pc);
  end
  */