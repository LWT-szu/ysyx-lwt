module lsu (
    input               clk,
    input               rst, 
    // 控制信号（来自IDU）
    input reg lsu_respValid,
    output reg lsu_reqValid,
    input               lsu_valid,  // 访存有效标志(mem_valid)（1=需要访存）
    input               mem_wen,    // 写使能（1=store，0=load）
    input       [31:0]  mem_addr,   // 访存地址（读/写共用）
    input       [31:0]  mem_wdata,  // 待写入数据（store时有效）
    input       [3:0]   mem_wmask,  // 写掩码（每bit对应1字节，1=写入）
    // 输出信号（到EXU/WBU）
    output reg  [31:0]  mem_rdata,  // 读出的数据（load时有效）
    output reg          lsu_done,    // 访存完成标志（1=操作结束）

    output [31:0] lsu_addr,
    output        lsu_wen,
    output [31:0] lsu_wdata,
    output [ 3:0] lsu_wmask,
    input  [31:0] lsu_rdata
);

    assign lsu_wen    = mem_wen;
    assign lsu_wmask  = mem_wmask;
    assign lsu_wdata  = mem_wdata;

    // 状态定义（基础2状态机）
    localparam IDLE  = 1'b0;  // 空闲状态：等待访存请求
    localparam WAIT  = 1'b1;  // 等待状态：处理访存并延迟1周期

    // 内部信号
    reg         state;               // 状态寄存器

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state       <= IDLE;
        end else begin
            case (state)
                IDLE: begin
                    if (lsu_respValid) begin
                        if (!mem_wen) begin
                            state     <= WAIT;
                        end else begin
                            state     <= IDLE;
                        end
                    end
                end
                WAIT: begin
                    // state       <= pc_update_en ? IDLE : WAIT;
                    state       <= IDLE;
                end
            endcase
        end
    end

    always @(*) begin
        mem_rdata = 32'b0;
        case (state)
            IDLE:begin
                if (lsu_valid) begin
                    lsu_addr = mem_addr;
                    lsu_reqValid = 1;
                end
                lsu_done = lsu_valid ? (mem_wen ? 1 : 0) : 1;
            end 
            WAIT:begin
                if (lsu_respValid) begin
                    mem_rdata    = lsu_rdata;
                end
                lsu_done = 1;
            end
        endcase
    end
endmodule