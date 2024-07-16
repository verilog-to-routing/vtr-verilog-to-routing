
 module ex4EP16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [21:0] x1 ;
 wire [22:0] x5 ;
 wire [22:0] x5_0_1 ;
 wire [24:0] y0 ;
 wire [22:0] x5_1_1 ;
 wire [24:0] y9 ;
 wire [24:0] x6 ;
 wire [26:0] x9 ;
 wire [26:0] y1 ;
 wire [26:0] y8 ;
 wire [27:0] x7 ;
 wire [27:0] x7_0_1 ;
 wire [28:0] y3 ;
 wire [27:0] x7_1_1 ;
 wire [28:0] y6 ;
 wire [14:0] x2 ;
 wire [24:0] x8 ;
 wire [27:0] x10 ;
 wire [28:0] y2 ;
 wire [28:0] y7 ;
 wire [19:0] x3 ;
 wire [20:0] x4 ;
 wire [11:0] x0_9_1 ;
 wire [11:0] x0_9_2 ;
 wire [11:0] x0_9_3 ;
 wire [26:0] y4 ;
 wire [11:0] x0_10_1 ;
 wire [11:0] x0_10_2 ;
 wire [11:0] x0_10_3 ;
 wire [26:0] y5 ;
 wire [24:0] x11 ;
 wire [27:0] x12 ;
 wire [27:0] x12_1 ;
 wire [28:0] x13 ;
 wire [28:0] x13_1 ;
 wire [28:0] x14 ;
 wire [28:0] x14_1 ;
 wire [28:0] x15 ;
 wire [28:0] x15_1 ;
 wire [28:0] x16 ;
 wire [28:0] x16_1 ;
 wire [28:0] x17 ;
 wire [28:0] x17_1 ;
 wire [28:0] x18 ;
 wire [28:0] x18_1 ;
 wire [28:0] x19 ;
 wire [28:0] x19_1 ;
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


 Adder21Bit Adder0(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 9'b0 },x1); 


 Subtractor22Bit Sub0(clk, { x1[21: 0] },  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0], 3'b0 },x5); 

 Register23Bit Reg0 (clk,x5, x5_0_1 );
 Register25Bit Reg1 (clk, {x5_0_1, 2'b0}, y0 );
 Register23Bit Reg2 (clk,x5, x5_1_1 );
 Register25Bit Reg3 (clk, {x5_1_1, 2'b0}, y9 );

 Subtractor24Bit Sub1(clk, {x1[21],x1[21], x1[21: 0] },  { x1[21: 0], 2'b0 },x6); 


 Subtractor14Bit Sub2(clk, {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x2); 


 Subtractor27Bit Sub3(clk, {x1[21],x1[21],x1[21],x1[21],x1[21], x1[21: 0] },  { x2[14: 0], 12'b0 },x7); 

 Register28Bit Reg4 (clk,x7, x7_0_1 );
 Register29Bit Reg5 (clk, {x7_0_1, 1'b0}, y3 );
 Register28Bit Reg6 (clk,x7, x7_1_1 );
 Register29Bit Reg7 (clk, {x7_1_1, 1'b0}, y6 );

 Subtractor19Bit Sub4(clk, { x0[11: 0], 7'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x3); 


 Adder24Bit Adder1(clk,  {x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] },  { x3[19: 0], 4'b0 },x8); 


 Adder26Bit Adder2(clk,  { x3[19: 0], 6'b0 }, {x6[24], x6[24: 0]  }, x9); 

 Register27Bit Reg8 (clk,x9, y1 );
 Register27Bit Reg9 (clk,x9, y8 );

 Adder20Bit Adder3(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 8'b0 },x4); 


 Subtractor27Bit Sub5(clk, { x8[24: 0], 2'b0 }, {x4[20],x4[20],x4[20],x4[20],x4[20],x4[20], x4[20: 0] }, x10); 

 Register29Bit Reg10 (clk, {x10, 1'b0}, y2 );
 Register29Bit Reg11 (clk, {x10, 1'b0}, y7 );
 Register12Bit Reg12 (clk,x0, x0_9_1 );
 Register12Bit Reg13 (clk,x0_9_1, x0_9_2 );
 Register12Bit Reg14 (clk,x0_9_2, x0_9_3 );
 Register27Bit Reg15 (clk, {x0_9_3, 15'b0}, y4 );
 Register12Bit Reg16 (clk,x0, x0_10_1 );
 Register12Bit Reg17 (clk,x0_10_1, x0_10_2 );
 Register12Bit Reg18 (clk,x0_10_2, x0_10_3 );
 Register27Bit Reg19 (clk, {x0_10_3, 15'b0}, y5 );
 Register25Bit Reg20 (clk,y0, x11 );

 Adder27Bit Adder4(clk,  {x11[24],x11[24], x11[24: 0] },  { y1[26: 0]  }, x12); 

 Register28Bit Reg21 (clk,x12, x12_1 );

 Adder29Bit Adder5(clk,  {x12_1[27], x12_1[27: 0] },  { y2[28: 0]  }, x13); 

 Register29Bit Reg22 (clk,x13, x13_1 );

 Adder29Bit Adder6(clk,  { x13_1[28: 0] },  { y3[28: 0]  }, x14); 

 Register29Bit Reg23 (clk,x14, x14_1 );

 Adder29Bit Adder7(clk,  { x14_1[28: 0] },  {y4[26],y4[26], y4[26: 0]  }, x15); 

 Register29Bit Reg24 (clk,x15, x15_1 );

 Adder29Bit Adder8(clk,  { x15_1[28: 0] },  {y5[26],y5[26], y5[26: 0]  }, x16); 

 Register29Bit Reg25 (clk,x16, x16_1 );

 Adder29Bit Adder9(clk,  { x16_1[28: 0] },  { y6[28: 0]  }, x17); 

 Register29Bit Reg26 (clk,x17, x17_1 );

 Adder29Bit Adder10(clk,  { x17_1[28: 0] },  { y7[28: 0]  }, x18); 

 Register29Bit Reg27 (clk,x18, x18_1 );

 Adder29Bit Adder11(clk,  { x18_1[28: 0] },  {y8[26],y8[26], y8[26: 0]  }, x19); 

 Register29Bit Reg28 (clk,x19, x19_1 );

 Adder29Bit Adder12(clk,  { x19_1[28: 0] },  {y9[24],y9[24],y9[24],y9[24], y9[24: 0]  }, Out_Y_wire); 

 endmodule 



module Adder21Bit (clk, In1, In2, AddOut);
 input clk;
 input [20:0] In1, In2; 

 output [21:0] AddOut;

 reg [21 :0] AddOut; 

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



module Subtractor22Bit (clk, In1, In2, SubOut);
 input clk;
 input [21:0] In1, In2;
 output [22 :0] SubOut;

 reg [22 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule

module Subtractor24Bit (clk, In1, In2, SubOut);
 input clk;
 input [23:0] In1, In2;
 output [24 :0] SubOut;

 reg [24 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
end
endmodule

module Subtractor27Bit (clk, In1, In2, SubOut);
 input clk;
 input [26:0] In1, In2;
 output [27 :0] SubOut;

 reg [27 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
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


