import "DPI-C" function void halt(input int pc,input int halt_ret);
module top (
  input clk,
  input rst,
  //input [31:0]inst,
  output w_ram,
  output is_load_type,
  output [31:0]pc,
  output [31:0]next_pc,
  output [127:0]asm_out1,

  output [31:0] rf[15:0],
  output [31:0] rs2_data,
  output [31:0] rdata_ram,
  output [31:0] wdata_ram,
  output [31:0] alu_ram,//addr
  output [31:0] inst_out,
  output [31:0] zero,//zero
  output [31:0] ra,//ra

  output [31:0] sp,//sp
  output [31:0] gp,//gp
  output [31:0] tp,//tp
  output [31:0] t0,//tp
  output [31:0] t1,//tp
  output [31:0] t2,//tp
  
  output [31:0] s0,//s0
  output [31:0] s1,//s1

  output [31:0] a0,//a0
  output [31:0] a1,//a1
  output [31:0] a2,//a2
  output [31:0] a3,//a3
  output [31:0] a4,//a4
  output [31:0] a5//a5
  
);

  //打印寄存器看看
  genvar i;
  generate
    for (i = 0; i < 16; i = i + 1) begin : rf_export
      assign rf[i] = RegisterFile_init.rf[i];
    end
  endgenerate

  //wire [31:0]next_pc;      // 下一条指令的 PC，由 WBU 产生
  //wire [31:0]inst_out;     // IFU 输出的指令（传给 IDU）

  wire [31:0]imm;          // 立即数解码结果（IDU -> EXU）
  wire [3:0]rd;            // 写回寄存器号（IDU -> WBU）
  wire [3:0]rs1;           // 源寄存器号（IDU -> RegisterFile）

  wire [3:0]rs2;

  wire [2:0]func;          // 功能码（IDU -> EXU）
  wire [6:0]func7;
  wire [6:0]opcode;        // 操作码（IDU -> EXU）
  wire Reg_write;          // 写使能信号（IDU -> WBU/RegFile）
  wire Jal_en;
  wire Jump_en;
  wire add_alu;
  wire ls_vaild;   //访存
  //wire w_ram;
  //wire is_load_type;
  wire is_lbu_type;
  wire is_lb_type;
  wire is_sb_type;
  wire is_sh_type;
  wire is_branch;
  wire is_lh_type;
  wire is_lhu_type;

  wire [31:0]rs1_data;     // 源寄存器数据（RegisterFile -> EXU）
  //wire [31:0]rs2_data;
  wire [31:0]alu_result;   // ALU 计算结果（EXU -> WBU）
  //wire [31:0]alu_ram;
  //wire [31:0]rdata_ram;
  wire branch_taken;
  wire [31:0]branch_target;

  wire wb_wen;             // 写回使能（WBU -> RegisterFile）
  wire [31:0] wb_Rresult;  // 写回数据（WBU -> RegisterFile）
  wire [3:0] wb_rd;        // 写回寄存器号（WBU -> RegisterFile）

  reg [31:0] pc_reg = 32'h80000000;       //保存当前 PC值,实现顺序执行和跳转
  assign pc = pc_reg;      //pc_reg的“输出端口”

  //激励文件中寄存器赋值读取
  
  assign zero = RegisterFile_init.rf[0];
  assign ra = RegisterFile_init.rf[1];

  assign sp = RegisterFile_init.rf[2];//sp
  assign gp = RegisterFile_init.rf[3];//gp
  assign tp = RegisterFile_init.rf[4];//tp
  assign s0 = RegisterFile_init.rf[8];//s0
  assign s1 = RegisterFile_init.rf[9];//s1

  assign a0 = RegisterFile_init.rf[10];//a0
  assign a1 = RegisterFile_init.rf[11];//a1


  assign a3 = RegisterFile_init.rf[13];//a3
  assign a4 = RegisterFile_init.rf[14];//a4
  assign a5 = RegisterFile_init.rf[15];//a5

  
  always @(posedge clk or posedge rst) begin
    
    if (rst==1)
        pc_reg <= 32'h80000000;
    else
        pc_reg <= next_pc;
  end

  //取指
  IFU IFU_init(
    .pc(pc),
    //.inst_in(inst),
    .inst_out(inst_out)//
  );
  //译码
  IDU IDU_init(
  .inst_ym(inst_out),
  .pc(pc),

  .IDU_imm(imm),//
  .IDU_rd(rd),
  .IDU_rs1(rs1),
  .IDU_rs2(rs2),
  .IDU_func(func),
  .IDU_func7(func7),
  .IDU_opcode(opcode),
  .Reg_write(Reg_write),
  .Jal_en(Jal_en),
  .Jump_en(Jump_en),
  .add_alu(add_alu),
  .ls_vaild(ls_vaild),   //访存
  .w_ram(w_ram),          //访存
  .is_load_type(is_load_type),
  .is_lbu_type(is_lbu_type),  //字节区分
  .is_lb_type(is_lb_type),
  .is_sb_type (is_sb_type),
  .is_sh_type(is_sh_type),
  .is_branch(is_branch),
  .is_lh_type(is_lh_type),
  .is_lhu_type(is_lhu_type)

  //注意去掉逗号！！！！！！！！！！！！！！
);

//ALU算术逻辑单元
  EXU EXU_init(
  .pc(pc),
  .imm_alu(imm),//imm
  .rs1_alu(rs1_data),
  .rs2_alu(rs2_data),//x[rs2]
  .alu_src(add_alu),
  .func_alu(func),
  .func7_alu(func7),
  .opcode_alu(opcode),
  .is_branch(is_branch),

  .alu_result(alu_result), //output
  .alu_ram(alu_ram),
  .branch_taken(branch_taken),
  .branch_target(branch_target)
  //注意去掉逗号！！！！！！！！！！！！！！
);

//访存
  ISU ISU_init(
    .clk(clk),
    .valid(ls_vaild),         // 是否有访存请求
    .wen_ram(w_ram),          // 是否是写入
    .raddr_ram(alu_ram),      // 读地址lw,lbu????????
    .waddr_ram(alu_ram),      // 写地址?????
    .wdata_ram(rs2_data),     // 要写入的数据 from gpr
    .is_sb_type(is_sb_type),  // 写掩码
    .is_sh_type(is_sh_type),
    //.is_lh_type(is_lh_type),
    .pc(pc),
    .rdata_ram(rdata_ram)     // out读取内存内容
);



//写入寄存器，更新PC
  WBU WBU_init (
  .pc(pc_reg),
  .alu_data(alu_result),//从alu中读取数据
  .alu_addr(alu_ram),// 读地址lw,lbu--------------------
  .ram_data(rdata_ram),//从ram中读取数据
  .waddr(rd),          //往rd中写入
  .reg_en(Reg_write),
  .Jal_en(Jal_en),
  .jalr_en(Jump_en),
  .is_load_type(is_load_type),
  .is_lbu_type(is_lbu_type),
  .is_lb_type(is_lb_type),
  .is_branch(is_branch),
  .is_lh_type(is_lh_type),
  .is_lhu_type(is_lhu_type),

  .wb_wen(wb_wen),//  output  wb_wen = reg_en
  .wb_rd(wb_rd),
  .wb_Rresult(wb_Rresult),//写回到寄存器的数据来自ALU,RAM
  .next_pc(next_pc),
  .branch_taken(branch_taken),
  .branch_target(branch_target)
  //注意去掉逗号！！！！！！！！！！！！！！
);

  RegisterFile RegisterFile_init(
  .clk(clk),
  .pc(pc),
  .rs1(rs1),
  .rs2(rs2),
  .reg_wdata(wb_Rresult),     // 写入ALU,RAM数据
  .reg_waddr(wb_rd),          // 写入地址rd
  .wen(wb_wen),               // 写使能
  .inst_out(inst_out),
  .rs1_data(rs1_data),
  .rs2_data(rs2_data)
  //注意去掉逗号！！！！！！！！！！！！！！
);

  
endmodule


module RegisterFile(
  input clk,
  input [31:0]pc,
  input [3:0]rs1,
  input [3:0]rs2,
  input [31:0] reg_wdata,     // 接收要写入的alu|ram数据
  input [3:0] reg_waddr,      // 接收要写入的地址rd
  input wen,                  // 写使能
  input [31:0]inst_out,
  output  [31:0] rs1_data,
  output  [31:0] rs2_data
);
  // rf为寄存器数组，大小为32，每个寄存器宽度为32
  reg[31:0] rf[15:0];
  // 写操作：时钟上升沿，当wen为1时，将wdata写入waddr对应的寄存器
  always @(posedge clk) begin
    if (wen && reg_waddr != 0) begin
      rf[reg_waddr] <= reg_wdata; // 0号寄存器保护
    end

  end

  assign rs1_data = (rs1 == 0) ? 32'b0 : rf[rs1];
  assign rs2_data = (rs2 == 0) ? 32'b0 : rf[rs2];//如果 rs2 没有用到或者等于 0 号寄存器，输出就是 0
  always @(*) begin
    if(inst_out == 32'h00100073 && rf[10] == 32'b0)begin
      halt(pc,0);
    end   
    else if(inst_out == 32'h00100073 && rf[10] == 32'b1)begin
      $display("a0=0x%08x",rf[10]);
      halt(pc,1);
    end
  end


endmodule


