import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
module IFU (
  input clk,
  input rst,
  input [31:0]infu_raddr,//pc
  //input load_wait, // load指令等待信号
  //nput [31:0]ifu_data,//存储器发送的数据

  //output reg [31:0]infu_raddr,//读地址
  output reg [31:0]ifu_data,//输出指令 inst_out
  output reg inst_valid 
  //注意去掉逗号！！！！！！！！！！！！！！
);
  // 状态定义
  typedef enum reg [0:0] {IDLE=1'b0, WAIT=1'b1} state_t;
  state_t current_state, next_state;

  reg [31:0] pmem_data;
  
  // ==================== 第一段：状态寄存器 ====================
  always @(posedge clk or posedge rst) begin
    if (rst) begin
      current_state <= IDLE;//IDLE
      inst_valid <= 1'b0;
      //infu_raddr <= 32'h80000000;
      ifu_data <= 32'b0;
    end else begin
      current_state <= next_state;

      case (current_state)
        IDLE:begin
          ifu_data <= 32'b0;
          inst_valid <= 1'b0;
        end
        WAIT: begin
          ifu_data <= pmem_data;// 使用第三段读取的数据
          inst_valid <= 1'b1;
        end 
        //default: 
      endcase
    end
  end

  // ==================== 第二段：状态转移逻辑 ====================
  always @(*) begin
      case (current_state)
        IDLE: begin
          // 在IDLE状态，总是转移到WAIT状态发起取指
          next_state = WAIT;
          /*
          if (!load_wait)begin
            state <= WAIT;
          end else begin
            state <= IDLE;
          end
          */
        end
        WAIT: begin
          next_state = IDLE;
        end
        default: begin
          next_state = IDLE;
        end
      endcase
    end
    
  // ==================== 第三段：输出逻辑 ====================
  always @(*) begin
    case (current_state)
      IDLE: begin
        pmem_data = 32'b0;
        //$display("IFU IDLE: pc=%08x", pc);
      end
      WAIT: begin
        pmem_data = pmem_read(infu_raddr,infu_raddr,0,0);
      end
      default: begin
        pmem_data = 32'b0;
      end
    endcase
  end
endmodule

/*
  assign ifu_data = pmem_read(pc,pc,0,0);
  always @(*) begin
    //assign inst_out = pmem_read(pc,pc);
    $display("pc=%08x",pc);
  end
  */