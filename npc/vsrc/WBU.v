//写入寄存器，更新PC
module WBU (
  input clk,
  input rst,
  input [31:0]pc,
  input [31:0]alu_data,//从alu中读取数据
  input [31:0]alu_addr,//从alu中读取的地址
  input [31:0]ram_data,//从ram中读取数据
  input [3:0]waddr,
  input reg_en,
  input Jal_en,
  input jalr_en,
  input is_load_type,
  input is_lbu_type,
  input is_branch,
  input is_lh_type,
  input is_lhu_type,
  input csr_write,
  input [11:0] csr_addr, // csr address
  input [31:0] rs1_data, // csr write data
  input [63:0] mcycle,           // cycle计数器
  input [31:0] mvendorid, // ysyx
  input [31:0] marchid,   // student_ID
  input reg_load_wait,
  input state_wait,
  input load_wait,

  output wb_wen,
  output [3:0]wb_rd,
  output [31:0]wb_Rresult,//写回到寄存器的数据来自ALU,RAM
  output reg [31:0]next_pc,
  output reg branch_taken,
  output reg [31:0]branch_target,
  output reg [63:0] wbu_mcycle // cycle计数器
  //注意去掉逗号！！！！！！！！！！！！！！
);
  wire [7:0] lbu_byte;
  wire [15:0] lh_byte;
  reg [31:0] wb_Rresult_reg;
  reg wen_load ;
  reg [3:0] rd_load;
  reg [31:0] Result_load;
  reg lbu_wait;
  reg lw_wait;//is_load_type的延迟信号

  //选择器
  assign lbu_byte =
    (alu_addr[1:0] == 2'b00) ? ram_data[7:0] ://////alu_data=0???????
    (alu_addr[1:0] == 2'b01) ? ram_data[15:8] :
    (alu_addr[1:0] == 2'b10) ? ram_data[23:16] :
                               ram_data[31:24];

  assign lh_byte =  
    (alu_addr[1:0] == 2'b00) ? ram_data[15:0] : 
    (alu_addr[1:0] == 2'b10) ? ram_data[31:16] : 16'b0;
  /*
  //优先级 lbu > jalr > load > 普通算术,选择器
  assign wb_Rresult = 
    is_lbu_type         ?   {24'b0, lbu_byte} : //lbu
    (jalr_en == 1 || Jal_en == 1)      ?   (pc + 32'h4) : //jalr jal
    (is_load_type == 1) ?   ram_data : //lw
                            alu_data ;//add
  */
  always @(*) begin
    if(lbu_wait)
      wb_Rresult_reg = {24'b0, lbu_byte} ; //lbu
    else if(is_lh_type)
      wb_Rresult_reg = {{16{lh_byte[15]}}, lh_byte} ; //lh  符号扩展
    else if(is_lhu_type)
      wb_Rresult_reg = {16'b0, lh_byte} ;//lhu 零扩展

    else if  (jalr_en || Jal_en)
      wb_Rresult_reg = pc + 32'h4;//jalr jal

    else if(lw_wait && !lbu_wait)
      wb_Rresult_reg = ram_data;//lw

    else if(csr_write)
      case(csr_addr)
        12'hB00: begin
          wb_Rresult_reg = alu_data; // mcycle
          wbu_mcycle = {mcycle[63:32], rs1_data}; // 低32位写入rs1_data
        end

        12'hB80: begin
          wb_Rresult_reg = alu_data; // mcycleh
          wbu_mcycle = {rs1_data , mcycle[31:0]}; // 高32位写入rs1_data
        end

        12'hF11: begin
          wb_Rresult_reg = mvendorid; // mvendorid
        end
        12'hF12: begin
          wb_Rresult_reg = marchid; // marchid
        end
        default: wb_Rresult_reg = 32'b0;
      endcase
    else 
      wb_Rresult_reg = alu_data;//add sub addi
      //$display("wb_Rresult_reg = %08x",wb_Rresult_reg);

    if(is_branch && branch_taken)begin
      next_pc = branch_target;
    end

    else if (jalr_en) begin
      next_pc = alu_data & 32'hfffffffe;
    end

    else if (Jal_en) begin
      next_pc =alu_data;
    end

    else begin
      next_pc = pc + 4;
    end


  end

  always @(posedge clk) begin
    if(state_wait) begin
      wen_load <= reg_en;
      rd_load <= waddr;
      Result_load <= wb_Rresult_reg;
      lbu_wait <= is_lbu_type;
      lw_wait <= is_load_type;
    end else begin
      wen_load <= 0;
      rd_load <= 4'b0;
      Result_load <= 32'b0;
    end
  end
  //load_wait ? 0 : (reg_load_wait ? wen_load : reg_en);

  assign wb_Rresult =  wb_Rresult_reg;//区分load指令和普通指令
  assign wb_wen = load_wait ? 0 : (reg_load_wait ? wen_load : reg_en);//区分load指令和普通指令
  assign wb_rd = reg_load_wait ? rd_load : waddr;//区分load指令和普通指令
/*
  assign next_pc = jalr_en ? (alu_data & 32'hfffffffe) :
          Jal_en  ? alu_data :
          pc + 4;
*/
endmodule
    /*always @(*) begin
    if (wb_wen) begin
      if (is_lbu_type)
        $display("[WBU] lbu: wb_Rresult=0x%08x, writre_to_rd=%0d\n", wb_Rresult, wb_rd);
      else if (is_load_type)
        $display("[WBU] lw: wb_Rresult=0x%08x, writre_to_rd=%0d\n", wb_Rresult, wb_rd);
    end
  end*/
  
  /*
  always @(*) begin
    if (rst == 1) next_pc <= 32'h80000000;
    else if(jalr_en == 1) next_pc <= alu_data & 32'hfffffffe;//最低位清零
    else next_pc <= pc + 32'h00000004;
  end
  */