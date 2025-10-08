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
    input is_lh_type,
    input [31:0]pc,

    input reg io_lsu_respValid,//存储器响应有效（数据已准备好）
    output reg io_lsu_reqValid,//访存请求有效 发给存储器（MEM）
    output reg lsu_done, //访存完成标志 （通知下游模块）

    output reg [31:0]rdata_ram, // 读出的数据 (返回给EXU/WBU)
    output [31:0] io_lsu_addr,
    output        io_lsu_wen,
    output [31:0] io_lsu_wdata,
    output [ 3:0] io_lsu_wmask,
    output [ 1:0] io_lsu_size,

    output reg load_wait, // load指令等待信号
    output reg LSU_WAIT
);
    
    localparam IDLE = 1'b0;// 空闲状态：等待访存请求（上游发来 valid=1）
    localparam WAIT = 1'b1;// 等待状态：等待数据返回（respValid=1），只有访存请求发出后才进入
    reg state;

    assign LSU_WAIT = (state == WAIT);

    wire [3:0]wmask;
    reg [31:0] data_ram;

    //assign rdata_ram = io_lsu_respValid ? io_lsu_rdata : 32'b0;
    // 组合逻辑计算访存地址和写使能
    assign io_lsu_addr  = wen_ram ? waddr_ram : raddr_ram;
    assign io_lsu_wen   = wen_ram;//
    assign io_lsu_wdata = data_ram;//
    assign io_lsu_wmask = wmask;//
    assign io_lsu_size  = is_sb_type ? 2'b00 : 
                         is_sh_type ? 2'b01 : 
                         2'b10;
    //assign io_lsu_reqValid = valid;

assign wmask = io_lsu_wen ? (
                  is_sb_type ? (4'b0001 << waddr_ram[1:0]) : 
                  is_sh_type ? (waddr_ram[1] ? 4'b1100 : 4'b0011) : 
                  4'b1111
               ) : 4'b0000;

    
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
        end else begin
            case (state)
                IDLE: begin
                    // 读请求进入WAIT，写请求保持IDLE（写单周期完成）
                    if (valid && !wen_ram) begin//// 读请求
                        state <= WAIT;
                    end else if (valid && wen_ram) begin
                        state <= IDLE;
                    end
                end
                WAIT: begin
                    // 等待存储器respValid拉高时回到IDLE
                    if (io_lsu_respValid) begin// 存储器响应
                        state <= IDLE;
                    end else begin
                        state <= WAIT;
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
        io_lsu_reqValid = 1'b0;
        case (state)
            IDLE: begin
                if (valid) begin
                    io_lsu_reqValid = 1'b1;                // 发起访存请求
                    load_wait = !wen_ram;               // 读时进入等待
                    lsu_done = wen_ram ? 1'b1 : 1'b0;   // 写操作立即完成
                end
            end
            WAIT: begin
                if (io_lsu_respValid) begin
                    load_wait = 1'b0;                     // 读操作等待数据 
                    rdata_ram = io_lsu_rdata;            // 数据返回，采集结果
                    lsu_done = 1'b1;                   // 访存完成
                end else begin
                    load_wait = 1'b1;                   //  [延迟]读操作等待数据
                    rdata_ram = 32'b0;
                    lsu_done = 1'b0;
                end
                
            end
        endcase
    end 

endmodule
