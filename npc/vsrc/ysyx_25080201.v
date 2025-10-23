`ifdef VERILATOR
import "DPI-C" function void halt(input int pc,input int halt_ret);
`endif
module ysyx_25080201 (
    input  clock,
    input  reset,
    output reg [31:0] io_ifu_addr,
    output reg        io_ifu_reqValid,
    input  [31:0] io_ifu_rdata,
    input         io_ifu_respValid,

    output  [31:0] io_lsu_addr,
    output reg        io_lsu_reqValid,
    input [31:0] io_lsu_rdata,
    input         io_lsu_respValid,
    output  [1:0]  io_lsu_size,
    output reg        io_lsu_wen,
    output reg [31:0] io_lsu_wdata,
    output reg [3:0]  io_lsu_wmask
); 


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
  wire is_lw_type;
  wire is_sb_type;
  wire is_sh_type;
  wire is_branch;
  wire is_lh_type;
  wire is_lhu_type;


  wire load_wait; // load指令等待信号
  //wire state_wait;
  //wire reg_load_wait; //load指令next等待信号WBU->RegFile

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

  reg [31:0] pc_reg;       //保存当前 PC值,实现顺序执行和跳转
  reg [31:0] pc;
  always @(*) begin
    pc = pc_reg;      //pc_reg的“输出端口”
  end
  //assign pc[31:0] = pc_reg;      //pc_reg的“输出端口”

  //================LSU======================
  reg        lsu_done; //访存完成标志
  wire        LSU_WAIT;
  //================LSU======================

//================CSR======================
  reg [63:0] mcycle;//cycle计数器
  reg [63:0] wbu_mcycle;//wbu cycle mcycle需要用一个中间变量暂存,然后在顶层模块赋值
  reg [31:0] mvendorid = 32'h79737978;//ysyx 32'h79737978
  reg [31:0] marchid = 32'h25080201;//STUDENT_ID 32'h017EB189 32'd2025080201
  reg [63:0] alu_csr; // csr读出数据
//================CSR======================
  
  always @(posedge clock) begin//or posedge reset
    
    if (reset)begin
      //pc <= 32'h30000000;
      pc_reg <= 32'h30000000;
      mcycle <= 0;
    end else if(csr_write && io_ifu_respValid && !ls_vaild) begin
      mcycle <= wbu_mcycle;
      pc_reg <= next_pc; 
      // 只有在没有load等待、且指令有效时才允许更新PC
    end else if(io_ifu_respValid && !(ls_vaild)) begin //load指令外的普通指令PC更新,防止译码延迟
    //同时也能预防译码的延迟，因为此时load相关的信号为0
      pc_reg <= next_pc;
      mcycle <= mcycle + 1;
    end else if (lsu_done) begin//load,store指令的PC更新 防止读取内存延迟 
    //即使译码延迟也没关系，因为两周期的指令，在第二个周期译码和lsu_done同时为1
        pc_reg <= next_pc;
        mcycle <= mcycle + 1;
    end
    
  end


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
    .ls_vaild(ls_vaild),


    .io_ifu_addr(io_ifu_addr),//请求读存储器地址 pc
    .inst_out(inst_out),//输出指令
    //.inst_valid(inst_valid),
    .io_ifu_reqValid(io_ifu_reqValid)
    //.state_wait(state_wait)
  );
  //译码
  ysyx_25080201_IDU IDU_init(
  .inst_ym(inst_out),//inst_out
  //.pc(pc),
  //.io_ifu_respValid(io_ifu_respValid),
  //.clock(clock),

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
  .is_lw_type(is_lw_type),    //字区分
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
  .branch_target(branch_target),
  .alu_csr(alu_csr)
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
    //.is_lh_type(is_lh_type),
    //.pc(pc),
    .rdata_ram(rdata_ram),     // out读取内存内容

    .io_lsu_respValid(io_lsu_respValid),//MEM返回给 CPU (LSU)      告诉 CPU：“我已经把你要的数据准备好了，现在你可以用 rdata/结果了！”
    .io_lsu_reqValid(io_lsu_reqValid),//发给存储器（MEM）    告诉存储器：“我现在真的有一个读/写请求了，请你处理！”
    //.io_ifu_respValid(io_ifu_respValid),
    .lsu_done(lsu_done), //访存完成标志
    .io_ifu_respValid(io_ifu_respValid),

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
  // .clock(clock),
  // .reset(reset),
  .pc(pc_reg),
  .alu_data(alu_result),//从alu中读取数据
  .alu_addr(alu_ram[1:0]),// 读地址lw,lbu--------------------alu_ram[31:0]
  .ram_data(rdata_ram),//从ram中读取数据
  .waddr(rd),          //往rd中写入
  .reg_en(Reg_write),
  .Jal_en(Jal_en),
  .jalr_en(Jump_en),
  //.is_load_type(is_load_type),
  .is_lbu_type(is_lbu_type),
  .is_lw_type(is_lw_type),
  .is_branch(is_branch),
  .is_lh_type(is_lh_type),
  .is_lhu_type(is_lhu_type),
  .csr_write(csr_write),
  .csr_addr(imm[11:0]), // csr address
  .rs1_data(rs1_data), // csr write data
  .mcycle(mcycle),
  .mvendorid(mvendorid),
  .marchid(marchid),
  //.reg_load_wait(reg_load_wait),
  //.state_wait(state_wait),
  .load_wait(load_wait),
  //.lsu_done(lsu_done),//访存完成标志
  .alu_csr(alu_csr),  // csr读出数据
  .func(func),

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
  .wen(wb_wen && ((is_load_type && io_lsu_respValid) || (!is_load_type && io_ifu_respValid))),               // 写使能
  .io_ifu_rdata(io_ifu_rdata),
  .lsu_done(lsu_done),
  
  .rs1_data(rs1_data),
  .rs2_data(rs2_data)
  //注意去掉逗号！！！！！！！！！！！！！！
);

  
endmodule


//===============================  IFU  ===========================
module ysyx_25080201_IFU (
  input clock,
  input reset,
  input [31:0] pc,
  input [31:0] io_ifu_rdata,//LSU->
  input load_wait,
  input io_ifu_respValid,//MEM->IFU
  input LSU_WAIT,
  input lsu_done,
  input ls_vaild,

  output reg [31:0] io_ifu_addr,//->raddr temp from pc
  output reg [31:0] inst_out,//->IDU temp from rdata
  //output reg inst_valid,//只是说明指令取出的那个周期
  output reg io_ifu_reqValid//IFU->MEM
  //output state_wait
  //此处直接用了MEM输出的数据，没有传给IFU，因为clock会延迟一拍
  //地址同理，直接把PC输入给MEM
);
  // typedef enum reg [0:0] {IDLE=1'b0, WAIT=1'b1} state_t;
  // state_t state;
  localparam IDLE = 1'b0;
  localparam WAIT = 1'b1;

  reg state;
  // assign io_ifu_addr = pc;
  always @(*) begin
    io_ifu_addr = pc;
    inst_out = io_ifu_rdata;
  end
  //assign inst_out = io_ifu_rdata;
  //assign state_wait = (state == WAIT);

  always @(posedge clock) begin//
    if (reset) begin
      state <= IDLE;
      //inst_valid <= 0;
      io_ifu_reqValid <= 0;
    end else begin
      case (state)
        IDLE: begin
            // idle_hold生效，进入WAIT  第一周期不译码
            io_ifu_reqValid <= 1;
            state <= WAIT;
            //inst_valid <= 1;//第二周期译码

        end
        WAIT: begin
          io_ifu_reqValid <= 0;//防止连续请求
          if (io_ifu_respValid) begin
              //inst_valid <= 1;
              if (!ls_vaild) begin//此时为第二周期 (load_wait || LSU_WAIT)防止延迟 but time logger
                state <= IDLE;
                //inst_valid <= 0;//next 第一周期不译码
            end else begin// 只有 lsu_done==1 才能转IDLE
                state <= WAIT;
                //inst_valid <= 1;//第三周期不译码
            end
          end else if(!io_ifu_respValid && (load_wait || LSU_WAIT)) begin// 只有 lsu_done==1 才能转IDLE
                if (lsu_done) begin//防止延迟
                  state <= IDLE;
                  //inst_valid <= 0;//next 第一周期不译码 
                end
            end else begin
              //inst_valid <= 1;//等待时有效
            end

        end
      endcase
    end
  end
endmodule
//===============================  IFU  ===========================


//===============================  IDU  ===========================
//译码
module ysyx_25080201_IDU (
  input [31:0]inst_ym,
  //input [31:0]pc,
  //input io_ifu_respValid,//指令是否有效
  //input clock,

  output reg [31:0]IDU_imm,
  output reg [3:0]IDU_rd,
  output reg [3:0]IDU_rs1,
  output reg [3:0]IDU_rs2,
  output reg [2:0]IDU_func,
  output reg [6:0]IDU_func7,
  output reg [6:0]IDU_opcode,
  output reg Reg_write,         // 是否写回寄存器
  output reg Jal_en,
  output reg Jump_en,
  output reg add_alu,
  output reg ls_vaild,          // 是否为访存指令（load/store）
  output reg w_ram,             // 是否访问RAM（load/store写使能）
  output reg is_load_type,      // 是否为load类型指令（用于WBU选择写回数据）,区分alu_result和alu_RAM
  output reg is_lbu_type,        //字节区分
  output reg is_lw_type,        //字区分
  output reg is_sb_type,
  output reg is_sh_type,
  output reg is_branch,
  output reg is_lh_type,
  output reg is_lhu_type,
  output reg csr_write         // 是否为CSR寄存器写入
  //注意去掉逗号！！！！！！！！！！！！！！
);

  always @(*) begin
    // 默认初始化所有信号，防止锁存器
    IDU_imm      = 32'b0;
    IDU_rd       = 4'b0;
    IDU_rs1      = 4'b0;
    IDU_rs2      = 4'b0;
    IDU_func     = 3'b0;
    IDU_opcode   = 7'b0;
    IDU_func7    = 7'b0;
    Reg_write    = 0;
    Jal_en       = 0;
    Jump_en      = 0;
    add_alu      = 1;
    ls_vaild     = 0;
    w_ram        = 0;
    is_load_type = 0;
    is_lbu_type  = 0;
    is_lw_type   = 0;
    is_sb_type   = 0;
    is_sh_type   = 0;
    is_branch    = 0;
    is_lh_type   = 0;
    is_lhu_type  = 0;
    csr_write    = 0;

      // 指令类型译码与控制信号生成
      case (inst_ym[6:0])
        // I-type: addi, lw, lbu, jalr, sltiu ,srai,andi,xori,srli
        7'b0010011, 7'b0000011, 7'b1100111: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = inst_ym[14:12];
          IDU_rs1    = inst_ym[18:15];
          IDU_rd     = inst_ym[10:7];
          IDU_imm    = { {20{inst_ym[31]}}, inst_ym[31:20] };

          if (inst_ym[6:0] == 7'b0010011) begin // addi
            if (inst_ym[14:12] == 3'b000) begin
              Reg_write = 1;
              add_alu   = 1;
              //$display("addi");
            end
            else if (inst_ym[14:12] == 3'b111) begin//andi
              Reg_write = 1;
              add_alu   = 1;
              //$display("andi");
            end
            else if(inst_ym[14:12] == 3'b011)begin //sltiu
              Reg_write = 1;
              //$display("sltiu");
            end
            else if(inst_ym[14:12] == 3'b101)begin//srai
              Reg_write = 1;
              IDU_func7    =  inst_ym[31:25];
              //$display("srai");
            end
            else if(inst_ym[14:12] == 3'b100)begin//xori
              Reg_write = 1;
            end
            else if(inst_ym[14:12] == 3'b101)begin//srli
              Reg_write = 1;
              IDU_func7    =  inst_ym[31:25];
            end
            else if(inst_ym[14:12] == 3'b001)begin//slli
              Reg_write = 1;
              IDU_func7    =  inst_ym[31:25];
            end
          end

          else if (inst_ym[6:0] == 7'b1100111) begin // jalr
            if (inst_ym[14:12] == 3'b000) begin
              Reg_write = 1;
              Jump_en   = 1;
              //$display("jalr");
            end
          end

          else if (inst_ym[6:0] == 7'b0000011) begin // load
            Reg_write    = 1;
            is_load_type = 1;
            if (inst_ym[14:12] == 3'b010) begin // lw
              ls_vaild = 1;
              w_ram    = 0;
              is_lw_type = 1;
              //$display("lw");
            end else if (inst_ym[14:12] == 3'b100) begin // lbu
              ls_vaild    = 1;
              w_ram       = 0;
              is_lbu_type = 1;
              //$display("lbu");
            end else if (inst_ym[14:12] == 3'b001) begin // lh
              ls_vaild    = 1;
              w_ram       = 0;
              is_lh_type  = 1;
            end else if (inst_ym[14:12] == 3'b101) begin // lhu
              ls_vaild    = 1;
              w_ram       = 0;
              is_lhu_type  = 1;
            end

          end
        end

        // R-type: add,sub,sll
        7'b0110011: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = inst_ym[14:12];
          IDU_rs1    = inst_ym[18:15];
          IDU_rs2    = inst_ym[23:20];
          IDU_rd     = inst_ym[10:7];
          IDU_imm    = 32'b0;
          IDU_func7  = inst_ym[31:25];

          if (inst_ym[14:12] == 3'b000) begin // func3
            if(IDU_func7 == 7'b0000000) begin // func7 add
              Reg_write = 1;
              add_alu   = 0;
              //$display("add");
            end
            else if (IDU_func7 == 7'b0100000) begin// func7 sub
              Reg_write = 1;
              add_alu   = 0;
              //$display("sub");
            end
          end
          else if(IDU_func == 3'b001)begin//sll
            Reg_write = 1;
            add_alu   = 0;
          end
          else if(IDU_func == 3'b111)begin//and
            Reg_write = 1;
            add_alu   = 0;
          end
          else if(IDU_func == 3'b011)begin//sltu 7 0
            Reg_write = 1;
            add_alu   = 0;
          end
          else if(IDU_func == 3'b110)begin//or 7 0
            Reg_write = 1;
            add_alu   = 0;
          end
          else if(IDU_func == 3'b100)begin//xor 7 0
            Reg_write = 1;
            add_alu   = 0;
          end
          else if(IDU_func == 3'b010)begin//slt 7 0
            Reg_write = 1;
            add_alu   = 0;
          end
          else if(IDU_func == 3'b101)begin//sra 7 0100000  srl
            Reg_write = 1;
            add_alu   = 0;
          end
        end


        // U-type: lui
        7'b0110111: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = 3'b0;
          IDU_rs1    = 4'b0;
          IDU_rs2    = 4'b0;
          IDU_rd     = inst_ym[10:7];
          IDU_imm    = { inst_ym[31:12], 12'b0 }; // U-type立即数左移12位
          Reg_write  = 1;
          //$display("lui");
        end

        // U-type: auipc
        7'b0010111: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = 3'b0;
          IDU_rs1    = 4'b0;
          IDU_rs2    = 4'b0;
          IDU_rd     = inst_ym[10:7];
          IDU_imm    = { inst_ym[31:12], 12'b0 }; // U-type立即数左移12位
          Reg_write  = 1;
          //$display("auipc");
        end

        // B-type: beq(beqz),bne,bgeu  这里我没用区分bne和beq
        7'b1100011: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = inst_ym[14:12];
          IDU_rs1    = inst_ym[18:15];
          IDU_rs2    = inst_ym[23:20];
          IDU_rd     = 4'b0;
          IDU_imm    = { {20{inst_ym[31]}},inst_ym[7], inst_ym[30:25], inst_ym[11:8],1'b0}; // B-type offset

          add_alu   = 0;
          is_branch = 1;
          //$display("beq");
        end

        // J-type: jal
        7'b1101111: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = 3'b0;
          IDU_rs1    = 4'b0;
          IDU_rs2    = 4'b0;
          IDU_rd     = inst_ym[10:7];
          IDU_imm    = { {12{inst_ym[31]}}, inst_ym[19:12], inst_ym[20] , inst_ym[30:21],1'b0}; // J-type offset
          Reg_write  = 1;
          Jal_en   = 1;
          //$display("jal");
        end

        // S-type: sw, sb
        7'b0100011: begin
          IDU_opcode = inst_ym[6:0];
          IDU_func   = inst_ym[14:12];
          IDU_rs1    = inst_ym[18:15];
          IDU_rs2    = inst_ym[23:20];
          IDU_rd     = 4'b0;
          IDU_imm    = { {20{inst_ym[31]}}, inst_ym[31:25], inst_ym[11:7] };
          Reg_write  = 0;
          if (inst_ym[14:12] == 3'b010) begin // sw
            ls_vaild = 1;
            w_ram    = 1;
            //$display("sw");
          end else if (inst_ym[14:12] == 3'b000) begin // sb
            ls_vaild    = 1;
            w_ram       = 1;
            is_sb_type  = 1;
            //$display("sb");
          end else if (inst_ym[14:12] == 3'b001) begin // sh
            ls_vaild    = 1;
            w_ram       = 1;
            is_sh_type  = 1;
            //$display("sh");
          end

        end

        7'b1110011: begin//特权指令csrrw
          if(inst_ym[14:12] == 3'b001)begin
            IDU_opcode = inst_ym[6:0];
            IDU_func   = inst_ym[14:12];
            IDU_rs1    = inst_ym[18:15];
            IDU_rd     = inst_ym[10:7];
            IDU_imm    = { {20{inst_ym[31]}}, inst_ym[31:20] };//csr地址
            Reg_write  = 1;
            csr_write  = 1;
          end else if(inst_ym[14:12] == 3'b010)begin//csrrs
            IDU_opcode = inst_ym[6:0];
            IDU_func   = inst_ym[14:12];
            IDU_rs1    = inst_ym[18:15];
            IDU_rd     = inst_ym[10:7];
            IDU_imm    = { {20{inst_ym[31]}}, inst_ym[31:20] };//csr地址
            Reg_write  = 1;
            csr_write  = 1;
          end
            //$display("csrrw");
          
        
          else begin
          
            //halt(pc,0);
            Reg_write  = 0;
            csr_write  = 0;
          end
        end

        default: begin
          //$display("2.Unknown/illegal io_ifu_respValid: %d instruction: %08x at pc=%08x", io_ifu_respValid,inst_ym, pc);
              IDU_imm      = 32'b0;
              IDU_rd       = 4'b0;
              IDU_rs1      = 4'b0;
              IDU_rs2      = 4'b0;
              IDU_func     = 3'b0;
              IDU_opcode   = 7'b0;
              IDU_func7    = 7'b0;
              Reg_write    = 0;
              Jal_en       = 0;
              Jump_en      = 0;
              add_alu      = 1;
              ls_vaild     = 0;
              w_ram        = 0;
              is_load_type = 0;
              is_lbu_type  = 0;
              is_lw_type   = 0;
              is_sb_type   = 0;
              is_sh_type   = 0;
              is_branch    = 0;
              is_lh_type   = 0;
              is_lhu_type  = 0;
              csr_write    = 0;
          //halt(pc,1);
        end
      endcase
    
    //$display("opcode_alu = %07b,IDU_imm=%12b,func_alu=%03b,IDU_rs1=%08x,IDU_rs2=%08x,IDU_rd=%08x | pc = %08x,Reg_write=%d",IDU_opcode,IDU_imm,IDU_func,IDU_rs1,IDU_rs2,IDU_rd,pc,Reg_write);
    // 特权指令halt
    end
  //end

endmodule

//===============================  IDU  ===========================


//===============================  EXU  ===========================
//ALU算术逻辑单元
module ysyx_25080201_EXU (
  input [31:0]pc,
  input [31:0]imm_alu,        // 立即数（I型/S型/U型等）或无效
  input [31:0]rs1_alu,
  input [31:0]rs2_alu,        // R型时有效，其它时无效
  input [2:0]func_alu,
  input [6:0]func7_alu,
  input [6:0]opcode_alu,
  input alu_src,              // 1: imm_alu, 0: rs2_alu
  input is_branch,
  input [63:0]mcycle,           // cycle计数器
  input csr_write,

  output reg [31:0]alu_result,// 寄存器写回
  output reg [31:0]alu_ram,    // 访存地址
  output reg branch_taken,
  output reg [31:0]branch_target,
  output reg [63:0]alu_csr   // csr读出数据
  //注意去掉逗号！！！！！！！！！！！！！！
);
  // 选择ALU第二操作数
  wire  [31:0]in2_alu = alu_src ? imm_alu : rs2_alu;
  always @(*) begin
    alu_result = 32'b0;
    alu_ram = 32'b0;
    branch_taken = 1'b0;         
    branch_target = 32'b0;
    alu_csr = 64'b0;     
    //$display("opcode_alu = %07b,func_alu=%03b,alu_result=%08x,rs1_alu=%08x,in2_alu=%08x| pc = %08x\n",opcode_alu,func_alu,alu_result,rs1_alu,in2_alu,pc);
    case (opcode_alu)
    
      7'b0010011: begin // I型算术（如 addi sltiu andi）
        if (func_alu == 3'b000)
          alu_result = rs1_alu + in2_alu;//addi
        else if(func_alu == 3'b011)
          alu_result = (rs1_alu < in2_alu) ? 32'd1 : 32'd0;//sltiu
        else if(func_alu == 3'b101 && func7_alu == 7'b0100000)
          alu_result = rs1_alu >>> imm_alu[4:0];// srai
          //$display("opcode_alu = %07b,func_alu=%03b,alu_result=%08x,rs1_alu=%08x,imm_alu[4:0]=%05b| pc = %08x\n",opcode_alu,func_alu,alu_result,rs1_alu,imm_alu[4:0],pc);
        else if(func_alu == 3'b111)
          alu_result = rs1_alu & in2_alu; //andi
        else if(func_alu == 3'b100)
          alu_result = rs1_alu ^ imm_alu;//xori
        else if(func_alu == 3'b101 && func7_alu == 7'b0000000)
          alu_result = rs1_alu >> imm_alu[4:0];//srli
        else if(func_alu == 3'b001 && func7_alu == 7'b0000000)
          alu_result = rs1_alu << imm_alu[4:0];//slli
        else begin
          alu_result = 32'b0;
        end
      end

      7'b0110011: begin // R型算术（如 add sub sll）
      case (func_alu)
        3'b000: begin
          if (func7_alu == 7'b0000000)
            alu_result = rs1_alu + in2_alu;// add
          else if(func7_alu == 7'b0100000)
            alu_result = rs1_alu - in2_alu;// sub
          //$display("alu_result(%08x) = rs1_alu(%08x) + in2_alu(%08x) | pc = %08x",alu_result,rs1_alu,in2_alu,pc);
        end
        3'b001:begin
          if(func7_alu == 7'b0000000)
            alu_result = rs1_alu << in2_alu[4:0];//sll
            //$display("opcode_alu = %07b,func_alu=%03b,alu_result=%08x,rs1_alu=%08x,in2_alu[4:0]=%05b| pc = %08x\n",opcode_alu,func_alu,alu_result,rs1_alu,in2_alu[4:0],pc);
        end
        3'b111: alu_result = rs1_alu & in2_alu;//and
        3'b011: alu_result = ( rs1_alu < in2_alu) ? 1 : 0;//sltu
        3'b110: alu_result = rs1_alu | in2_alu ;//or
        3'b100: alu_result = rs1_alu ^ in2_alu ;//xor
        3'b010: alu_result = ( rs1_alu < in2_alu ) ? 1 : 0;//slt
        3'b101: begin
          if(func7_alu == 7'b0100000) alu_result = ( rs1_alu >>> in2_alu );//sra
          else if(func7_alu == 7'b0000000) alu_result = ( rs1_alu >> in2_alu[4:0] );//srl
        end
        
        
        default: alu_result = 32'b0;
      endcase
      end

      7'b1101111: begin // jal
        alu_result = pc + in2_alu; //rs1 + signed-offset
        //$display("alu_result(%08x) = rs1_alu(%08x) + in2_alu(%08x)\n",alu_result,pc,in2_alu);
      end

      7'b1100111: begin // jalr
        alu_result = rs1_alu + in2_alu;
      end

      7'b0000011: begin // load（如 lw,lbu,lh）
        if(func_alu == 3'b010 || func_alu == 3'b100 || func_alu == 3'b001 || func_alu == 3'b101 )begin
          alu_ram = rs1_alu + in2_alu;
        end
      end

      7'b0100011: begin // store（如 sb,sw,sh）
        if(func_alu == 3'b010 || func_alu == 3'b000 || func_alu == 3'b001)begin
          alu_ram = rs1_alu + in2_alu;
        end
      end

      7'b0110111: begin // lui
        alu_result = in2_alu;
      end

      7'b0010111:begin // auipc
        alu_result = pc + in2_alu;
      end

      7'b1100011:begin // 分支指令 B-type
        if (is_branch) begin
          case (func_alu)
            3'b000: branch_taken = (rs1_alu == in2_alu);// beq
            3'b001: branch_taken = (rs1_alu != in2_alu);//bne
            3'b111: branch_taken = (rs1_alu >= in2_alu );//bgeu
            3'b110: branch_taken = (rs1_alu < in2_alu );//bltu
            3'b101: branch_taken = (rs1_alu >= in2_alu );//bge
            3'b100: branch_taken = (rs1_alu < in2_alu );//blt
            default: ;
          endcase
          branch_target = pc + imm_alu;
        end
        else begin
          branch_taken = 1'b0;//??????????????
        end
      end

      7'b1110011: begin // csrrw
        if(csr_write && func_alu == 3'b001)begin
          case (imm_alu[11:0])
            12'hB00: alu_result = mcycle[31:0]; // mcycle
            12'hB80: alu_result = mcycle[63:32]; // mcycleh
            12'hF11, 12'hF12: ;  // 只读 CSR，不允许写
            default: ;
          endcase
        end else if (csr_write && func_alu == 3'b010 )begin//csrrs
          alu_csr = mcycle;
          case (imm_alu[11:0])
            12'hF11, 12'hF12: ; // 只读 CSR
            12'hB00: begin
              alu_result = mcycle[31:0];
              if(rs1_alu != 0)begin
                alu_csr = {mcycle[63:32],mcycle[31:0] | rs1_alu }; // mcycle
              end 
              else begin
                alu_csr = mcycle;//?????????????????????????????????????
              end
            end
            12'hB80: begin
              alu_result = mcycle[63:32];
              if(rs1_alu != 0)begin
                alu_csr = {mcycle[63:32] | rs1_alu , mcycle[31:0]}; // mcycleh
              end
              else begin
                alu_csr = mcycle;//?????????????????????????????????????
              end
            end
            default: alu_result = 32'b0;
          endcase
        end
        else 
          alu_result = 32'b0;
      end

      default: alu_result = 32'b0;
    endcase
    end
endmodule

//===============================  EXU  ===========================




//===============================  LSU  ===========================
module ysyx_25080201_LSU (
    input clock,
    input reset,
    input  [31:0] io_lsu_rdata,
    //=============================原
    input valid,              // 是否有访存请求  ym
    input wen_ram,                // 是否是写入 ym Write_enable
    input [31:0]raddr_ram,        // 读地址EXU io_lsu_addr
    input [31:0]waddr_ram,        // 写地址EXU io_lsu_addr
    input [31:0]wdata_ram,        //  要写入的数据gpr 
    //=============================原
    input is_sb_type,
    input is_sh_type,
    //input is_lh_type,
    //input [31:0]pc,
    input io_ifu_respValid,

    input reg io_lsu_respValid,//存储器响应有效（数据已准备好）
    output reg io_lsu_reqValid,//访存请求有效 发给存储器（MEM）
    output reg lsu_done, //访存完成标志 （通知下游模块）

    output reg [31:0]rdata_ram, // 读出的数据 (返回给EXU/WBU) load 
    output [31:0] io_lsu_addr,
    output reg       io_lsu_wen,//=============================================
    output reg [31:0] io_lsu_wdata,// 写入存储器的数据 组合逻辑
    output reg [ 3:0] io_lsu_wmask,
    output [ 1:0] io_lsu_size,

    output reg load_wait, // load指令等待信号
    output reg LSU_WAIT
);
    
    localparam IDLE = 1'b0;// 空闲状态：等待访存请求（上游发来 valid=1）
    localparam WAIT = 1'b1;// 等待状态：等待数据返回（respValid=1），只有访存请求发出后才进入
    reg state;
    always @(*) begin
      LSU_WAIT = (state == WAIT);
    end
    //assign LSU_WAIT = (state == WAIT);

    reg [3:0]wmask;
    reg [31:0] data_ram;

    //assign rdata_ram = io_lsu_respValid ? io_lsu_rdata : 32'b0;
    // 组合逻辑计算访存地址和写使能
    assign io_lsu_addr  = wen_ram ? waddr_ram : raddr_ram;
    //assign io_lsu_wen   = wen_ram;//
    //assign io_lsu_wdata = data_ram;//
    //assign io_lsu_wmask = wmask;//
    assign io_lsu_size  = is_sb_type ? 2'b00 : 
                         is_sh_type ? 2'b01 : 
                         2'b10;
    //assign io_lsu_reqValid = valid;
  always @(*) begin
    io_lsu_wdata = data_ram;
    wmask = io_lsu_wen ? (
                  is_sb_type ? (4'b0001 << waddr_ram[1:0]) : 
                  is_sh_type ? (waddr_ram[1] ? 4'b1100 : 4'b0011) : 
                  4'b1111
               ) : 4'b0000;
    io_lsu_wmask = wmask;
  end
// assign wmask = io_lsu_wen ? (
//                   is_sb_type ? (4'b0001 << waddr_ram[1:0]) : 
//                   is_sh_type ? (waddr_ram[1] ? 4'b1100 : 4'b0011) : 
//                   4'b1111
//                ) : 4'b0000;

    
    // 组合逻辑计算写入数据
    always @(*) begin
        data_ram = 32'b0; 
        if(is_sb_type)begin//sb
            case (waddr_ram[1:0])
                2'b00: data_ram = {24'b0,wdata_ram[7:0]};
                2'b01: data_ram = {16'b0,wdata_ram[7:0],8'b0};
                2'b10: data_ram = {8'b0,wdata_ram[7:0],16'b0};
                2'b11: data_ram = {wdata_ram[7:0],24'b0};
                default: data_ram =32'b0;
            endcase
        end else if (is_sh_type) begin//sh
            case (waddr_ram[1:0])
                2'b00: data_ram = {16'b0, wdata_ram[15:0]};
                2'b10: data_ram = {wdata_ram[15:0], 16'b0};
                default: data_ram = 32'b0;
            endcase

        end else begin
            data_ram = wdata_ram;
        end
    end
    // 状态转移
    always @(posedge clock) begin
        if(reset) begin
            state <= IDLE;
            io_lsu_reqValid <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    // 读请求进入WAIT，写请求保持IDLE（写单周期完成）
                    if (valid && io_ifu_respValid) begin//// 读请求
                        state <= WAIT;
                        io_lsu_reqValid <= 1'b1; // 发起访存请求
                    end else begin
                        state <= IDLE;
                        io_lsu_reqValid <= 1'b0; // 无访存请求
                    end
                end
                WAIT: begin
                    // 等待存储器respValid拉高时回到IDLE
                    if (io_lsu_respValid) begin// 存储器响应
                        state <= IDLE;
                    end else begin
                        state <= WAIT;
                        io_lsu_reqValid <= 1'b0; // 访存请求已发出
                    end
                end
            endcase
        end
    end

    // /输出逻辑 : LSU输出信号控制 
    always @(*) begin
        rdata_ram = 32'b0;
        lsu_done = 1'b0;
        load_wait = 1'b0;
        // io_lsu_wen = 1'b0;
        io_lsu_wen = io_lsu_reqValid && wen_ram;//组合逻辑?
        case (state)
            IDLE: begin
                if (valid) begin
                    // /io_lsu_reqValid = 1'b1;                // 发起访存请求
                    load_wait = !wen_ram;               // 读时进入等待
                    lsu_done = io_lsu_respValid ? 1'b1 : 1'b0;   // 写操作立即完成
                end
                //===================================================================================================
                else begin
                    //io_lsu_reqValid = 1'b0;                // 无访存请求
                    load_wait = 1'b0;
                    rdata_ram = 32'b0;
                    lsu_done = 1'b0;
                end
            end
            WAIT: begin
                //io_lsu_reqValid = 1'b0;                // 访存请求已发出
                if (io_lsu_respValid) begin
                    load_wait = 1'b0;                     // 读操作等待数据 
                    rdata_ram = io_lsu_rdata;            // 数据返回，采集结果 load
                    lsu_done = 1'b1;                   // 访存完成
                // end else if (valid && wen_ram) begin
                //     io_lsu_wen = 1'b1;                  // 写操作
                    
                end else begin
                    load_wait = 1'b1;                   //  [延迟]读操作等待数据 load
                    rdata_ram = 32'b0;
                    lsu_done = 1'b0;
                end
                
            end
        endcase
    end 

endmodule

//===============================  LSU  ===========================



//===============================  WBU  ===========================

//写入寄存器，更新PC
module ysyx_25080201_WBU (
  // input clock,
  // input reset,
  input [31:0]pc,
  input [31:0]alu_data,//从alu中读取数据
  input [1:0]alu_addr,//从alu中读取的地址 [31:0]alu_addr,
  input [31:0]ram_data,//从ram中读取数据
  input [3:0]waddr,
  input reg_en,
  input Jal_en,
  input jalr_en,
  //input is_load_type,
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
  //input reg_load_wait,
  //input state_wait,
  input load_wait,
  //input lsu_done,//访存完成标志
  input is_lw_type,
  input branch_taken,
  input [31:0]branch_target,
  input [63:0] alu_csr,   // csr读出数据
  input [2:0] func,

  output wb_wen,
  output [3:0]wb_rd,
  output [31:0]wb_Rresult,//写回到寄存器的数据来自ALU,RAM
  output reg [31:0]next_pc,
  //output reg branch_taken,
  //output reg [31:0]branch_target,
  output reg [63:0] wbu_mcycle // cycle计数器
  //注意去掉逗号！！！！！！！！！！！！！！
);
  wire [7:0] lbu_byte;
  wire [15:0] lh_byte;
  reg [31:0] wb_Rresult_reg;
  //reg wen_load ;
  //reg [3:0] rd_load;
  //reg [31:0] Result_load;
  //reg [31:0] addr_load;
  //reg lbu_wait;
  //reg lw_wait;//is_load_type的延迟信号

  //选择器
  assign lbu_byte =
    (alu_addr == 2'b00) ? ram_data[7:0] ://////alu_data=0???????
    (alu_addr == 2'b01) ? ram_data[15:8] :
    (alu_addr == 2'b10) ? ram_data[23:16] :
                               ram_data[31:24];

  assign lh_byte =  
    (alu_addr == 2'b00) ? ram_data[15:0] : 
    (alu_addr == 2'b10) ? ram_data[31:16] : 16'b0;

  always @(*) begin
    wb_Rresult_reg = 32'b0;
    wbu_mcycle     = mcycle;   // 默认不修改 CSR 值
    if(is_lbu_type)
      wb_Rresult_reg = {24'b0, lbu_byte} ; //lbu
    else if(is_lh_type)
      wb_Rresult_reg = {{16{lh_byte[15]}}, lh_byte} ; //lh  符号扩展
    else if(is_lhu_type)
      wb_Rresult_reg = {16'b0, lh_byte} ;//lhu 零扩展

    else if  (jalr_en || Jal_en)
      wb_Rresult_reg = pc + 32'h4;//jalr jal

    else if(is_lw_type)
      wb_Rresult_reg = ram_data;//lw

    else if(csr_write)
      case(csr_addr)
        12'hB00: begin
          wb_Rresult_reg = alu_data; // mcycle 读出当前CSR值到rd
          if(func == 3'b010 && rs1_data != 0)
              wbu_mcycle = alu_csr;
          else
              wbu_mcycle = {mcycle[63:32], rs1_data}; // 低32位写入rs1_data
        end

        12'hB80: begin
          wb_Rresult_reg = alu_data; // mcycleh 读出当前CSR值到rd
          if(func == 3'b010 && rs1_data != 0)
              wbu_mcycle = alu_csr;
          else
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

  assign wb_Rresult =  wb_Rresult_reg;//区分load指令和普通指令
  assign wb_wen = load_wait ? 0 : reg_en;//区分load指令和普通指令 wb_wen && ((is_load_type && io_lsu_respValid) || (!is_load_type && io_ifu_respValid))
  assign wb_rd =  waddr;//区分load指令和普通指令
endmodule

//===============================  WBU  ===========================


//===============================  RegisterFile  ===========================
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
    if (lsu_done || (wen && reg_waddr != 0)) begin
      rf[reg_waddr] <= reg_wdata; // 0号寄存器保护  
    end

  end

  assign rs1_data = (rs1 == 0) ? 32'b0 : rf[rs1];
  assign rs2_data = (rs2 == 0) ? 32'b0 : rf[rs2];//如果 rs2 没有用到或者等于 0 号寄存器，输出就是 0
`ifdef VERILATOR
  always @(*) begin
    if(io_ifu_rdata == 32'h00100073 && rf[10] == 32'b0)begin
      halt(pc,0);
    end   
    else if(io_ifu_rdata == 32'h00100073 && rf[10] == 32'b1)begin
      //$display("a0=0x%08x",rf[10]);
      halt(pc,1);
    end
  end
`endif

endmodule


