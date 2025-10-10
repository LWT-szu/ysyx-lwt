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
    //$display("opcode_alu = %07b,func_alu=%03b,alu_result=%08x,rs1_alu=%08x,in2_alu=%08x| pc = %08x\n",opcode_alu,func_alu,alu_result,rs1_alu,in2_alu,pc);
    case (opcode_alu)
    
      7'b0010011: begin // I型算术（如 addi sltiu andi）
        if (func_alu == 3'b000)
          alu_result = rs1_alu + in2_alu;//addi
        else if(func_alu == 3'b011)
          alu_result = ($unsigned(rs1_alu) < $unsigned(in2_alu)) ? 32'd1 : 32'd0;//sltiu
        else if(func_alu == 3'b101 && func7_alu == 7'b0100000)
          alu_result = $signed(rs1_alu) >>> imm_alu[4:0];// srai
          //$display("opcode_alu = %07b,func_alu=%03b,alu_result=%08x,rs1_alu=%08x,imm_alu[4:0]=%05b| pc = %08x\n",opcode_alu,func_alu,alu_result,rs1_alu,imm_alu[4:0],pc);
        else if(func_alu == 3'b111)
          alu_result = rs1_alu & in2_alu; //andi
        else if(func_alu == 3'b100)
          alu_result = rs1_alu ^ imm_alu;//xori
        else if(func_alu == 3'b101 && func7_alu == 7'b0000000)
          alu_result = rs1_alu >> imm_alu[4:0];//srli
        else if(func_alu == 3'b001 && func7_alu == 7'b0000000)
          alu_result = rs1_alu << imm_alu[4:0];//slli
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
        3'b011: alu_result = ( $unsigned(rs1_alu) < $unsigned(in2_alu)) ? 1 : 0;//sltu
        3'b110: alu_result = rs1_alu | in2_alu ;//or
        3'b100: alu_result = rs1_alu ^ in2_alu ;//xor
        3'b010: alu_result = ( $signed(rs1_alu) < $signed(in2_alu) ) ? 1 : 0;//slt
        3'b101: begin
          if(func7_alu == 7'b0100000) alu_result = ( $signed(rs1_alu) >>> in2_alu );//sra
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
            3'b111: branch_taken = ($unsigned(rs1_alu) >= $unsigned(in2_alu) );//bgeu
            3'b110: branch_taken = ($unsigned(rs1_alu) < $unsigned(in2_alu) );//bltu
            3'b101: branch_taken = ($signed(rs1_alu) >= $signed(in2_alu) );//bge
            3'b100: branch_taken = ($signed(rs1_alu) < $signed(in2_alu) );//blt
            default: ;
          endcase
          branch_target = pc + imm_alu;
        end
      end

      7'b1110011: begin // csrrw
        if(csr_write && func_alu == 3'b001)begin
          case (imm_alu[11:0])
            12'hB00: alu_result = mcycle[31:0]; // mcycle
            12'hB80: alu_result = mcycle[63:32]; // mcycleh
            default: alu_result = 32'b0;
          endcase
        end else if (csr_write && func_alu == 3'b010 )begin//csrrs
          alu_csr = mcycle;
          case (imm_alu[11:0])
            12'hF11: alu_result = 32'h79737978; // mvendorid
            12'hF12: alu_result = 32'h25080201; // marchid
            12'hB00: begin
              alu_result = mcycle[31:0];
              if(rs1_alu != 0)begin
                alu_csr = {mcycle[63:32],mcycle[31:0] | rs1_alu }; // mcycle
              end
            end
            12'hB80: begin
              alu_result = mcycle[63:32];
              if(rs1_alu != 0)begin
                alu_csr = {mcycle[63:32] | rs1_alu , mcycle[31:0]}; // mcycleh
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
/*
  if((func_alu == 3'b000 && (opcode_alu == 7'b0010011 | opcode_alu == 7'b1100111) ) ||(func_alu == 3'b000 && opcode_alu == 7'0000011))begin//addi,jalr
    alu_result = in2_alu + rs1_alu;
  end 

  else if (func_alu == 3'b000 && opcode_alu == 7'b0110011) begin//add
    alu_result = in2_alu + rs1_alu;
  end

  else if (opcode_alu == 7'b0110111)  begin//lui
    alu_result = in2_alu;
  end

  else begin
    alu_result = 32'b0;
  end
  */

