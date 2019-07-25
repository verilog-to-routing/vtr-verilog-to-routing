
 module ex1EP16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [14:0] x1 ;
 wire [18:0] x3 ;
 wire [26:0] x6 ;
 wire [26:0] y0 ;
 wire [26:0] y5 ;
 wire [17:0] x4 ;
 wire [26:0] x5 ;
 wire [27:0] y1 ;
 wire [27:0] y4 ;
 wire [16:0] x2 ;
 wire [11:0] x0_6_1 ;
 wire [11:0] x0_6_2 ;
 wire [11:0] x0_6_3 ;
 wire [26:0] y2 ;
 wire [11:0] x0_7_1 ;
 wire [11:0] x0_7_2 ;
 wire [11:0] x0_7_3 ;
 wire [26:0] y3 ;
 wire [26:0] x7 ;
 wire [28:0] x8 ;
 wire [28:0] x8_1 ;
 wire [28:0] x9 ;
 wire [28:0] x9_1 ;
 wire [28:0] x10 ;
 wire [28:0] x10_1 ;
 wire [28:0] x11 ;
 wire [28:0] x11_1 ;
 wire [28:0] Out_Y_wire ;
 reg [28:0] Out_Y;


 always @(posedge clk) 
 begin
 x0 <= In_X; 
 end 


 always @(posedge clk) 
 begin
 Out_Y = Out_Y_wire;
 end


 Adder14Bit Adder0(clk,  {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x1); 


 Subtractor18Bit Sub0(clk, { x0[11: 0], 6'b0 }, {x1[14],x1[14],x1[14], x1[14: 0] }, x3); 


 Adder17Bit Adder1(clk,  { x0[11: 0], 5'b0 }, {x1[14],x1[14], x1[14: 0]  }, x4); 


 Subtractor26Bit Sub1(clk, { x1[14: 0], 11'b0 }, {x4[17],x4[17],x4[17],x4[17],x4[17],x4[17],x4[17],x4[17], x4[17: 0] }, x5); 

 Register28Bit Reg0 (clk, {x5, 1'b0}, y1 );
 Register28Bit Reg1 (clk, {x5, 1'b0}, y4 );

 Subtractor16Bit Sub2(clk, { x0[11: 0], 4'b0 }, {x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x2); 


 Adder26Bit Adder2(clk,  {x2[16],x2[16],x2[16],x2[16],x2[16],x2[16],x2[16],x2[16],x2[16], x2[16: 0] },  { x3[18: 0], 7'b0 },x6); 

 Register27Bit Reg2 (clk,x6, y0 );
 Register27Bit Reg3 (clk,x6, y5 );
 Register12Bit Reg4 (clk,x0, x0_6_1 );
 Register12Bit Reg5 (clk,x0_6_1, x0_6_2 );
 Register12Bit Reg6 (clk,x0_6_2, x0_6_3 );
 Register27Bit Reg7 (clk, {x0_6_3, 15'b0}, y2 );
 Register12Bit Reg8 (clk,x0, x0_7_1 );
 Register12Bit Reg9 (clk,x0_7_1, x0_7_2 );
 Register12Bit Reg10 (clk,x0_7_2, x0_7_3 );
 Register27Bit Reg11 (clk, {x0_7_3, 15'b0}, y3 );
 Register27Bit Reg12 (clk,y0, x7 );

 Adder28Bit Adder3(clk,  {x7[26], x7[26: 0] },  { y1[27: 0]  }, x8); 

 Register29Bit Reg13 (clk,x8, x8_1 );

 Adder29Bit Adder4(clk,  { x8_1[28: 0] },  {y2[26],y2[26], y2[26: 0]  }, x9); 

 Register29Bit Reg14 (clk,x9, x9_1 );

 Adder29Bit Adder5(clk,  { x9_1[28: 0] },  {y3[26],y3[26], y3[26: 0]  }, x10); 

 Register29Bit Reg15 (clk,x10, x10_1 );

 Adder29Bit Adder6(clk,  { x10_1[28: 0] },  {y4[27], y4[27: 0]  }, x11); 

 Register29Bit Reg16 (clk,x11, x11_1 );

 Adder29Bit Adder7(clk,  { x11_1[28: 0] },  {y5[26],y5[26], y5[26: 0]  }, Out_Y_wire); 

 endmodule 



module Adder14Bit (clk, In1, In2, AddOut);
 input clk;
 input [13:0] In1, In2; 

 output [14:0] AddOut;

 reg [14 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder26Bit (clk, In1, In2, AddOut);
 input clk;
 input [25:0] In1, In2; 

 output [26:0] AddOut;

 reg [26 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder17Bit (clk, In1, In2, AddOut);
 input clk;
 input [16:0] In1, In2; 

 output [17:0] AddOut;

 reg [17 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder28Bit (clk, In1, In2, AddOut);
 input clk;
 input [27:0] In1, In2; 

 output [28:0] AddOut;

 reg [28 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder29Bit (clk, In1, In2, AddOut);
 input clk;
 input [28:0] In1, In2; 

 output [28:0] AddOut;

 reg [28 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule



module Subtractor18Bit (clk, In1, In2, SubOut);
 input clk;
 input [17:0] In1, In2;
 output [18 :0] SubOut;

 reg [18 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule

module Subtractor26Bit (clk, In1, In2, SubOut);
 input clk;
 input [25:0] In1, In2;
 output [26 :0] SubOut;

 reg [26 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule

module Subtractor16Bit (clk, In1, In2, SubOut);
 input clk;
 input [15:0] In1, In2;
 output [16 :0] SubOut;

 reg [16 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule



module Register27Bit (clk, In1, RegOut);
 input clk;
 input [26:0] In1;
 output [26 :0] RegOut;

 reg [26 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register28Bit (clk, In1, RegOut);
 input clk;
 input [27:0] In1;
 output [27 :0] RegOut;

 reg [27 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register12Bit (clk, In1, RegOut);
 input clk;
 input [11:0] In1;
 output [11 :0] RegOut;

 reg [11 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register29Bit (clk, In1, RegOut);
 input clk;
 input [28:0] In1;
 output [28 :0] RegOut;

 reg [28 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule


