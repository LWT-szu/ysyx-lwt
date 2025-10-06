import "DPI-C" function void pmem_write(
  input  int waddr, input int  wdata, input byte wmask,input int pc);
  
module LSU (
    input clk,
    input rst,
    input valid,              // 是否有访存请求  ym
    input wen_ram,                // 是否是写入 ym Write_enable
    input [31:0]raddr_ram,        // 读地址EXU lsu_addr
    input [31:0]waddr_ram,        // 写地址EXU lsu_addr
    input [31:0]wdata_ram,        //  要写入的数据gpr
    input is_sb_type,
    input is_sh_type,
    input is_lh_type,
    input [31:0]pc,

    output reg [31:0]rdata_ram,
    output reg load_wait // load指令等待信号
    /*output [31:0] lsu_addr,
    output        lsu_wen,
    output [31:0] lsu_wdata,
    output [ 3:0] lsu_wmask,
    input  [31:0] lsu_rdata,*/

);
    wire [7:0]wmask;
    reg [31:0] data_ram;

    always @(*) begin
    if (valid && !wen_ram)begin
        load_wait = 1'b1;
        //$display("LSU load_wait = %d pc = 0x%08x",load_wait,pc);
    end else begin
        load_wait = 1'b0;
        //$display("LSU load_wait = %d pc = 0x%08x",load_wait,pc);
        end
    end
    

    assign wmask = is_sb_type ? (8'b00000001 << waddr_ram[1:0]): 
            is_sh_type ? (waddr_ram[1] ? 8'b00001100 : 8'b00000011) :
            8'b00001111;

    // 组合逻辑计算写入数据
    always @(*) begin
        data_ram = 32'b0; 
        if(is_sb_type)begin//sb
            case (waddr_ram[1:0])
                2'b00:begin
                    data_ram = {24'b0,wdata_ram[7:0]};
                end 
                2'b01:begin
                    data_ram = {16'b0,wdata_ram[7:0],8'b0};
                end 
                2'b10:begin
                    data_ram = {8'b0,wdata_ram[7:0],16'b0};
                end 
                2'b11:begin
                    data_ram = {wdata_ram[7:0],24'b0};
                end 
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

    always @(posedge clk) begin
        if (rst) begin
            rdata_ram <= 0;
        end else begin
            if(valid && !wen_ram)begin
                rdata_ram <= pmem_read(raddr_ram,pc,{31'b0, valid},{31'b0, wen_ram});// 有读请求时
                //$display("LSU rdata_ram = %d  raddr_ram = 0x%08x rdata_ram = 0x%08x pc = 0x%08x",rdata_ram,raddr_ram,rdata_ram,pc);
            end
            else if (valid && wen_ram) pmem_write(waddr_ram, data_ram, wmask,pc);// 有写请求时
            else begin
                rdata_ram <= 0;
            end

        end
    end
/*
    always @(posedge clk) begin
        if (valid && wen_ram) begin // 有写请求时
            pmem_write(waddr_ram, data_ram, wmask,pc);
    end
    
    end
*/
endmodule