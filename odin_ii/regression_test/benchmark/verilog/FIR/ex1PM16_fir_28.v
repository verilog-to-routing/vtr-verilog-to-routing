
 module ex1PM16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [17:0] x1 ;
 wire [18:0] x9 ;
 wire [23:0] x20 ;
 wire [27:0] y11 ;
 wire [27:0] y16 ;
 wire [18:0] x10 ;
 wire [18:0] x26 ;
 wire [23:0] y4 ;
 wire [23:0] y23 ;
 wire [20:0] x11 ;
 wire [22:0] x18 ;
 wire [25:0] y7 ;
 wire [25:0] y20 ;
 wire [21:0] x13 ;
 wire [21:0] x13_0_1 ;
 wire [22:0] y0 ;
 wire [21:0] x13_1_1 ;
 wire [22:0] y27 ;
 wire [20:0] x14 ;
 wire [25:0] x23 ;
 wire [25:0] y9 ;
 wire [25:0] y18 ;
 wire [24:0] x15 ;
 wire [24:0] x15_0_1 ;
 wire [24:0] y5 ;
 wire [24:0] x15_1_1 ;
 wire [24:0] y22 ;
 wire [24:0] x16 ;
 wire [24:0] x16_0_1 ;
 wire [25:0] y3 ;
 wire [24:0] x16_1_1 ;
 wire [25:0] y24 ;
 wire [22:0] x17 ;
 wire [22:0] x17_0_1 ;
 wire [24:0] y10 ;
 wire [22:0] x17_1_1 ;
 wire [24:0] y17 ;
 wire [14:0] x2 ;
 wire [16:0] x5 ;
 wire [21:0] x19 ;
 wire [24:0] y2 ;
 wire [24:0] y25 ;
 wire [25:0] x21 ;
 wire [26:0] y12 ;
 wire [26:0] y15 ;
 wire [21:0] x24 ;
 wire [21:0] y1 ;
 wire [21:0] x25 ;
 wire [21:0] y26 ;
 wire [16:0] x3 ;
 wire [23:0] x22 ;
 wire [23:0] x22_0_1 ;
 wire [23:0] y8 ;
 wire [23:0] x22_1_1 ;
 wire [23:0] y19 ;
 wire [21:0] x4 ;
 wire [15:0] x6 ;
 wire [20:0] x7 ;
 wire [18:0] x8 ;
 wire [19:0] x12 ;
 wire [19:0] x12_0_1 ;
 wire [20:0] y6 ;
 wire [19:0] x12_1_1 ;
 wire [20:0] y21 ;
 wire [11:0] x24_19_1 ;
 wire [11:0] x24_19_2 ;
 wire [11:0] x25_20_1 ;
 wire [11:0] x25_20_2 ;
 wire [11:0] x0_21_1 ;
 wire [11:0] x0_21_2 ;
 wire [11:0] x0_21_3 ;
 wire [26:0] y13 ;
 wire [11:0] x0_22_1 ;
 wire [11:0] x0_22_2 ;
 wire [11:0] x0_22_3 ;
 wire [26:0] y14 ;
 wire [22:0] x27 ;
 wire [23:0] x28 ;
 wire [23:0] x28_1 ;
 wire [25:0] x29 ;
 wire [25:0] x29_1 ;
 wire [26:0] x30 ;
 wire [26:0] x30_1 ;
 wire [27:0] x31 ;
 wire [27:0] x31_1 ;
 wire [28:0] x32 ;
 wire [28:0] x32_1 ;
 wire [28:0] x33 ;
 wire [28:0] x33_1 ;
 wire [28:0] x34 ;
 wire [28:0] x34_1 ;
 wire [28:0] x35 ;
 wire [28:0] x35_1 ;
 wire [28:0] x36 ;
 wire [28:0] x36_1 ;
 wire [28:0] x37 ;
 wire [28:0] x37_1 ;
 wire [28:0] x38 ;
 wire [28:0] x38_1 ;
 wire [28:0] x39 ;
 wire [28:0] x39_1 ;
 wire [28:0] x40 ;
 wire [28:0] x40_1 ;
 wire [28:0] x41 ;
 wire [28:0] x41_1 ;
 wire [28:0] x42 ;
 wire [28:0] x42_1 ;
 wire [28:0] x43 ;
 wire [28:0] x43_1 ;
 wire [28:0] x44 ;
 wire [28:0] x44_1 ;
 wire [28:0] x45 ;
 wire [28:0] x45_1 ;
 wire [28:0] x46 ;
 wire [28:0] x46_1 ;
 wire [28:0] x47 ;
 wire [28:0] x47_1 ;
 wire [28:0] x48 ;
 wire [28:0] x48_1 ;
 wire [28:0] x49 ;
 wire [28:0] x49_1 ;
 wire [28:0] x50 ;
 wire [28:0] x50_1 ;
 wire [28:0] x51 ;
 wire [28:0] x51_1 ;
 wire [28:0] x52 ;
 wire [28:0] x52_1 ;
 wire [28:0] x53 ;
 wire [28:0] x53_1 ;
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


 Adder17Bit Adder0(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 5'b0 },x1); 


 Subtractor18Bit Sub0(clk, {x0[11],x0[11],x0[11],x0[11], x0[11: 0], 2'b0 }, { x1[17: 0] }, x9); 


 Adder18Bit Adder1(clk,  {x0[11],x0[11],x0[11],x0[11], x0[11: 0], 2'b0 }, { x1[17: 0]  }, x10); 

 Negator19Bit Neg0(clk, x10, x26 );
 Register24Bit Reg0 (clk, {x26, 5'b0}, y4 );
 Register24Bit Reg1 (clk, {x26, 5'b0}, y23 );

 Subtractor20Bit Sub1(clk, { x1[17: 0], 2'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x11); 


 Subtractor22Bit Sub2(clk, {x11[20], x11[20: 0] },  { x1[17: 0], 4'b0 },x18); 

 Register26Bit Reg2 (clk, {x18, 3'b0}, y7 );
 Register26Bit Reg3 (clk, {x18, 3'b0}, y20 );

 Adder21Bit Adder2(clk,  {x1[17],x1[17],x1[17], x1[17: 0] },  { x1[17: 0], 3'b0 },x13); 

 Register22Bit Reg4 (clk,x13, x13_0_1 );
 Register23Bit Reg5 (clk, {x13_0_1, 1'b0}, y0 );
 Register22Bit Reg6 (clk,x13, x13_1_1 );
 Register23Bit Reg7 (clk, {x13_1_1, 1'b0}, y27 );

 Adder20Bit Adder3(clk,  {x1[17],x1[17], x1[17: 0] },  { x1[17: 0], 2'b0 },x14); 


 Adder14Bit Adder4(clk,  {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x2); 


 Subtractor16Bit Sub3(clk, { x0[11: 0], 4'b0 }, {x2[14], x2[14: 0] }, x5); 


 Subtractor21Bit Sub4(clk, {x5[16],x5[16],x5[16],x5[16], x5[16: 0] },  { x2[14: 0], 6'b0 },x19); 

 Register25Bit Reg8 (clk, {x19, 3'b0}, y2 );
 Register25Bit Reg9 (clk, {x19, 3'b0}, y25 );

 Subtractor23Bit Sub5(clk, { x9[18: 0], 4'b0 }, {x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] }, x20); 

 Register28Bit Reg10 (clk, {x20, 4'b0}, y11 );
 Register28Bit Reg11 (clk, {x20, 4'b0}, y16 );

 Adder16Bit Adder5(clk,  {x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 4'b0 },x3); 


 Adder24Bit Adder6(clk,  {x1[17],x1[17],x1[17],x1[17],x1[17],x1[17], x1[17: 0] },  { x3[16: 0], 7'b0 },x15); 

 Register25Bit Reg12 (clk,x15, x15_0_1 );
 Register25Bit Reg13 (clk,x15_0_1, y5 );
 Register25Bit Reg14 (clk,x15, x15_1_1 );
 Register25Bit Reg15 (clk,x15_1_1, y22 );

 Subtractor25Bit Sub6(clk, { x3[16: 0], 8'b0 }, {x5[16],x5[16],x5[16],x5[16],x5[16],x5[16],x5[16],x5[16], x5[16: 0] }, x21); 

 Register27Bit Reg16 (clk, {x21, 1'b0}, y12 );
 Register27Bit Reg17 (clk, {x21, 1'b0}, y15 );

 Subtractor21Bit Sub7(clk, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 9'b0 },x4); 


 Subtractor24Bit Sub8(clk, { x4[21: 0], 2'b0 }, {x1[17],x1[17],x1[17],x1[17],x1[17],x1[17], x1[17: 0] }, x16); 

 Register25Bit Reg18 (clk,x16, x16_0_1 );
 Register26Bit Reg19 (clk, {x16_0_1, 1'b0}, y3 );
 Register25Bit Reg20 (clk,x16, x16_1_1 );
 Register26Bit Reg21 (clk, {x16_1_1, 1'b0}, y24 );

 Subtractor22Bit Sub9(clk, { x4[21: 0] },  {x1[17],x1[17], x1[17: 0], 2'b0 },x17); 

 Register23Bit Reg22 (clk,x17, x17_0_1 );
 Register25Bit Reg23 (clk, {x17_0_1, 2'b0}, y10 );
 Register23Bit Reg24 (clk,x17, x17_1_1 );
 Register25Bit Reg25 (clk, {x17_1_1, 2'b0}, y17 );

 Adder15Bit Adder7(clk,  {x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 3'b0 },x6); 


 Subtractor25Bit Sub10(clk, { x6[15: 0], 9'b0 }, {x14[20],x14[20],x14[20],x14[20], x14[20: 0] }, x23); 

 Register26Bit Reg26 (clk,x23, y9 );
 Register26Bit Reg27 (clk,x23, y18 );

 Adder20Bit Adder8(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 8'b0 },x7); 


 Subtractor23Bit Sub11(clk, { x3[16: 0], 6'b0 }, {x7[20],x7[20], x7[20: 0] }, x22); 

 Register24Bit Reg28 (clk,x22, x22_0_1 );
 Register24Bit Reg29 (clk,x22_0_1, y8 );
 Register24Bit Reg30 (clk,x22, x22_1_1 );
 Register24Bit Reg31 (clk,x22_1_1, y19 );

 Adder18Bit Adder9(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 6'b0 },x8); 


 Subtractor19Bit Sub12(clk, {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0], 2'b0 }, { x8[18: 0] }, x12); 

 Register20Bit Reg32 (clk,x12, x12_0_1 );
 Register21Bit Reg33 (clk, {x12_0_1, 1'b0}, y6 );
 Register20Bit Reg34 (clk,x12, x12_1_1 );
 Register21Bit Reg35 (clk, {x12_1_1, 1'b0}, y21 );
 Register12Bit Reg36 (clk,x0, x24_19_1 );
 Register12Bit Reg37 (clk,x24_19_1, x24_19_2 );

 Subtractor21Bit Sub13(clk, { x5[16: 0], 4'b0 }, {x24_19_2[11],x24_19_2[11],x24_19_2[11],x24_19_2[11],x24_19_2[11],x24_19_2[11],x24_19_2[11],x24_19_2[11],x24_19_2[11], x24_19_2[11: 0] }, x24); 

 Register22Bit Reg38 (clk,x24, y1 );
 Register12Bit Reg39 (clk,x0, x25_20_1 );
 Register12Bit Reg40 (clk,x25_20_1, x25_20_2 );

 Subtractor21Bit Sub14(clk, { x5[16: 0], 4'b0 }, {x25_20_2[11],x25_20_2[11],x25_20_2[11],x25_20_2[11],x25_20_2[11],x25_20_2[11],x25_20_2[11],x25_20_2[11],x25_20_2[11], x25_20_2[11: 0] }, x25); 

 Register22Bit Reg41 (clk,x25, y26 );
 Register12Bit Reg42 (clk,x0, x0_21_1 );
 Register12Bit Reg43 (clk,x0_21_1, x0_21_2 );
 Register12Bit Reg44 (clk,x0_21_2, x0_21_3 );
 Register27Bit Reg45 (clk, {x0_21_3, 15'b0}, y13 );
 Register12Bit Reg46 (clk,x0, x0_22_1 );
 Register12Bit Reg47 (clk,x0_22_1, x0_22_2 );
 Register12Bit Reg48 (clk,x0_22_2, x0_22_3 );
 Register27Bit Reg49 (clk, {x0_22_3, 15'b0}, y14 );
 Register23Bit Reg50 (clk,y0, x27 );

 Adder23Bit Adder10(clk,  { x27[22: 0] },  {y1[21], y1[21: 0]  }, x28); 

 Register24Bit Reg51 (clk,x28, x28_1 );

 Adder25Bit Adder11(clk,  {x28_1[23], x28_1[23: 0] },  { y2[24: 0]  }, x29); 

 Register26Bit Reg52 (clk,x29, x29_1 );

 Adder26Bit Adder12(clk,  { x29_1[25: 0] },  { y3[25: 0]  }, x30); 

 Register27Bit Reg53 (clk,x30, x30_1 );

 Adder27Bit Adder13(clk,  { x30_1[26: 0] },  {y4[23],y4[23],y4[23], y4[23: 0]  }, x31); 

 Register28Bit Reg54 (clk,x31, x31_1 );

 Adder28Bit Adder14(clk,  { x31_1[27: 0] },  {y5[24],y5[24],y5[24], y5[24: 0]  }, x32); 

 Register29Bit Reg55 (clk,x32, x32_1 );

 Adder29Bit Adder15(clk,  { x32_1[28: 0] },  {y6[20],y6[20],y6[20],y6[20],y6[20],y6[20],y6[20],y6[20], y6[20: 0]  }, x33); 

 Register29Bit Reg56 (clk,x33, x33_1 );

 Adder29Bit Adder16(clk,  { x33_1[28: 0] },  {y7[25],y7[25],y7[25], y7[25: 0]  }, x34); 

 Register29Bit Reg57 (clk,x34, x34_1 );

 Adder29Bit Adder17(clk,  { x34_1[28: 0] },  {y8[23],y8[23],y8[23],y8[23],y8[23], y8[23: 0]  }, x35); 

 Register29Bit Reg58 (clk,x35, x35_1 );

 Adder29Bit Adder18(clk,  { x35_1[28: 0] },  {y9[25],y9[25],y9[25], y9[25: 0]  }, x36); 

 Register29Bit Reg59 (clk,x36, x36_1 );

 Adder29Bit Adder19(clk,  { x36_1[28: 0] },  {y10[24],y10[24],y10[24],y10[24], y10[24: 0]  }, x37); 

 Register29Bit Reg60 (clk,x37, x37_1 );

 Adder29Bit Adder20(clk,  { x37_1[28: 0] },  {y11[27], y11[27: 0]  }, x38); 

 Register29Bit Reg61 (clk,x38, x38_1 );

 Adder29Bit Adder21(clk,  { x38_1[28: 0] },  {y12[26],y12[26], y12[26: 0]  }, x39); 

 Register29Bit Reg62 (clk,x39, x39_1 );

 Adder29Bit Adder22(clk,  { x39_1[28: 0] },  {y13[26],y13[26], y13[26: 0]  }, x40); 

 Register29Bit Reg63 (clk,x40, x40_1 );

 Adder29Bit Adder23(clk,  { x40_1[28: 0] },  {y14[26],y14[26], y14[26: 0]  }, x41); 

 Register29Bit Reg64 (clk,x41, x41_1 );

 Adder29Bit Adder24(clk,  { x41_1[28: 0] },  {y15[26],y15[26], y15[26: 0]  }, x42); 

 Register29Bit Reg65 (clk,x42, x42_1 );

 Adder29Bit Adder25(clk,  { x42_1[28: 0] },  {y16[27], y16[27: 0]  }, x43); 

 Register29Bit Reg66 (clk,x43, x43_1 );

 Adder29Bit Adder26(clk,  { x43_1[28: 0] },  {y17[24],y17[24],y17[24],y17[24], y17[24: 0]  }, x44); 

 Register29Bit Reg67 (clk,x44, x44_1 );

 Adder29Bit Adder27(clk,  { x44_1[28: 0] },  {y18[25],y18[25],y18[25], y18[25: 0]  }, x45); 

 Register29Bit Reg68 (clk,x45, x45_1 );

 Adder29Bit Adder28(clk,  { x45_1[28: 0] },  {y19[23],y19[23],y19[23],y19[23],y19[23], y19[23: 0]  }, x46); 

 Register29Bit Reg69 (clk,x46, x46_1 );

 Adder29Bit Adder29(clk,  { x46_1[28: 0] },  {y20[25],y20[25],y20[25], y20[25: 0]  }, x47); 

 Register29Bit Reg70 (clk,x47, x47_1 );

 Adder29Bit Adder30(clk,  { x47_1[28: 0] },  {y21[20],y21[20],y21[20],y21[20],y21[20],y21[20],y21[20],y21[20], y21[20: 0]  }, x48); 

 Register29Bit Reg71 (clk,x48, x48_1 );

 Adder29Bit Adder31(clk,  { x48_1[28: 0] },  {y22[24],y22[24],y22[24],y22[24], y22[24: 0]  }, x49); 

 Register29Bit Reg72 (clk,x49, x49_1 );

 Adder29Bit Adder32(clk,  { x49_1[28: 0] },  {y23[23],y23[23],y23[23],y23[23],y23[23], y23[23: 0]  }, x50); 

 Register29Bit Reg73 (clk,x50, x50_1 );

 Adder29Bit Adder33(clk,  { x50_1[28: 0] },  {y24[25],y24[25],y24[25], y24[25: 0]  }, x51); 

 Register29Bit Reg74 (clk,x51, x51_1 );

 Adder29Bit Adder34(clk,  { x51_1[28: 0] },  {y25[24],y25[24],y25[24],y25[24], y25[24: 0]  }, x52); 

 Register29Bit Reg75 (clk,x52, x52_1 );

 Adder29Bit Adder35(clk,  { x52_1[28: 0] },  {y26[21],y26[21],y26[21],y26[21],y26[21],y26[21],y26[21], y26[21: 0]  }, x53); 

 Register29Bit Reg76 (clk,x53, x53_1 );

 Adder29Bit Adder36(clk,  { x53_1[28: 0] },  {y27[22],y27[22],y27[22],y27[22],y27[22],y27[22], y27[22: 0]  }, Out_Y_wire); 

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

module Adder15Bit (clk, In1, In2, AddOut);
 input clk;
 input [14:0] In1, In2; 

 output [15:0] AddOut;

 reg [15 :0] AddOut; 

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

module Subtractor20Bit (clk, In1, In2, SubOut);
 input clk;
 input [19:0] In1, In2;
 output [20 :0] SubOut;

 reg [20 :0] SubOut; 

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

module Subtractor25Bit (clk, In1, In2, SubOut);
 input clk;
 input [24:0] In1, In2;
 output [25 :0] SubOut;

 reg [25 :0] SubOut; 

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

module Register22Bit (clk, In1, RegOut);
 input clk;
 input [21:0] In1;
 output [21 :0] RegOut;

 reg [21 :0] RegOut; 

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



module Negator19Bit (clk, In1, NegOut);
 input clk;
 input [18:0] In1;
 output [18 :0] NegOut;

 reg [18 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule
