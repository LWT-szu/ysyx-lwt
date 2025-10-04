module IFU (
  input clk,
  input rst,
  input [31:0] pc,
  input [31:0] ifu_rdata,
  output reg [31:0] ifu_raddr,
  output reg [31:0] inst_out,
  output reg inst_valid,
  output reg raddr_ready
);
  typedef enum reg [0:0] {IDLE=1'b0, WAIT=1'b1} state_t;
  state_t state;
  always @(posedge clk) begin//
    if (rst) begin
      state <= IDLE;
      inst_valid <= 0;
      inst_out <= 0;
      raddr_ready <= 1;
      ifu_raddr <= 32'h80000000;
      //$display("rst = %d",rst);
      //$display("rst ifu_raddr = 0x%08x",ifu_raddr);
    end else begin
      case (state)
        IDLE: begin
          ifu_raddr <= pc;
          inst_valid <= 1;
          inst_out <= 0;
          raddr_ready <= 0;
          state <= WAIT;
          //$display("IDLE inst_valid = %d",inst_valid);
          //$display("IDLE ifu_raddr = 0x%08x",ifu_raddr);
        end
        WAIT: begin
          inst_out <= ifu_rdata; // 采样上拍地址的数据
          inst_valid <= 0;
          raddr_ready <= 1;
          state <= IDLE;
          //$display("WAIT ifu_rdata = 0x%08x",ifu_rdata);
          //$display("WAIT inst_out = 0x%08x",inst_out);
          
        end
      endcase
    end
  end
endmodule