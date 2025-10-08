module ysyx_25080201_IFU (
  input clock,
  input reset,
  input [31:0] pc,
  input [31:0] io_ifu_rdata,//LSU->
  input load_wait,
  input io_ifu_respValid,//MEM->IFU
  input LSU_WAIT,
  input lsu_done,

  output [31:0] io_ifu_addr,//->raddr temp from pc
  output reg [31:0] inst_out,//->IDU temp from rdata
  output reg inst_valid,//只是说明指令取出的那个周期
  output reg io_ifu_reqValid,//IFU->MEM
  output state_wait
  //此处直接用了MEM输出的数据，没有传给IFU，因为clock会延迟一拍
  //地址同理，直接把PC输入给MEM
);
  typedef enum reg [0:0] {IDLE=1'b0, WAIT=1'b1} state_t;
  state_t state;
  assign io_ifu_addr = pc;
  assign inst_out = io_ifu_rdata;
  assign state_wait = (state == WAIT);

  always @(posedge clock) begin//
    if (reset) begin
      state <= IDLE;
      inst_valid <= 0;
      io_ifu_reqValid <= 1;
    end else begin
      case (state)
        IDLE: begin
            // idle_hold生效，进入WAIT  第一周期不译码
            inst_valid <= 1;//第二周期译码
            io_ifu_reqValid <= 0;
            state <= WAIT;

        end
        WAIT: begin
          //inst_valid <= 0;
          if (io_ifu_respValid) begin
              if (!load_wait ) begin//此时为第二周期 (load_wait || LSU_WAIT)防止延迟 but time logger
                state <= IDLE;
                inst_valid <= 0;//next 第一周期不译码
                io_ifu_reqValid <= 1;
            end else if(load_wait || LSU_WAIT) begin// 只有 lsu_done==1 才能转IDLE
                state <= WAIT;
                inst_valid <= 0;//第三周期不译码
                io_ifu_reqValid <= 0;
            end
          end  else if(!io_ifu_respValid && (load_wait || LSU_WAIT)) begin// 只有 lsu_done==1 才能转IDLE
                if (lsu_done) begin//防止延迟
                  state <= IDLE;
                  inst_valid <= 0;//next 第一周期不译码 
                  io_ifu_reqValid <= 1;
                end
            end

        end
      endcase
    end
  end
endmodule