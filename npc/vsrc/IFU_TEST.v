module IFU (
  input clk,
  input rst,
  input [31:0] pc,
  input [31:0] ifu_rdata,//LSU->
  input load_wait,

  output [31:0] ifu_raddr,//->raddr temp from pc
  output reg [31:0] inst_out,//->IDU temp from rdata
  output reg inst_valid,
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
      //$display("rst = %d",rst);
      //$display("rst ifu_raddr = 0x%08x",ifu_raddr);
    end else begin
      case (state)
        IDLE: begin
          if (load_wait) begin
            state <= IDLE;
            inst_valid <= 0;
            raddr_ready <= 1;
          end else begin
            // idle_hold生效，进入WAIT
            inst_valid <= 1;
            raddr_ready <= 0;
            state <= WAIT;
          end
          //$display("IDLE inst_valid = %d",inst_valid);
          //$display("IDLE ifu_raddr = 0x%08x",ifu_raddr);
        end
        WAIT: begin
          //inst_valid <= 0;
          raddr_ready <= 1;
          if (!load_wait) begin
            state <= IDLE;
            inst_valid <= 0;
          end else begin
            state <= WAIT;
            inst_valid <= 0;
          end
          //$display("WAIT inst_valid = %d",inst_valid);
          //$display("WAIT ifu_rdata = 0x%08x",ifu_rdata);
          //$display("WAIT inst_out = 0x%08x",inst_out);
          
        end
      endcase
    end
  end
endmodule