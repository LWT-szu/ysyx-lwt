//写入寄存器，更新PC
module WBU (
  input clk,
  input rst,
  input [31:0]pc,
  input [31:0]alu_data,//从alu中读取数据
  input [31:0]alu_addr,//从alu中读取的地址
  input [31:0]ram_data,//从ram中读取数据
  input [4:0]waddr,
  input reg_en,
  input jalr_en,
  input is_load_type,
  input is_lbu_type,
  
  output wb_wen,
  output [4:0]wb_rd,
  output [31:0]wb_Rresult,//写回到寄存器的数据来自ALU,RAM
  output reg [31:0]next_pc
  //注意去掉逗号！！！！！！！！！！！！！！
);
  wire [7:0] lbu_byte;
  //选择器
  assign lbu_byte =
    (alu_addr[1:0] == 2'b00) ? ram_data[7:0] ://////alu_data=0???????
    (alu_addr[1:0] == 2'b01) ? ram_data[15:8] :
    (alu_addr[1:0] == 2'b10) ? ram_data[23:16] :
                               ram_data[31:24];
    
  //优先级 lbu > jalr > load > 普通算术,选择器
  assign wb_Rresult = 
    is_lbu_type         ?   {24'b0, lbu_byte} : //lbu
    (jalr_en == 1)      ?   (pc + 32'h4) : //jalr
    (is_load_type == 1) ?   ram_data : //lw
                            alu_data ;//add

  assign wb_wen = reg_en;
  assign wb_rd = waddr;
  assign next_pc = (jalr_en) ? (alu_data & 32'hfffffffe) : (pc + 4);

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