import "DPI-C" function int  pmem_read(input int  raddr,input int  pc,input int valid,input int wen_ram);
module MEM(
    input clk,
    input [31:0] addr,
    input inst_valid,
    output reg [31:0] data_out
);
    always @(posedge clk) begin
        if(inst_valid)begin
            data_out <= pmem_read(addr,addr,0,1);
            $display("1. MEM: addr=%08x, data=%08x", addr, data_out);
        end
        else begin
            
            $display("2. MEM: addr=%08x, data=%08x", addr, data_out);
        end 
    end
    
endmodule
/*  MEM MEM_init(  //访存
    .clk(clk),
    .addr(infu_raddr),//读地址
    .inst_valid(inst_valid),

    .data_out(ifu_data)
  );
  */