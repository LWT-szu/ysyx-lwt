//ALU算术逻辑单元
module EXU (
  input [31:0]imm_alu,        // 立即数（I型/S型/U型等）或无效
  input [31:0]rs1_alu,
  input [31:0]rs2_alu,        // R型时有效，其它时无效
  input [2:0]func_alu,
  input [6:0]opcode_alu,
  input alu_src,              // 1: imm_alu, 0: rs2_alu
  output reg [31:0]alu_result,// 寄存器写回
  output reg [31:0]alu_ram    // 访存地址
  //注意去掉逗号！！！！！！！！！！！！！！
);
  // 选择ALU第二操作数
  wire  [31:0]in2_alu = alu_src ? imm_alu : rs2_alu;

  always @(*) begin
    alu_result = 32'b0;
    alu_ram = 32'b0;
    case (opcode_alu)
    
      7'b0010011: begin // I型算术（如 addi）
        if (func_alu == 3'b000)
          alu_result = rs1_alu + in2_alu;
      end

      7'b0110011: begin // R型算术（如 add）
        if (func_alu == 3'b000)
          alu_result = rs1_alu + in2_alu;
      end

      7'b1100111: begin // jalr
        alu_result = rs1_alu + in2_alu;
      end

      7'b0000011: begin // load（如 lw\lbu）
        if(func_alu == 3'b010 || func_alu == 3'b100)begin
          alu_ram = rs1_alu + in2_alu;
        end
      end

      7'b0100011: begin // store（如 sb,sw）
        if(func_alu == 3'b010 || func_alu == 3'b000)begin
          alu_ram = rs1_alu + in2_alu;
        end
      end

      7'b0110111: begin // lui
        alu_result = in2_alu;
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

