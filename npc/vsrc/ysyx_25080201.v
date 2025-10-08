import "DPI-C" function void halt(input int pc,input int halt_ret);
module ysyx_25080201 (
    input  clock,
    input  reset,
    output reg [31:0] io_ifu_addr,
    output reg        io_ifu_reqValid,
    output reg [31:0] io_ifu_rdata,
    output reg        io_ifu_respValid,

    output reg [31:0] io_lsu_addr,
    output reg        io_lsu_reqValid,
    output reg [31:0] io_lsu_rdata,
    output reg        io_lsu_respValid,
    output reg [1:0]  io_lsu_size,
    output         io_lsu_wen,
    output reg [31:0] io_lsu_wdata,
    output [3:0]  io_lsu_wmask
); 

  //打印寄存器看看
  /*
  genvar i;
  generate 
    for (i = 0; i < 16; i = i + 1) begin : rf_export
      assign rf[i] = RegisterFile_init.rf[i];
    end
  endgenerate
  */
  wire [31:0]next_pc;      // 下一条指令的 PC，由 WBU 产生
  //wire [31:0]io_ifu_rdata;//存储器发送的数据 MEM-->IFU
  //wire [31:0]io_ifu_addr;//读地址IFU-->MEM

  wire [31:0]inst_out;     // IFU 输出的指令（传给 IDU）

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
  wire w_ram;
  wire is_load_type;
  wire is_lbu_type;
  wire is_sb_type;
  wire is_sh_type;
  wire is_branch;
  wire is_lh_type;
  wire is_lhu_type;

  wire inst_valid;//指令是否有效
  //wire io_ifu_reqValid;
  wire load_wait; // load指令等待信号
  wire state_wait;
  wire reg_load_wait; //load指令next等待信号WBU->RegFile

  wire [31:0]rs1_data;     // 源寄存器数据（RegisterFile -> EXU）
  wire [31:0]rs2_data;
  wire [31:0]alu_result;   // ALU 计算结果（EXU -> WBU）
  wire [31:0]alu_ram;
  wire [31:0]rdata_ram;
  wire branch_taken;
  wire [31:0]branch_target;

  wire wb_wen;             // 写回使能（WBU -> RegisterFile）
  wire [31:0] wb_Rresult;  // 写回数据（WBU -> RegisterFile）
  wire [3:0] wb_rd;        // 写回寄存器号（WBU -> RegisterFile）
  wire csr_write;         // 是否为CSR寄存器写入

  reg [31:0] pc_reg = 32'h30000000;       //保存当前 PC值,实现顺序执行和跳转
  reg [31:0] pc = 32'h30000000;
  assign pc[31:0] = pc_reg;      //pc_reg的“输出端口”

  //================LSU======================
  //wire [31:0] io_lsu_rdata;//从MEM中读取数据 MEM->LSU
  //
  //wire [31:0] io_lsu_addr;//LSU->MEM
  //wire        io_lsu_wen;
  //wire [31:0] io_lsu_wdata;
  //wire [ 3:0] io_lsu_wmask;
  //wire        io_lsu_respValid;//返回给 CPU (LSU)      告诉 CPU：“我已经把你要的数据准备好了，现在你可以用 rdata/结果了！”
  //wire        io_lsu_reqValid;//发给存储器（MEM）    告诉存储器：“我现在真的有一个读/写请求了，请你处理！”
  wire        lsu_done; //访存完成标志
  //wire        io_ifu_respValid;
  wire        LSU_WAIT;
  //wire [ 1:0] io_lsu_size;//  I/O
  //================LSU======================

  //激励文件中寄存器赋值读取
  /*
  assign zero = RegisterFile_init.rf[0];
  assign ra = RegisterFile_init.rf[1];
  assign sp = RegisterFile_init.rf[2];//sp
  assign gp = RegisterFile_init.rf[3];//gp
  assign tp = RegisterFile_init.rf[4];//tp
  assign s0 = RegisterFile_init.rf[8];//s0
  assign s1 = RegisterFile_init.rf[9];//s1
  assign a0 = RegisterFile_init.rf[10];//a0
  assign a1 = RegisterFile_init.rf[11];//a1
  assign a2 = RegisterFile_init.rf[12];//a2
  assign a3 = RegisterFile_init.rf[13];//a3
  assign a4 = RegisterFile_init.rf[14];//a4
  assign a5 = RegisterFile_init.rf[15];//a5
  */
//================CSR======================
  reg [63:0] mcycle;//cycle计数器
  reg [63:0] wbu_mcycle;//wbu cycle mcycle需要用一个中间变量暂存,然后在顶层模块赋值
  reg [31:0] mvendorid = 32'h79737978;//ysyx 32'h79737978
  reg [31:0] marchid = 32'h25080201;//STUDENT_ID 32'h017EB189 32'd2025080201
//================CSR======================
  
  always @(posedge clock or posedge reset) begin
    
    if (reset==1)begin
      pc_reg <= 32'h30000000;
      mcycle <= 0;
      //$display("Top Reset: pc=%08x reset=%d", pc_reg,reset);
    end else if(csr_write && io_ifu_respValid && !LSU_WAIT) begin
      mcycle <= wbu_mcycle;
      pc_reg <= next_pc; 
      //wd <= state_wait || lsu_done;
      //wl <= state_wait && !load_wait;
      // 只有在没有load等待、且指令有效时才允许更新PC
    end else if(io_ifu_respValid && !(LSU_WAIT || load_wait || lsu_done)) begin //load指令外的普通指令PC更新,防止译码延迟
    //同时也能预防译码的延迟，因为此时load相关的信号为0
      pc_reg <= next_pc;
      mcycle <= mcycle + 1;
      //wl <= state_wait && !load_wait;
      //wd <= state_wait || lsu_done;
      //$display("Top Reset: inst_valid=%d ", inst_valid);
    
    end else if (lsu_done) begin//load,store指令的PC更新 防止读取内存延迟 
    //即使译码延迟也没关系，因为两周期的指令，在第二个周期译码和lsu_done同时为1
        pc_reg <= next_pc;
        mcycle <= mcycle + 1;
    end
    
    //$display("[PC_DBG] t=%0t reset=%0d pc_reg=%08x", $time, reset, pc_reg);
    //注意跳转的情况，如果跳到了其他地方，这里不会打印
/*
    if(pc < 32'h80000030 && pc > 32'h80000014)begin
      $display("PC: %08x, inst: %08x, csr_write: %d, next_pc: %08x, mcycle: %08x, mcycleh: %08x", pc_reg, inst_out, csr_write, next_pc, mcycle[31:0], mcycle[63:32]);
    end
    */
  end
/*
  ysyx_25080201_MEM MEM_init (
  .clock(clock),
  .reset(reset),
  .io_ifu_addr(io_ifu_addr),
  .io_ifu_reqValid(io_ifu_reqValid),
  .io_lsu_addr(io_lsu_addr),//LSU->MEM
  .io_lsu_wen(io_lsu_wen),
  .io_lsu_wdata(io_lsu_wdata),
  .io_lsu_wmask(io_lsu_wmask),
  .io_lsu_reqValid(io_lsu_reqValid),//来自CPU (LSU)      告诉 MEM：“我现在真的有一个读/写请求了，请你处理！”
  .io_lsu_respValid(io_lsu_respValid),//返回给 CPU (LSU)      告诉 CPU：“我已经把你要的数据准备好了，现在你可以用 rdata/结果了！”

  .io_lsu_rdata(io_lsu_rdata),//MEM->LSU
  .io_ifu_rdata(io_ifu_rdata),
  .io_ifu_respValid(io_ifu_respValid)
);
*/

  //取指
  ysyx_25080201_IFU IFU_init(
    .clock(clock),
    .reset(reset),
    .pc(pc),
    .io_ifu_rdata(io_ifu_rdata),//存储器发送的数据
    .load_wait(load_wait),// load指令等待信号
    .io_ifu_respValid(io_ifu_respValid),//译码准备好
    .LSU_WAIT(LSU_WAIT),
    .lsu_done(lsu_done),


    .io_ifu_addr(io_ifu_addr),//请求读存储器地址 pc
    .inst_out(inst_out),//输出指令
    //.inst_valid(inst_valid),
    .io_ifu_reqValid(io_ifu_reqValid),
    .state_wait(state_wait)
  );
  //译码
  ysyx_25080201_IDU IDU_init(
  .inst_ym(inst_out),//inst_out
  .pc(pc),
  .io_ifu_respValid(io_ifu_respValid),
  .clock(clock),

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
  .is_sb_type (is_sb_type),
  .is_sh_type(is_sh_type),
  .is_branch(is_branch),
  .is_lh_type(is_lh_type),
  .is_lhu_type(is_lhu_type),
  .csr_write(csr_write)

  //注意去掉逗号！！！！！！！！！！！！！！
);

//ALU算术逻辑单元
  ysyx_25080201_EXU EXU_init(
  .pc(pc),
  .imm_alu(imm),//imm
  .rs1_alu(rs1_data),//x[rs1]
  .rs2_alu(rs2_data),//x[rs2]
  .alu_src(add_alu),
  .func_alu(func),
  .func7_alu(func7),
  .opcode_alu(opcode),
  .is_branch(is_branch),
  .mcycle(mcycle),           // cycle计数器
  .csr_write(csr_write),

  .alu_result(alu_result), //output
  .alu_ram(alu_ram),
  .branch_taken(branch_taken),
  .branch_target(branch_target)
  //注意去掉逗号！！！！！！！！！！！！！！
);

//访存
  ysyx_25080201_LSU LSU_init(
    .clock(clock),
    .reset(reset),

    .io_lsu_rdata(io_lsu_rdata),//从MEM中读取数据->rdata_ram

    .valid(ls_vaild),         // 是否有访存请求
    .wen_ram(w_ram),          // 是否是写入
    .raddr_ram(alu_ram),      // 读地址lw,lbu????????
    .waddr_ram(alu_ram),      // 写地址?????
    .wdata_ram(rs2_data),     // 要写入的数据 from gpr
    .is_sb_type(is_sb_type),  // 写掩码
    .is_sh_type(is_sh_type),
    .is_lh_type(is_lh_type),
    .pc(pc),
    .rdata_ram(rdata_ram),     // out读取内存内容

    .io_lsu_respValid(io_lsu_respValid),//MEM返回给 CPU (LSU)      告诉 CPU：“我已经把你要的数据准备好了，现在你可以用 rdata/结果了！”
    .io_lsu_reqValid(io_lsu_reqValid),//发给存储器（MEM）    告诉存储器：“我现在真的有一个读/写请求了，请你处理！”
    .lsu_done(lsu_done), //访存完成标志

    .io_lsu_addr(io_lsu_addr), //读地址
    .io_lsu_wen(io_lsu_wen),
    .io_lsu_wdata(io_lsu_wdata),
    .io_lsu_wmask(io_lsu_wmask),//写掩码
    .io_lsu_size(io_lsu_size),

    .load_wait(load_wait),
    .LSU_WAIT(LSU_WAIT)
);



//写入寄存器，更新PC
  ysyx_25080201_WBU WBU_init (
  .clock(clock),
  .reset(reset),
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
  .is_branch(is_branch),
  .is_lh_type(is_lh_type),
  .is_lhu_type(is_lhu_type),
  .csr_write(csr_write),
  .csr_addr(imm[11:0]), // csr address
  .rs1_data(rs1_data), // csr write data
  .mcycle(mcycle),
  .mvendorid(mvendorid),
  .marchid(marchid),
  .reg_load_wait(reg_load_wait),
  .state_wait(state_wait),
  .load_wait(load_wait),
  .lsu_done(lsu_done),//访存完成标志

  .wb_wen(wb_wen),//  output  wb_wen = reg_en
  .wb_rd(wb_rd),
  .wb_Rresult(wb_Rresult),//写回到寄存器的数据来自ALU,RAM
  .next_pc(next_pc),
  .branch_taken(branch_taken),
  .branch_target(branch_target),
  .wbu_mcycle(wbu_mcycle)
  //注意去掉逗号！！！！！！！！！！！！！！
);

  ysyx_25080201_RegisterFile RegisterFile_init(
  .clock(clock),
  .pc(pc),
  .rs1(rs1),
  .rs2(rs2),
  .reg_wdata(wb_Rresult),     // 写入ALU,RAM数据
  .reg_waddr(wb_rd),          // 写入地址rd
  .wen(wb_wen),               // 写使能
  .io_ifu_rdata(io_ifu_rdata),
  .lsu_done(lsu_done),
  
  .rs1_data(rs1_data),
  .rs2_data(rs2_data)
  //注意去掉逗号！！！！！！！！！！！！！！
);

  
endmodule


module ysyx_25080201_RegisterFile(
  input clock,
  input [31:0]pc,
  input [3:0]rs1,
  input [3:0]rs2,
  input [31:0] reg_wdata,     // 接收要写入的alu|ram数据
  input [3:0] reg_waddr,      // 接收要写入的地址rd
  input wen,                  // 写使能
  input [31:0]io_ifu_rdata,
  input lsu_done,

  output  [31:0] rs1_data,
  output  [31:0] rs2_data
);
  // rf为寄存器数组，大小为16，每个寄存器宽度为32
  reg[31:0] rf[15:0];
  // 写操作：时钟上升沿，当wen为1时，将wdata写入waddr对应的寄存器
  always @(posedge clock) begin
    //$display("reg_wdata=0x%08x",reg_wdata);
    //$display("reg_waddr=%05b",reg_waddr);
    if (lsu_done || (wen && reg_waddr != 0)) begin
      rf[reg_waddr] <= reg_wdata; // 0号寄存器保护  
      //$display("Write Reg: pc=%08x, x[%0d]=%08x", pc, reg_waddr, reg_wdata);
      //$display("a0=0x%08x",rf[10]);
    end

  end

  assign rs1_data = (rs1 == 0) ? 32'b0 : rf[rs1];
  assign rs2_data = (rs2 == 0) ? 32'b0 : rf[rs2];//如果 rs2 没有用到或者等于 0 号寄存器，输出就是 0
  always @(*) begin
    if(io_ifu_rdata == 32'h00100073 && rf[10] == 32'b0)begin
      halt(pc,0);
    end   
    else if(io_ifu_rdata == 32'h00100073 && rf[10] == 32'b1)begin
      //$display("a0=0x%08x",rf[10]);
      halt(pc,1);
    end
  end


endmodule


