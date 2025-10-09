//译码
module ysyx_25080201_IDU (
  input [31:0]inst_ym,
  input [31:0]pc,
  input io_ifu_respValid,//指令是否有效
  input clock,

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
  //reg [31:0] idu_inst;
  /*
  reg idu_valid;
  always @(posedge clock) begin
    idu_valid <= inst_valid;//指令是否有效
  end
  */
  //idu_valid <= inst_valid;//指令是否有效

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
    //idu_inst  <= inst_out; 
    //idu_valid <= inst_valid;//指令是否有效
    //if (io_ifu_respValid) begin

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
            //$display("csrrw");
          end
        
          else begin
            //$display("========");
            //$display("1.Unknown/illegal inst_valid: %d instruction: %08x at pc=%08x", inst_valid,inst_ym, pc);
            //halt(pc,0);
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