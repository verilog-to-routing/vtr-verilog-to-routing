
 module ex1BT16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [14:0] x1 ;
 wire [19:0] x2 ;
 wire [27:0] x11 ;
 wire [27:0] y7 ;
 wire [27:0] y12 ;
 wire [23:0] x12 ;
 wire [23:0] y3 ;
 wire [23:0] y16 ;
 wire [24:0] x13 ;
 wire [25:0] y4 ;
 wire [25:0] y15 ;
 wire [17:0] x4 ;
 wire [28:0] x15 ;
 wire [29:0] y8 ;
 wire [29:0] y11 ;
 wire [19:0] x9 ;
 wire [23:0] x14 ;
 wire [26:0] y6 ;
 wire [26:0] y13 ;
 wire [27:0] x10 ;
 wire [14:0] x1_6_1 ;
 wire [14:0] x1_6_2 ;
 wire [15:0] y1 ;
 wire [14:0] x1_7_1 ;
 wire [14:0] x1_7_2 ;
 wire [15:0] y18 ;
 wire [22:0] x3 ;
 wire [23:0] x8 ;
 wire [23:0] x8_0_1 ;
 wire [25:0] y5 ;
 wire [23:0] x8_1_1 ;
 wire [25:0] y14 ;
 wire [16:0] x5 ;
 wire [16:0] x5_0_1 ;
 wire [16:0] x5_0_2 ;
 wire [18:0] y2 ;
 wire [16:0] x5_1_1 ;
 wire [16:0] x5_1_2 ;
 wire [18:0] y17 ;
 wire [17:0] x6 ;
 wire [18:0] x7 ;
 wire [11:0] x0_13_1 ;
 wire [11:0] x0_13_2 ;
 wire [11:0] x0_13_3 ;
 wire [26:0] y9 ;
 wire [11:0] x0_14_1 ;
 wire [11:0] x0_14_2 ;
 wire [11:0] x0_14_3 ;
 wire [26:0] y10 ;
 wire [11:0] x16 ;
 wire [16:0] x17 ;
 wire [16:0] x17_1 ;
 wire [19:0] x18 ;
 wire [19:0] x18_1 ;
 wire [24:0] x19 ;
 wire [24:0] x19_1 ;
 wire [26:0] x20 ;
 wire [26:0] x20_1 ;
 wire [27:0] x21 ;
 wire [27:0] x21_1 ;
 wire [28:0] x22 ;
 wire [28:0] x22_1 ;
 wire [28:0] x23 ;
 wire [28:0] x23_1 ;
 wire [28:0] x24 ;
 wire [28:0] x24_1 ;
 wire [28:0] x25 ;
 wire [28:0] x25_1 ;
 wire [28:0] x26 ;
 wire [28:0] x26_1 ;
 wire [28:0] x27 ;
 wire [28:0] x27_1 ;
 wire [28:0] x28 ;
 wire [28:0] x28_1 ;
 wire [28:0] x29 ;
 wire [28:0] x29_1 ;
 wire [28:0] x30 ;
 wire [28:0] x30_1 ;
 wire [28:0] x31 ;
 wire [28:0] x31_1 ;
 wire [28:0] x32 ;
 wire [28:0] x32_1 ;
 wire [28:0] x33 ;
 wire [28:0] x33_1 ;
 wire [28:0] x34 ;
 wire [28:0] x34_1 ;
 wire [28:0] Out_Y_wire ;
 wire [11:0] y0 ;
 wire [11:0] y19 ;
 reg [11:0] Out_Y;

 reg [11:0] ZeroReg; 

 always @(posedge clk) 
 begin
 x0 <= In_X; 
 ZeroReg <= 12'b0 ;
 end 


 always @(posedge clk) 
 begin
 Out_Y = Out_Y_wire;
 end

 assign y0 = ZeroReg; 
 assign y19 = ZeroReg; 

 Subtractor14Bit Sub0(clk, { x0[11: 0], 2'b0 }, {x0[11],x0[11], x0[11: 0] }, x1); 


 Subtractor19Bit Sub1(clk, { x1[14: 0], 4'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x2); 


 Adder17Bit Adder0(clk,  { x0[11: 0], 5'b0 }, {x1[14],x1[14], x1[14: 0]  }, x4); 


 Subtractor27Bit Sub2(clk, { x4[17: 0], 9'b0 }, {x2[19],x2[19],x2[19],x2[19],x2[19],x2[19],x2[19], x2[19: 0] }, x11); 

 Register28Bit Reg0 (clk,x11, y7 );
 Register28Bit Reg1 (clk,x11, y12 );

 Subtractor19Bit Sub3(clk, { x1[14: 0], 4'b0 }, {x1[14],x1[14],x1[14],x1[14], x1[14: 0] }, x9); 


 Subtractor27Bit Sub4(clk, { x1[14: 0], 12'b0 }, {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] }, x10); 


 Adder28Bit Adder1(clk,  {x4[17],x4[17],x4[17],x4[17],x4[17], x4[17: 0], 5'b0 }, { x10[27: 0]  }, x15); 

 Register30Bit Reg2 (clk, {x15, 1'b0}, y8 );
 Register30Bit Reg3 (clk, {x15, 1'b0}, y11 );
 Register15Bit Reg4 (clk,x1, x1_6_1 );
 Register15Bit Reg5 (clk,x1_6_1, x1_6_2 );
 Register16Bit Reg6 (clk, {x1_6_2, 1'b0}, y1 );
 Register15Bit Reg7 (clk,x1, x1_7_1 );
 Register15Bit Reg8 (clk,x1_7_1, x1_7_2 );
 Register16Bit Reg9 (clk, {x1_7_2, 1'b0}, y18 );

 Subtractor22Bit Sub5(clk, { x0[11: 0], 10'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x3); 


 Adder23Bit Adder2(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0], 3'b0 }, { x3[22: 0]  }, x8); 

 Register24Bit Reg10 (clk,x8, x8_0_1 );
 Register26Bit Reg11 (clk, {x8_0_1, 2'b0}, y5 );
 Register24Bit Reg12 (clk,x8, x8_1_1 );
 Register26Bit Reg13 (clk, {x8_1_1, 2'b0}, y14 );

 Adder23Bit Adder3(clk,  { x3[22: 0] },  {x9[19], x9[19: 0], 2'b0 },x14); 

 Register27Bit Reg14 (clk, {x14, 3'b0}, y6 );
 Register27Bit Reg15 (clk, {x14, 3'b0}, y13 );

 Subtractor16Bit Sub6(clk, { x0[11: 0], 4'b0 }, {x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x5); 

 Register17Bit Reg16 (clk,x5, x5_0_1 );
 Register17Bit Reg17 (clk,x5_0_1, x5_0_2 );
 Register19Bit Reg18 (clk, {x5_0_2, 2'b0}, y2 );
 Register17Bit Reg19 (clk,x5, x5_1_1 );
 Register17Bit Reg20 (clk,x5_1_1, x5_1_2 );
 Register19Bit Reg21 (clk, {x5_1_2, 2'b0}, y17 );

 Adder17Bit Adder4(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 5'b0 },x6); 


 Subtractor23Bit Sub7(clk, { x2[19: 0], 3'b0 }, {x6[17],x6[17],x6[17],x6[17],x6[17], x6[17: 0] }, x12); 

 Register24Bit Reg22 (clk,x12, y3 );
 Register24Bit Reg23 (clk,x12, y16 );

 Adder18Bit Adder5(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 6'b0 },x7); 


 Subtractor24Bit Sub8(clk, { x2[19: 0], 4'b0 }, {x7[18],x7[18],x7[18],x7[18],x7[18], x7[18: 0] }, x13); 

 Register26Bit Reg24 (clk, {x13, 1'b0}, y4 );
 Register26Bit Reg25 (clk, {x13, 1'b0}, y15 );
 Register12Bit Reg26 (clk,x0, x0_13_1 );
 Register12Bit Reg27 (clk,x0_13_1, x0_13_2 );
 Register12Bit Reg28 (clk,x0_13_2, x0_13_3 );
 Register27Bit Reg29 (clk, {x0_13_3, 15'b0}, y9 );
 Register12Bit Reg30 (clk,x0, x0_14_1 );
 Register12Bit Reg31 (clk,x0_14_1, x0_14_2 );
 Register12Bit Reg32 (clk,x0_14_2, x0_14_3 );
 Register27Bit Reg33 (clk, {x0_14_3, 15'b0}, y10 );
 Register12Bit Reg34 (clk,y0, x16 );

 Adder16Bit Adder6(clk,  {x16[11],x16[11],x16[11],x16[11], x16[11: 0] },  { y1[15: 0]  }, x17); 

 Register17Bit Reg35 (clk,x17, x17_1 );

 Adder19Bit Adder7(clk,  {x17_1[16],x17_1[16], x17_1[16: 0] },  { y2[18: 0]  }, x18); 

 Register20Bit Reg36 (clk,x18, x18_1 );

 Adder24Bit Adder8(clk,  {x18_1[19],x18_1[19],x18_1[19],x18_1[19], x18_1[19: 0] },  { y3[23: 0]  }, x19); 

 Register25Bit Reg37 (clk,x19, x19_1 );

 Adder26Bit Adder9(clk,  {x19_1[24], x19_1[24: 0] },  { y4[25: 0]  }, x20); 

 Register27Bit Reg38 (clk,x20, x20_1 );

 Adder27Bit Adder10(clk,  { x20_1[26: 0] },  {y5[25], y5[25: 0]  }, x21); 

 Register28Bit Reg39 (clk,x21, x21_1 );

 Adder28Bit Adder11(clk,  { x21_1[27: 0] },  {y6[26], y6[26: 0]  }, x22); 

 Register29Bit Reg40 (clk,x22, x22_1 );

 Adder29Bit Adder12(clk,  { x22_1[28: 0] },  {y7[27], y7[27: 0]  }, x23); 

 Register29Bit Reg41 (clk,x23, x23_1 );

 Adder29Bit Adder13(clk,  { x23_1[28: 0] },  { y8[29:1] },x24); 

 Register29Bit Reg42 (clk,x24, x24_1 );

 Adder29Bit Adder14(clk,  { x24_1[28: 0] },  {y9[26],y9[26], y9[26: 0]  }, x25); 

 Register29Bit Reg43 (clk,x25, x25_1 );

 Adder29Bit Adder15(clk,  { x25_1[28: 0] },  {y10[26],y10[26], y10[26: 0]  }, x26); 

 Register29Bit Reg44 (clk,x26, x26_1 );

 Adder29Bit Adder16(clk,  { x26_1[28: 0] },  { y11[29:1] },x27); 

 Register29Bit Reg45 (clk,x27, x27_1 );

 Adder29Bit Adder17(clk,  { x27_1[28: 0] },  {y12[27], y12[27: 0]  }, x28); 

 Register29Bit Reg46 (clk,x28, x28_1 );

 Adder29Bit Adder18(clk,  { x28_1[28: 0] },  {y13[26],y13[26], y13[26: 0]  }, x29); 

 Register29Bit Reg47 (clk,x29, x29_1 );

 Adder29Bit Adder19(clk,  { x29_1[28: 0] },  {y14[25],y14[25],y14[25], y14[25: 0]  }, x30); 

 Register29Bit Reg48 (clk,x30, x30_1 );

 Adder29Bit Adder20(clk,  { x30_1[28: 0] },  {y15[25],y15[25],y15[25], y15[25: 0]  }, x31); 

 Register29Bit Reg49 (clk,x31, x31_1 );

 Adder29Bit Adder21(clk,  { x31_1[28: 0] },  {y16[23],y16[23],y16[23],y16[23],y16[23], y16[23: 0]  }, x32); 

 Register29Bit Reg50 (clk,x32, x32_1 );

 Adder29Bit Adder22(clk,  { x32_1[28: 0] },  {y17[18],y17[18],y17[18],y17[18],y17[18],y17[18],y17[18],y17[18],y17[18],y17[18], y17[18: 0]  }, x33); 

 Register29Bit Reg51 (clk,x33, x33_1 );

 Adder29Bit Adder23(clk,  { x33_1[28: 0] },  {y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15],y18[15], y18[15: 0]  }, x34); 

 Register29Bit Reg52 (clk,x34, x34_1 );

 Adder29Bit Adder24(clk,  { x34_1[28: 0] },  {y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11],y19[11], y19[11: 0]  }, Out_Y_wire); 

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

module Adder23Bit (clk, In1, In2, AddOut);
 input clk;
 input [22:0] In1, In2; 

 output [23:0] AddOut;

 reg [23 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder18Bit (clk, In1, In2, AddOut);
 input clk;
 input [17:0] In1, In2; 

 output [18:0] AddOut;

 reg [18 :0] AddOut; 

always @(posedge clk)
begin
AddOut <= (In1 + In2);
end
endmodule

module Adder16Bit (clk, In1, In2, AddOut);
 input clk;
 input [15:0] In1, In2; 

 output [16:0] AddOut;

 reg [16 :0] AddOut; 

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

module Subtractor23Bit (clk, In1, In2, SubOut);
 input clk;
 input [22:0] In1, In2;
 output [23 :0] SubOut;

 reg [23 :0] SubOut; 

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

module Register24Bit (clk, In1, RegOut);
 input clk;
 input [23:0] In1;
 output [23 :0] RegOut;

 reg [23 :0] RegOut; 

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

module Register30Bit (clk, In1, RegOut);
 input clk;
 input [29:0] In1;
 output [29 :0] RegOut;

 reg [29 :0] RegOut; 

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

module Register16Bit (clk, In1, RegOut);
 input clk;
 input [15:0] In1;
 output [15 :0] RegOut;

 reg [15 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register17Bit (clk, In1, RegOut);
 input clk;
 input [16:0] In1;
 output [16 :0] RegOut;

 reg [16 :0] RegOut; 

always @(posedge clk)
begin
RegOut <= (In1);
end
endmodule

module Register19Bit (clk, In1, RegOut);
 input clk;
 input [18:0] In1;
 output [18 :0] RegOut;

 reg [18 :0] RegOut; 

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

module Register20Bit (clk, In1, RegOut);
 input clk;
 input [19:0] In1;
 output [19 :0] RegOut;

 reg [19 :0] RegOut; 

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


