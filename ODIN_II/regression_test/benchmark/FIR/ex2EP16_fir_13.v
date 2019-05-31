
 module ex2EP16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [14:0] x1 ;
 wire [21:0] x3 ;
 wire [27:0] x8 ;
 wire [27:0] y3 ;
 wire [27:0] x13 ;
 wire [28:0] y5 ;
 wire [20:0] x4 ;
 wire [20:0] x4_0_1 ;
 wire [20:0] y0 ;
 wire [22:0] x5 ;
 wire [22:0] x5_0_1 ;
 wire [24:0] y1 ;
 wire [19:0] x6 ;
 wire [24:0] x7 ;
 wire [25:0] y2 ;
 wire [14:0] x8_5_1 ;
 wire [20:0] x9 ;
 wire [24:0] x10 ;
 wire [27:0] y4 ;
 wire [14:0] x2 ;
 wire [14:0] x7_1_1 ;
 wire [14:0] x10_2_1 ;
 wire [11:0] x4_5_1 ;
 wire [19:0] x11 ;
 wire [26:0] x12 ;
 wire [11:0] x12_8_1 ;
 wire [20:0] x14 ;
 wire [25:0] x15 ;
 wire [25:0] x15_1 ;
 wire [26:0] x16 ;
 wire [26:0] x16_1 ;
 wire [28:0] x17 ;
 wire [28:0] x17_1 ;
 wire [28:0] x18 ;
 wire [28:0] x18_1 ;
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


 Subtractor14Bit Sub0(clk, { x0[11: 0], 2'b0 }, {x0[11],x0[11], x0[11: 0] }, x1); 


 Subtractor21Bit Sub1(clk, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 6'b0 },x3); 


 Adder22Bit Adder0(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x1[14: 0], 7'b0 },x5); 

 Register23Bit Reg0 (clk,x5, x5_0_1 );
 Register25Bit Reg1 (clk, {x5_0_1, 2'b0}, y1 );
 Register15Bit Reg2 (clk,x1, x8_5_1 );

 Adder27Bit Adder1(clk,  { x8_5_1[14: 0], 12'b0 }, {x3[21],x3[21],x3[21],x3[21],x3[21], x3[21: 0]  }, x8); 

 Register28Bit Reg3 (clk,x8, y3 );

 Adder20Bit Adder2(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x1[14: 0], 5'b0 },x9); 


 Adder14Bit Adder3(clk,  {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x2); 


 Subtractor19Bit Sub2(clk, { x1[14: 0], 4'b0 }, {x2[14],x2[14],x2[14],x2[14], x2[14: 0] }, x6); 

 Register15Bit Reg4 (clk,x2, x7_1_1 );

 Adder24Bit Adder4(clk,  { x7_1_1[14: 0], 9'b0 }, {x6[19],x6[19],x6[19],x6[19], x6[19: 0]  }, x7); 

 Register26Bit Reg5 (clk, {x7, 1'b0}, y2 );
 Register15Bit Reg6 (clk,x2, x10_2_1 );

 Adder24Bit Adder5(clk,  { x10_2_1[14: 0], 9'b0 }, {x9[20],x9[20],x9[20], x9[20: 0]  }, x10); 

 Register28Bit Reg7 (clk, {x10, 3'b0}, y4 );
 Register12Bit Reg8 (clk,x0, x4_5_1 );

 Adder20Bit Adder6(clk,  { x4_5_1[11: 0], 8'b0 }, {x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0]  }, x4); 

 Register21Bit Reg9 (clk,x4, x4_0_1 );
 Register21Bit Reg10 (clk,x4_0_1, y0 );

 Adder19Bit Adder7(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 7'b0 },x11); 

 Register12Bit Reg11 (clk,x0, x12_8_1 );

 Subtractor26Bit Sub3(clk, { x12_8_1[11: 0], 14'b0 }, {x11[19],x11[19],x11[19],x11[19],x11[19],x11[19], x11[19: 0] }, x12); 


 Adder27Bit Adder8(clk,  {x3[21],x3[21], x3[21: 0], 3'b0 }, { x12[26: 0]  }, x13); 

 Register29Bit Reg12 (clk, {x13, 1'b0}, y5 );
 Register21Bit Reg13 (clk,y0, x14 );

 Adder25Bit Adder9(clk,  {x14[20],x14[20],x14[20],x14[20], x14[20: 0] },  { y1[24: 0]  }, x15); 

 Register26Bit Reg14 (clk,x15, x15_1 );

 Adder26Bit Adder10(clk,  { x15_1[25: 0] },  { y2[25: 0]  }, x16); 

 Register27Bit Reg15 (clk,x16, x16_1 );

 Adder28Bit Adder11(clk,  {x16_1[26], x16_1[26: 0] },  { y3[27: 0]  }, x17); 

 Register29Bit Reg16 (clk,x17, x17_1 );

 Adder29Bit Adder12(clk,  { x17_1[28: 0] },  {y4[27], y4[27: 0]  }, x18); 

 Register29Bit Reg17 (clk,x18, x18_1 );

 Adder29Bit Adder13(clk,  { x18_1[28: 0] },  { y5[28: 0]  }, Out_Y_wire); 

 endmodule 



module Adder27Bit (clk, In1, In2, AddOut);
 input clk;
 input [26:0] In1, In2; 

 output [27:0] AddOut;

 reg [27 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder20Bit (clk, In1, In2, AddOut);
 input clk;
 input [19:0] In1, In2; 

 output [20:0] AddOut;

 reg [20 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder22Bit (clk, In1, In2, AddOut);
 input clk;
 input [21:0] In1, In2; 

 output [22:0] AddOut;

 reg [22 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder24Bit (clk, In1, In2, AddOut);
 input clk;
 input [23:0] In1, In2; 

 output [24:0] AddOut;

 reg [24 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
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

module Adder19Bit (clk, In1, In2, AddOut);
 input clk;
 input [18:0] In1, In2; 

 output [19:0] AddOut;

 reg [19 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder25Bit (clk, In1, In2, AddOut);
 input clk;
 input [24:0] In1, In2; 

 output [25:0] AddOut;

 reg [25 :0] AddOut; 

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



module Subtractor14Bit (clk, In1, In2, SubOut);
 input clk;
 input [13:0] In1, In2;
 output [14 :0] SubOut;

 reg [14 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule

module Subtractor21Bit (clk, In1, In2, SubOut);
 input clk;
 input [20:0] In1, In2;
 output [21 :0] SubOut;

 reg [21 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule

module Subtractor19Bit (clk, In1, In2, SubOut);
 input clk;
 input [18:0] In1, In2;
 output [19 :0] SubOut;

 reg [19 :0] SubOut; 

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

module Register21Bit (clk, In1, RegOut);
 input clk;
 input [20:0] In1;
 output [20 :0] RegOut;

 reg [20 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register23Bit (clk, In1, RegOut);
 input clk;
 input [22:0] In1;
 output [22 :0] RegOut;

 reg [22 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register25Bit (clk, In1, RegOut);
 input clk;
 input [24:0] In1;
 output [24 :0] RegOut;

 reg [24 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register26Bit (clk, In1, RegOut);
 input clk;
 input [25:0] In1;
 output [25 :0] RegOut;

 reg [25 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register15Bit (clk, In1, RegOut);
 input clk;
 input [14:0] In1;
 output [14 :0] RegOut;

 reg [14 :0] RegOut; 

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


