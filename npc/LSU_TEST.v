// module ysyx_25080202_CSR(
//     input clk,
//     input rst,
//     input I_csrrs,
//     input I_csrrw,                    //判断指令是不是 csrrw
//     input [11:0] csr_addr,            // SR 地址 = inst[31:20]
//     input [31:0] csr_wdata,           //要写进CSR的数据
//     output reg [31:0] csr_rdata      //从CSR寄存器读到的数据

// );

//     reg [63:0] mcycle;
 

//     localparam [31:0] MVENDORID = 32'h79737978;  // "ysyx"
//     localparam [31:0] MARCHID   = 32'h017EB18A;  // 学号部分
//     //
//     always @(*) begin
//         if(I_csrrs||I_csrrw) begin
//         case (csr_addr)
//             12'hB00: csr_rdata = mcycle[31:0];
//             12'hB80: csr_rdata = mcycle[63:32];
//             12'hF11: csr_rdata = MVENDORID;
//             12'hF12: csr_rdata = MARCHID;
//             default: csr_rdata = 32'b0;
//         endcase
//         end
//         // if (I_csrrw) begin
//         //     $display("CSR exec: csr_addr=%h, csr_rdata=%h", csr_addr, csr_rdata);
//         // end
//     end

//     // ========= 时序逻辑：写 =========
//     always @(posedge clk) begin
//         if (rst) begin
//             mcycle <= 64'b0;
//         end else begin
//             mcycle <= mcycle + 1;

//             if (I_csrrw) begin
//                 case (csr_addr)
//                     12'hB00: mcycle[31:0] <= csr_wdata;
//                     12'hB80: mcycle[63:32] <= csr_wdata;
//                     12'hF11, 12'hF12: ;  // 只读 CSR，不允许写
//                     default: ;
//                 endcase
//             end else if (I_csrrs && csr_wdata != 32'b0) begin
//                 case (csr_addr)
//                     12'hB00: mcycle[31:0] <= mcycle[31:0] | csr_wdata;
//                     12'hB80: mcycle[63:32] <= mcycle[63:32] | csr_wdata;
//                     12'hF11, 12'hF12: ;  // 只读 CSR
//                     default: ;
//                 endcase
//             end
//         end
//     end

// endmodule