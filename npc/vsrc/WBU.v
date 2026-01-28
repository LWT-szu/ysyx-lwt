//写入寄存器，更新PC
module WBU (
  // input clk,
  // input rst,
  input [31:0]pc,
  input [31:0]alu_data,//从alu中读取数据
  input [31:0]alu_addr,//从alu中读取的地址 只用到低两位，也会报错
  input [31:0]ram_data,//从ram中读取数据
  input [3:0]waddr,
  input reg_en,
  input Jal_en,
  input jalr_en,
  input is_load_type,
  input is_lbu_type,
  input is_lb_type,
  input is_branch,
  input is_lh_type,
  input is_lhu_type,
  input is_ecall,
  input is_mret,
  input [31:0] csr_mtvec,
  input [31:0] csr_mepc,

  output wb_wen,
  output [3:0]wb_rd,
  output [31:0]wb_Rresult,//写回到寄存器的数据来自ALU,RAM
  output reg [31:0]next_pc,
  input  branch_taken,
  input  [31:0]branch_target
  //注意去掉逗号！！！！！！！！！！！！！！
);
  wire [7:0] lbu_byte;
  wire [15:0] lh_byte;
  reg [31:0] wb_Rresult_reg;//这个东西也可以丢掉了，直接assign wb_Rresult
  //选择器
  assign lbu_byte =
    (alu_addr[1:0] == 2'b00) ? ram_data[7:0] ://////alu_data=0???????
    (alu_addr[1:0] == 2'b01) ? ram_data[15:8] :
    (alu_addr[1:0] == 2'b10) ? ram_data[23:16] :
                               ram_data[31:24];

  assign lh_byte =  
    (alu_addr[1:0] == 2'b00) ? ram_data[15:0] : 
    (alu_addr[1:0] == 2'b10) ? ram_data[31:16] : 16'b0;

  always @(*) begin
    if(is_lbu_type)
      wb_Rresult_reg = {24'b0, lbu_byte} ; //lbu
    else if(is_lb_type)
      wb_Rresult_reg = {{24{lbu_byte[7]}}, lbu_byte} ; //lb  符号扩展
    else if(is_lh_type)
      wb_Rresult_reg = {{16{lh_byte[15]}}, lh_byte} ; //lh  符号扩展
    else if(is_lhu_type)
      wb_Rresult_reg = {16'b0, lh_byte} ;
    else if  (jalr_en || Jal_en)
      wb_Rresult_reg = pc + 32'h4;//jalr jal
    else if(is_load_type)
      wb_Rresult_reg = ram_data;//lw
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
    else if (is_ecall) begin
      next_pc = csr_mtvec;
    end
    else if (is_mret) begin
      next_pc = csr_mepc;
    end

    else begin
      next_pc = pc + 4;
    end
  end

  assign wb_Rresult = wb_Rresult_reg;
  assign wb_wen = reg_en;
  assign wb_rd = waddr;
/*
  assign next_pc = jalr_en ? (alu_data & 32'hfffffffe) :
          Jal_en  ? alu_data :
          pc + 4;
*/
endmodule

