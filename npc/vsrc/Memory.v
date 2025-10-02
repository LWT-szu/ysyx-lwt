module ROM(
    input [31:0] addr,
    output [31:0] data_out
);
    reg [31:0] rom[0:255]; // 256 x 32 bits = 1KB
    // 初始化ROM内容，可以从文件加载
    initial begin
        $readmemh("D:/npc/inst/inst_rom.txt", rom);
    end

    assign data_out = rom[addr[9:2]]; // 使用 addr 的高 8 位作为索引，忽略最低的 2 位（字节对齐）
    
endmodule