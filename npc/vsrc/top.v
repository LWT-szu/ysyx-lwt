module top(
    input clk,         
    input resetn,      
    input ps2_clk,     
    input ps2_data,     
    
    
    output [7:0] seglow_keycode,   
    output [7:0] seghigh_keycode, 
    output [7:0] seglow_ascii,     
    output [7:0] seghigh_ascii,    
    output [7:0] seglow_count,      
    output [7:0] seghigh_count,      
    output f0,

    output debug_ps2_clk,  
    output debug_ps2_data
);
    
    
    reg [7:0] scan_code;       
    reg data_valid;            
    reg key_released;          
    
    reg [7:0] ascii_code;      
    reg [7:0] key_code;        
    reg [7:0] counter;         


    assign debug_ps2_clk = ps2_clk;
    assign debug_ps2_data = ps2_data;

    // PS/2键盘接口模块实例化
    ps2_keyboard ps2_keyboard_inst(
        .clk(clk),
        .resetn(resetn),
        .ps2_clk(ps2_clk),
        .ps2_data(ps2_data),
        .scan_code(scan_code),
        .data_valid(data_valid),
        .key_released(key_released),
        .f0(f0)
    );

    // 键码到ASCII码转换模块实例化
    keycode_Ascii_out keycode_ascii_inst(
        .clk(clk),
        .scan_code(scan_code),
        .key_released(key_released),
        .data_valid(data_valid),
        .key_code(key_code),
        .ascii_code(ascii_code)
    );

    // 按键计数器模块实例化
    key_counter key_counter_inst(
        .clk(clk),
        .resetn(resetn),
        .data_valid(data_valid),
        .key_released(key_released),
        .scan_code(scan_code),
        .f0(f0),
        .counter(counter)
    );

    // 显示模块实例化
    display_result display_inst(
        .clk(clk),
        .scan_code(key_code),       // 使用稳定的key_code而不是实时scan_code
        .f0(f0),
        .ascii_code(ascii_code),
        .counter(counter),
        
        .seglow_keycode(seglow_keycode),
        .seghigh_keycode(seghigh_keycode),
        .seglow_ascii(seglow_ascii),
        .seghigh_ascii(seghigh_ascii),
        .seglow_count(seglow_count),
        .seghigh_count(seghigh_count)
    );

endmodule



module ps2_keyboard(clk, resetn, ps2_clk, ps2_data,scan_code,data_valid,key_released,f0);
    input clk;          
    input resetn;       
    input ps2_clk;      
    input ps2_data;     
    output reg [7:0]scan_code;
    output reg data_valid;
    output reg key_released;
    output reg f0;

    reg [9:0] buffer;   
    reg [3:0] count;     // 位计数器：记录当前已接收的位数
    reg [2:0] ps2_clk_sync;
   
    always @(posedge clk) begin
        ps2_clk_sync <= {ps2_clk_sync[1:0], ps2_clk}; 
        
    end

    wire sampling = ps2_clk_sync[2] & ~ps2_clk_sync[1];

    // 主接收逻辑,状态转移
    always @(posedge clk) begin
        if (resetn == 0) begin 
            count <= 0;        // 清零位计数器
            data_valid <= 0;   
            key_released <= 0;
            
        end else if (sampling) begin 
                if (count == 4'd10) begin
                    if ((buffer[0] == 0) && (ps2_data) && (^buffer[9:1])) begin   //奇校验
                      $display("receive %x", buffer[8:1]);
                      scan_code <= buffer[8:1];
                      
                      data_valid <= 1;

                      f0 <= key_released;
                      key_released <= (buffer[8:1] == 8'hF0);     
                        
                    end
                    count <= 0; 
                
                end else begin 
                    buffer[count] <= ps2_data; 
                    count <= count + 1;        
                    data_valid <= 0;
                end
        end else begin
              data_valid <= 0;
            end
      end
        
endmodule

//七段数码管的显示
module display_result (
  input clk,
  input[7:0]scan_code,
  input f0,
  input [7:0]ascii_code,//键码转换后的ASCII码
  input [7:0]counter,

  output reg[7:0]seglow_keycode,
  output reg[7:0]seghigh_keycode,

  output reg[7:0]seglow_ascii,
  output reg[7:0]seghigh_ascii,

  output reg[7:0]seglow_count,
  output reg[7:0]seghigh_count
);
  function [7:0] bin_to_seg;
    input [3:0] bin;
    begin
    case (bin)
      4'h0: bin_to_seg = 8'b00000011;   
      4'h1: bin_to_seg = 8'b10011111;
      4'h2: bin_to_seg = 8'b00100101;
      4'h3: bin_to_seg = 8'b00001101;
      4'h4: bin_to_seg = 8'b10011001;
      4'h5: bin_to_seg = 8'b01001001;
      4'h6: bin_to_seg = 8'b01000001;
      4'h7: bin_to_seg = 8'b00011111;
      4'h8: bin_to_seg = ~8'b11111110;
      4'h9: bin_to_seg = ~8'b11100110;
      4'hA: bin_to_seg = ~8'b11101110;
      4'hB: bin_to_seg = ~8'b00111110;
      4'hC: bin_to_seg = ~8'b10011100;
      4'hD: bin_to_seg = ~8'b01111010;
      4'hE: bin_to_seg = ~8'b10011110;
      4'hF: bin_to_seg = ~8'b10001110;
      default: bin_to_seg = 8'b1;
    endcase
    end
  endfunction

  always @(posedge clk) begin
    if(f0)begin

      seglow_keycode <= 8'b11111111;
      seghigh_keycode <= 8'b11111111;

      seglow_ascii <= 8'b11111111;
      seghigh_ascii <= 8'b11111111;
      
    end else begin
      seghigh_count <= bin_to_seg(counter[7:4]); 
      seglow_count <= bin_to_seg(counter[3:0]); // 计数低4位
      
      // ASCII码显示
      seghigh_ascii <= bin_to_seg(ascii_code[7:4]);
      seglow_ascii <= bin_to_seg(ascii_code[3:0]);
      
      // 原始键码显示
      seghigh_keycode <= bin_to_seg(scan_code[7:4]);
      seglow_keycode <= bin_to_seg(scan_code[3:0]);
    end
  end


  
endmodule


//「键码→ASCII 转换」
module keycode_Ascii_out( 
  input clk,
  input [7:0]scan_code,//键码
  input key_released,
  input data_valid,
  output reg[7:0] key_code,
  output reg[7:0] ascii_code
);

  always @(posedge clk) begin
      if(!key_released && data_valid)begin
        key_code <= scan_code;  // 仅更新有效键码
      end
    end

  shift_ASCII rom(
    .scan_code(key_code),
    .ascii(ascii_code)
  );
    
endmodule

//计数
module key_counter (
  input clk,
  input resetn,
  input data_valid,
  input key_released,
  input [7:0]scan_code,
  input f0,
  output reg [7:0]counter //按键被按下的总次数
);
  
  always @(posedge clk) begin
    if(!resetn)begin
      counter <= 0;
    end

    else if(data_valid && f0 ) begin
      counter <= counter + 8'b00000001;
    end
  end
endmodule

//转化为ASCII码
module shift_ASCII(scan_code,ascii);
    input [7:0]scan_code;
    output reg [7:0]ascii;

    always @(*) begin       //ROM
      case (scan_code)
        //字母键 (A-Z)
        8'h1C: ascii = 8'h41;
        8'h32: ascii = 8'h42;
        8'h21: ascii = 8'h43;
        8'h23: ascii = 8'h44;
        8'h24: ascii = 8'h45;
        8'h2B: ascii = 8'h46;
        8'h34: ascii = 8'h47;
        8'h33: ascii = 8'h48;
        8'h3B: ascii = 8'h49;
        8'h42: ascii = 8'h4B; 
        8'h4B: ascii = 8'h4C; 
        8'h3A: ascii = 8'h4D; 
        8'h31: ascii = 8'h4E; 
        8'h44: ascii = 8'h4F; 
        8'h4D: ascii = 8'h50; 
        8'h15: ascii = 8'h51; 
        8'h2D: ascii = 8'h52; 
        8'h1B: ascii = 8'h53; 
        8'h2C: ascii = 8'h54; 
        8'h3C: ascii = 8'h55; 
        8'h2A: ascii = 8'h56; 
        8'h1D: ascii = 8'h57; 
        8'h22: ascii = 8'h58; 
        8'h35: ascii = 8'h59; 
        8'h1A: ascii = 8'h5A; 

        // 数字(0-9)
        8'h45: ascii = 8'h30;
        8'h16: ascii = 8'h31; 
        8'h1E: ascii = 8'h32; 
        8'h26: ascii = 8'h33; 
        8'h25: ascii = 8'h34; 
        8'h2E: ascii = 8'h35; 
        8'h36: ascii = 8'h36; 
        8'h3D: ascii = 8'h37; 
        8'h3E: ascii = 8'h38; 
        8'h46: ascii = 8'h39; 

        default: ascii = 8'h00;      
      endcase
    end
endmodule