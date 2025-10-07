module IFU (
  input clk,
  input rst,
  input [31:0] pc,
  input [31:0] ifu_rdata,//LSU->
  input load_wait,
  input decode_ready,

  output [31:0] ifu_raddr,//->raddr temp from pc
  output reg [31:0] inst_out,//->IDU temp from rdata
  output reg inst_valid,//只是说明指令取出的那个周期
  output reg raddr_ready,
  output state_wait
  //此处直接用了MEM输出的数据，没有传给IFU，因为clk会延迟一拍
  //地址同理，直接把PC输入给MEM
);
  typedef enum reg [0:0] {IDLE=1'b0, WAIT=1'b1} state_t;
  state_t state;
  assign ifu_raddr = pc;
  assign inst_out = ifu_rdata;
  assign state_wait = (state == WAIT);

  always @(posedge clk) begin//
    if (rst) begin
      state <= IDLE;
      inst_valid <= 0;
      raddr_ready <= 1;
    end else begin
      case (state)
        IDLE: begin
            // idle_hold生效，进入WAIT  第一周期不译码
            inst_valid <= 1;//第二周期译码
            raddr_ready <= 0;
            state <= WAIT;

        end
        WAIT: begin
          //inst_valid <= 0;
          raddr_ready <= 1;
          if (!load_wait ) begin//此时为第二周期
            state <= IDLE;
            inst_valid <= 0;//next 第一周期不译码
          end else if(load_wait) begin
            state <= WAIT;
            inst_valid <= 0;//第三周期不译码
          end

        end
      endcase
    end
  end
endmodule