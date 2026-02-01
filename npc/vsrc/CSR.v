module CSR(
    input clk,
    input rst,
    input [31:0] pc,
    input [31:0] trap_NO,
    input [11:0] csr_addr,
    input [31:0] csr_wdata,
    input is_ecall,
    input csr_write_en,
    output reg [31:0] csr_rdata,
    output [31:0] csr_mtvec,
    output  [31:0] csr_mepc

);
    reg [63:0] mcycle_merge;
    reg [31:0] mvendorid;   //厂商ID 
    reg [31:0] marchid;     //架构ID
    reg [31:0] mtvec;       //0x305 机器模式中断向量基地址 
    reg [31:0] mepc;        //0x341 机器模式异常程序计数器
    reg [31:0] mcause;      //0x342 机器模式异常原因寄存器 异常号NO
    reg [31:0] mstatus;     //0x300 机器模式状态寄存器

    assign csr_mtvec = mtvec;
    assign csr_mepc = mepc;
    always @(posedge clk or posedge rst)begin//时序逻辑用来写csr
        if(rst)begin
            mcycle_merge <= 64'b0;
            mvendorid <= 32'h79737978;//ysyx
            marchid <= 32'h017EB189;//25080201
            mtvec <= 32'b0;
            mepc <= 32'b0;
            mcause <= 32'b0;
            mstatus <= 32'h00; //默认MIE置位00001800????00005170???0000???
        end else begin
            mcycle_merge <= mcycle_merge + 1;
            if(is_ecall)begin
                mepc <= pc;//保存异常发生时的pc到mepc
                mcause <= trap_NO;//保存异常号到mcause
            end
            else if (csr_write_en) begin
                case (csr_addr)
                    12'hB00:;12'hB80:;12'hF11:; 12'hF12:;
                    12'h305: mtvec <= csr_wdata;//mtvec 
                    12'h300: mstatus <= csr_wdata;//mstatus
                    12'h341: mepc <= csr_wdata;
                    12'h342: mcause <= csr_wdata;
                    default: ;
                endcase
            end
        end
    end

    always @(*) begin//组合逻辑用来读csr
        case (csr_addr)
            12'hB00: csr_rdata = mcycle_merge[31:0];//mcycle
            12'hB80: csr_rdata = mcycle_merge[63:32];//mcycleh
            12'hF11: csr_rdata = mvendorid;//mvendorid
            //=============================
            12'hF12: csr_rdata = marchid;//marchid
            12'h300: csr_rdata = mstatus;//mstatus
            12'h305: csr_rdata = mtvec;//mtvec
            12'h341: csr_rdata = mepc;//mepc
            12'h342: csr_rdata = mcause;//mcause
            default: csr_rdata = 32'b0;
        endcase
    end
endmodule


//控制状态寄存器CSR
//包括读后写csrrw
//读后置位csrrs     |   csrrs x0 csr,rs1 只置位不读取
//读后清零csrrc