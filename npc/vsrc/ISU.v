import "DPI-C" function void pmem_write(
  input int waddr, input int wdata, input byte wmask);
  
module ISU (
    input clk,
    input valid,              // 是否有访存请求  ym
    input wen_ram,                // 是否是写入 ym Write_enable
    input [31:0]raddr_ram,        // 读地址EXU
    input [31:0]waddr_ram,        // 写地址EXU
    input [31:0]wdata_ram,        //  要写入的数据gpr
    input is_sb_type,
    input [31:0]pc,
    output reg [31:0]rdata_ram

);
    wire [7:0]wmask;        
    assign wmask = is_sb_type ? (8'b00000001 << waddr_ram[1:0]): 8'b00001111;
    always @(*) begin
        if(valid && !wen_ram)begin
            //$display("pc=0x%08x", pc);
            rdata_ram = pmem_read(raddr_ram,pc);// 有读请求时
        end
        else begin
            rdata_ram = 0;
        end
    end

    always @(posedge clk) begin
        if (valid && wen_ram) begin // 有写请求时
        pmem_write(waddr_ram, wdata_ram, wmask);

    end
    end

endmodule