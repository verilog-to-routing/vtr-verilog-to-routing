
 module ex1LS16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [14:0] x1 ;
 wire [16:0] x7 ;
 wire [23:0] x25 ;
 wire [23:0] x25_0_1 ;
 wire [24:0] y18 ;
 wire [23:0] x25_1_1 ;
 wire [24:0] y22 ;
 wire [27:0] x33 ;
 wire [27:0] x33_0_1 ;
 wire [27:0] y17 ;
 wire [27:0] x33_1_1 ;
 wire [27:0] y23 ;
 wire [17:0] x8 ;
 wire [22:0] x26 ;
 wire [22:0] x26_0_1 ;
 wire [22:0] y11 ;
 wire [22:0] x26_1_1 ;
 wire [22:0] y29 ;
 wire [24:0] x27 ;
 wire [24:0] x27_0_1 ;
 wire [24:0] y16 ;
 wire [24:0] x27_1_1 ;
 wire [24:0] y24 ;
 wire [18:0] x13 ;
 wire [18:0] x13_0_1 ;
 wire [18:0] x13_0_2 ;
 wire [20:0] y1 ;
 wire [18:0] x13_1_1 ;
 wire [18:0] x13_1_2 ;
 wire [20:0] y39 ;
 wire [17:0] x14 ;
 wire [21:0] x29 ;
 wire [21:0] x29_0_1 ;
 wire [22:0] y6 ;
 wire [21:0] x29_1_1 ;
 wire [22:0] y34 ;
 wire [24:0] x15 ;
 wire [25:0] x31 ;
 wire [25:0] x31_0_1 ;
 wire [25:0] y13 ;
 wire [25:0] x31_1_1 ;
 wire [25:0] y27 ;
 wire [22:0] x17 ;
 wire [26:0] x32 ;
 wire [26:0] x39 ;
 wire [26:0] y15 ;
 wire [26:0] y25 ;
 wire [19:0] x18 ;
 wire [20:0] x19 ;
 wire [20:0] x19_0_1 ;
 wire [20:0] x19_0_2 ;
 wire [20:0] y0 ;
 wire [20:0] x19_1_1 ;
 wire [20:0] x19_1_2 ;
 wire [20:0] y40 ;
 wire [20:0] x20 ;
 wire [20:0] x20_0_1 ;
 wire [20:0] x20_0_2 ;
 wire [24:0] y12 ;
 wire [20:0] x20_1_1 ;
 wire [20:0] x20_1_2 ;
 wire [24:0] y28 ;
 wire [20:0] x21 ;
 wire [20:0] x21_0_1 ;
 wire [20:0] x21_0_2 ;
 wire [20:0] y3 ;
 wire [20:0] x21_1_1 ;
 wire [20:0] x21_1_2 ;
 wire [20:0] y37 ;
 wire [14:0] x2 ;
 wire [20:0] x22 ;
 wire [27:0] x34 ;
 wire [27:0] x34_0_1 ;
 wire [27:0] y19 ;
 wire [27:0] x34_1_1 ;
 wire [27:0] y21 ;
 wire [22:0] x23 ;
 wire [22:0] x38 ;
 wire [22:0] x38_0_1 ;
 wire [23:0] y10 ;
 wire [22:0] x38_1_1 ;
 wire [23:0] y30 ;
 wire [21:0] x24 ;
 wire [21:0] x24_0_1 ;
 wire [21:0] x24_0_2 ;
 wire [22:0] y4 ;
 wire [21:0] x24_1_1 ;
 wire [21:0] x24_1_2 ;
 wire [22:0] y36 ;
 wire [21:0] x28 ;
 wire [21:0] x28_0_1 ;
 wire [21:0] x28_0_2 ;
 wire [21:0] y9 ;
 wire [21:0] x28_1_1 ;
 wire [21:0] x28_1_2 ;
 wire [21:0] y31 ;
 wire [15:0] x3 ;
 wire [19:0] x16 ;
 wire [19:0] x16_0_1 ;
 wire [19:0] x16_0_2 ;
 wire [19:0] y5 ;
 wire [19:0] x16_1_1 ;
 wire [19:0] x16_1_2 ;
 wire [19:0] y35 ;
 wire [24:0] x30 ;
 wire [24:0] x30_0_1 ;
 wire [24:0] x30_0_2 ;
 wire [24:0] y14 ;
 wire [24:0] x30_1_1 ;
 wire [24:0] x30_1_2 ;
 wire [24:0] y26 ;
 wire [15:0] x4 ;
 wire [17:0] x5 ;
 wire [17:0] x36 ;
 wire [17:0] x36_0_1 ;
 wire [17:0] x36_0_2 ;
 wire [17:0] y7 ;
 wire [17:0] x36_1_1 ;
 wire [17:0] x36_1_2 ;
 wire [17:0] y33 ;
 wire [20:0] x6 ;
 wire [17:0] x9 ;
 wire [17:0] x35 ;
 wire [17:0] x35_0_1 ;
 wire [17:0] x35_0_2 ;
 wire [20:0] y2 ;
 wire [17:0] x35_1_1 ;
 wire [17:0] x35_1_2 ;
 wire [20:0] y38 ;
 wire [16:0] x10 ;
 wire [16:0] x37 ;
 wire [16:0] x37_0_1 ;
 wire [16:0] x37_0_2 ;
 wire [22:0] y8 ;
 wire [16:0] x37_1_1 ;
 wire [16:0] x37_1_2 ;
 wire [22:0] y32 ;
 wire [20:0] x11 ;
 wire [26:0] x12 ;
 wire [11:0] x0_26_1 ;
 wire [11:0] x0_26_2 ;
 wire [11:0] x0_26_3 ;
 wire [11:0] x0_26_4 ;
 wire [26:0] y20 ;
 wire [20:0] x40 ;
 wire [21:0] x41 ;
 wire [21:0] x41_1 ;
 wire [22:0] x42 ;
 wire [22:0] x42_1 ;
 wire [23:0] x43 ;
 wire [23:0] x43_1 ;
 wire [24:0] x44 ;
 wire [24:0] x44_1 ;
 wire [25:0] x45 ;
 wire [25:0] x45_1 ;
 wire [26:0] x46 ;
 wire [26:0] x46_1 ;
 wire [27:0] x47 ;
 wire [27:0] x47_1 ;
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
 wire [28:0] x54 ;
 wire [28:0] x54_1 ;
 wire [28:0] x55 ;
 wire [28:0] x55_1 ;
 wire [28:0] x56 ;
 wire [28:0] x56_1 ;
 wire [28:0] x57 ;
 wire [28:0] x57_1 ;
 wire [28:0] x58 ;
 wire [28:0] x58_1 ;
 wire [28:0] x59 ;
 wire [28:0] x59_1 ;
 wire [28:0] x60 ;
 wire [28:0] x60_1 ;
 wire [28:0] x61 ;
 wire [28:0] x61_1 ;
 wire [28:0] x62 ;
 wire [28:0] x62_1 ;
 wire [28:0] x63 ;
 wire [28:0] x63_1 ;
 wire [28:0] x64 ;
 wire [28:0] x64_1 ;
 wire [28:0] x65 ;
 wire [28:0] x65_1 ;
 wire [28:0] x66 ;
 wire [28:0] x66_1 ;
 wire [28:0] x67 ;
 wire [28:0] x67_1 ;
 wire [28:0] x68 ;
 wire [28:0] x68_1 ;
 wire [28:0] x69 ;
 wire [28:0] x69_1 ;
 wire [28:0] x70 ;
 wire [28:0] x70_1 ;
 wire [28:0] x71 ;
 wire [28:0] x71_1 ;
 wire [28:0] x72 ;
 wire [28:0] x72_1 ;
 wire [28:0] x73 ;
 wire [28:0] x73_1 ;
 wire [28:0] x74 ;
 wire [28:0] x74_1 ;
 wire [28:0] x75 ;
 wire [28:0] x75_1 ;
 wire [28:0] x76 ;
 wire [28:0] x76_1 ;
 wire [28:0] x77 ;
 wire [28:0] x77_1 ;
 wire [28:0] x78 ;
 wire [28:0] x78_1 ;
 wire [28:0] x79 ;
 wire [28:0] x79_1 ;
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


 Subtractor14Bit Sub0(clk, {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x1); 


 Subtractor16Bit Sub1(clk, {x1[14], x1[14: 0] },  { x0[11: 0], 4'b0 },x7); 


 Subtractor17Bit Sub2(clk, { x0[11: 0], 5'b0 }, {x1[14],x1[14], x1[14: 0] }, x8); 


 Subtractor18Bit Sub3(clk, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 3'b0 },x13); 

 Register19Bit Reg0 (clk,x13, x13_0_1 );
 Register19Bit Reg1 (clk,x13_0_1, x13_0_2 );
 Register21Bit Reg2 (clk, {x13_0_2, 2'b0}, y1 );
 Register19Bit Reg3 (clk,x13, x13_1_1 );
 Register19Bit Reg4 (clk,x13_1_1, x13_1_2 );
 Register21Bit Reg5 (clk, {x13_1_2, 2'b0}, y39 );

 Subtractor17Bit Sub4(clk, { x1[14: 0], 2'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x14); 


 Adder24Bit Adder0(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 9'b0 },x15); 


 Adder22Bit Adder1(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x1[14: 0], 7'b0 },x17); 


 Subtractor19Bit Sub5(clk, { x1[14: 0], 4'b0 }, {x1[14],x1[14],x1[14],x1[14], x1[14: 0] }, x18); 


 Adder27Bit Adder2(clk,  {x7[16],x7[16],x7[16],x7[16],x7[16],x7[16],x7[16],x7[16],x7[16],x7[16], x7[16: 0] },  { x18[19: 0], 7'b0 },x33); 

 Register28Bit Reg6 (clk,x33, x33_0_1 );
 Register28Bit Reg7 (clk,x33_0_1, y17 );
 Register28Bit Reg8 (clk,x33, x33_1_1 );
 Register28Bit Reg9 (clk,x33_1_1, y23 );

 Adder14Bit Adder3(clk,  {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x2); 


 Adder20Bit Adder4(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x2[14: 0], 5'b0 },x19); 

 Register21Bit Reg10 (clk,x19, x19_0_1 );
 Register21Bit Reg11 (clk,x19_0_1, x19_0_2 );
 Register21Bit Reg12 (clk,x19_0_2, y0 );
 Register21Bit Reg13 (clk,x19, x19_1_1 );
 Register21Bit Reg14 (clk,x19_1_1, x19_1_2 );
 Register21Bit Reg15 (clk,x19_1_2, y40 );

 Adder23Bit Adder5(clk,  {x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] },  { x7[16: 0], 6'b0 },x25); 

 Register24Bit Reg16 (clk,x25, x25_0_1 );
 Register25Bit Reg17 (clk, {x25_0_1, 1'b0}, y18 );
 Register24Bit Reg18 (clk,x25, x25_1_1 );
 Register25Bit Reg19 (clk, {x25_1_1, 1'b0}, y22 );

 Adder22Bit Adder6(clk,  { x2[14: 0], 7'b0 }, {x8[17],x8[17],x8[17],x8[17], x8[17: 0]  }, x26); 

 Register23Bit Reg20 (clk,x26, x26_0_1 );
 Register23Bit Reg21 (clk,x26_0_1, y11 );
 Register23Bit Reg22 (clk,x26, x26_1_1 );
 Register23Bit Reg23 (clk,x26_1_1, y29 );

 Adder24Bit Adder7(clk,  {x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] },  { x8[17: 0], 6'b0 },x27); 

 Register25Bit Reg24 (clk,x27, x27_0_1 );
 Register25Bit Reg25 (clk,x27_0_1, y16 );
 Register25Bit Reg26 (clk,x27, x27_1_1 );
 Register25Bit Reg27 (clk,x27_1_1, y24 );

 Subtractor21Bit Sub6(clk, {x14[17],x14[17],x14[17], x14[17: 0] },  { x2[14: 0], 6'b0 },x29); 

 Register22Bit Reg28 (clk,x29, x29_0_1 );
 Register23Bit Reg29 (clk, {x29_0_1, 1'b0}, y6 );
 Register22Bit Reg30 (clk,x29, x29_1_1 );
 Register23Bit Reg31 (clk, {x29_1_1, 1'b0}, y34 );

 Subtractor15Bit Sub7(clk, { x0[11: 0], 3'b0 }, {x0[11],x0[11],x0[11], x0[11: 0] }, x3); 


 Adder19Bit Adder8(clk,  { x0[11: 0], 7'b0 }, {x3[15],x3[15],x3[15], x3[15: 0]  }, x16); 

 Register20Bit Reg32 (clk,x16, x16_0_1 );
 Register20Bit Reg33 (clk,x16_0_1, x16_0_2 );
 Register20Bit Reg34 (clk,x16_0_2, y5 );
 Register20Bit Reg35 (clk,x16, x16_1_1 );
 Register20Bit Reg36 (clk,x16_1_1, x16_1_2 );
 Register20Bit Reg37 (clk,x16_1_2, y35 );

 Subtractor20Bit Sub8(clk, {x3[15],x3[15],x3[15],x3[15], x3[15: 0] },  { x1[14: 0], 5'b0 },x20); 

 Register21Bit Reg38 (clk,x20, x20_0_1 );
 Register21Bit Reg39 (clk,x20_0_1, x20_0_2 );
 Register25Bit Reg40 (clk, {x20_0_2, 4'b0}, y12 );
 Register21Bit Reg41 (clk,x20, x20_1_1 );
 Register21Bit Reg42 (clk,x20_1_1, x20_1_2 );
 Register25Bit Reg43 (clk, {x20_1_2, 4'b0}, y28 );

 Adder20Bit Adder9(clk,  { x2[14: 0], 5'b0 }, {x3[15],x3[15],x3[15],x3[15], x3[15: 0]  }, x22); 


 Adder15Bit Adder10(clk,  {x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 3'b0 },x4); 


 Subtractor20Bit Sub9(clk, {x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x4[15: 0], 4'b0 },x21); 

 Register21Bit Reg44 (clk,x21, x21_0_1 );
 Register21Bit Reg45 (clk,x21_0_1, x21_0_2 );
 Register21Bit Reg46 (clk,x21_0_2, y3 );
 Register21Bit Reg47 (clk,x21, x21_1_1 );
 Register21Bit Reg48 (clk,x21_1_1, x21_1_2 );
 Register21Bit Reg49 (clk,x21_1_2, y37 );

 Adder22Bit Adder11(clk,  { x2[14: 0], 7'b0 }, {x4[15],x4[15],x4[15],x4[15],x4[15],x4[15], x4[15: 0]  }, x23); 

 Negator23Bit Neg0(clk, x23, x38 );
 Register23Bit Reg50 (clk,x38, x38_0_1 );
 Register24Bit Reg51 (clk, {x38_0_1, 1'b0}, y10 );
 Register23Bit Reg52 (clk,x38, x38_1_1 );
 Register24Bit Reg53 (clk, {x38_1_1, 1'b0}, y30 );

 Adder25Bit Adder12(clk,  {x4[15],x4[15],x4[15],x4[15],x4[15],x4[15], x4[15: 0], 3'b0 }, { x15[24: 0]  }, x31); 

 Register26Bit Reg54 (clk,x31, x31_0_1 );
 Register26Bit Reg55 (clk,x31_0_1, y13 );
 Register26Bit Reg56 (clk,x31, x31_1_1 );
 Register26Bit Reg57 (clk,x31_1_1, y27 );

 Subtractor17Bit Sub10(clk, { x0[11: 0], 5'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x5); 


 Subtractor24Bit Sub11(clk, {x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15], x3[15: 0] },  { x5[17: 0], 6'b0 },x30); 

 Register25Bit Reg58 (clk,x30, x30_0_1 );
 Register25Bit Reg59 (clk,x30_0_1, x30_0_2 );
 Register25Bit Reg60 (clk,x30_0_2, y14 );
 Register25Bit Reg61 (clk,x30, x30_1_1 );
 Register25Bit Reg62 (clk,x30_1_1, x30_1_2 );
 Register25Bit Reg63 (clk,x30_1_2, y26 );
 Negator18Bit Neg1(clk, x5, x36 );
 Register18Bit Reg64 (clk,x36, x36_0_1 );
 Register18Bit Reg65 (clk,x36_0_1, x36_0_2 );
 Register18Bit Reg66 (clk,x36_0_2, y7 );
 Register18Bit Reg67 (clk,x36, x36_1_1 );
 Register18Bit Reg68 (clk,x36_1_1, x36_1_2 );
 Register18Bit Reg69 (clk,x36_1_2, y33 );

 Subtractor20Bit Sub12(clk, { x0[11: 0], 8'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x6); 


 Subtractor21Bit Sub13(clk, { x6[20: 0] },  {x2[14],x2[14],x2[14], x2[14: 0], 3'b0 },x24); 

 Register22Bit Reg70 (clk,x24, x24_0_1 );
 Register22Bit Reg71 (clk,x24_0_1, x24_0_2 );
 Register23Bit Reg72 (clk, {x24_0_2, 1'b0}, y4 );
 Register22Bit Reg73 (clk,x24, x24_1_1 );
 Register22Bit Reg74 (clk,x24_1_1, x24_1_2 );
 Register23Bit Reg75 (clk, {x24_1_2, 1'b0}, y36 );

 Adder26Bit Adder13(clk,  {x6[20],x6[20],x6[20],x6[20],x6[20], x6[20: 0] },  { x17[22: 0], 3'b0 },x32); 

 Negator27Bit Neg2(clk, x32, x39 );
 Register27Bit Reg76 (clk,x39, y15 );
 Register27Bit Reg77 (clk,x39, y25 );

 Adder17Bit Adder14(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 5'b0 },x9); 

 Negator18Bit Neg3(clk, x9, x35 );
 Register18Bit Reg78 (clk,x35, x35_0_1 );
 Register18Bit Reg79 (clk,x35_0_1, x35_0_2 );
 Register21Bit Reg80 (clk, {x35_0_2, 3'b0}, y2 );
 Register18Bit Reg81 (clk,x35, x35_1_1 );
 Register18Bit Reg82 (clk,x35_1_1, x35_1_2 );
 Register21Bit Reg83 (clk, {x35_1_2, 3'b0}, y38 );

 Subtractor16Bit Sub14(clk, {x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 4'b0 },x10); 

 Negator17Bit Neg4(clk, x10, x37 );
 Register17Bit Reg84 (clk,x37, x37_0_1 );
 Register17Bit Reg85 (clk,x37_0_1, x37_0_2 );
 Register23Bit Reg86 (clk, {x37_0_2, 6'b0}, y8 );
 Register17Bit Reg87 (clk,x37, x37_1_1 );
 Register17Bit Reg88 (clk,x37_1_1, x37_1_2 );
 Register23Bit Reg89 (clk, {x37_1_2, 6'b0}, y32 );

 Adder20Bit Adder15(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 8'b0 },x11); 


 Subtractor21Bit Sub15(clk, {x2[14],x2[14],x2[14], x2[14: 0], 3'b0 }, { x11[20: 0] }, x28); 

 Register22Bit Reg90 (clk,x28, x28_0_1 );
 Register22Bit Reg91 (clk,x28_0_1, x28_0_2 );
 Register22Bit Reg92 (clk,x28_0_2, y9 );
 Register22Bit Reg93 (clk,x28, x28_1_1 );
 Register22Bit Reg94 (clk,x28_1_1, x28_1_2 );
 Register22Bit Reg95 (clk,x28_1_2, y31 );

 Adder26Bit Adder16(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 14'b0 },x12); 


 Adder27Bit Adder17(clk,  { x12[26: 0] },  {x22[20],x22[20], x22[20: 0], 4'b0 },x34); 

 Register28Bit Reg96 (clk,x34, x34_0_1 );
 Register28Bit Reg97 (clk,x34_0_1, y19 );
 Register28Bit Reg98 (clk,x34, x34_1_1 );
 Register28Bit Reg99 (clk,x34_1_1, y21 );
 Register12Bit Reg100 (clk,x0, x0_26_1 );
 Register12Bit Reg101 (clk,x0_26_1, x0_26_2 );
 Register12Bit Reg102 (clk,x0_26_2, x0_26_3 );
 Register12Bit Reg103 (clk,x0_26_3, x0_26_4 );
 Register27Bit Reg104 (clk, {x0_26_4, 15'b0}, y20 );
 Register21Bit Reg105 (clk,y0, x40 );

 Adder21Bit Adder18(clk,  { x40[20: 0] },  { y1[20: 0]  }, x41); 

 Register22Bit Reg106 (clk,x41, x41_1 );

 Adder22Bit Adder19(clk,  { x41_1[21: 0] },  {y2[20], y2[20: 0]  }, x42); 

 Register23Bit Reg107 (clk,x42, x42_1 );

 Adder23Bit Adder20(clk,  { x42_1[22: 0] },  {y3[20],y3[20], y3[20: 0]  }, x43); 

 Register24Bit Reg108 (clk,x43, x43_1 );

 Adder24Bit Adder21(clk,  { x43_1[23: 0] },  {y4[22], y4[22: 0]  }, x44); 

 Register25Bit Reg109 (clk,x44, x44_1 );

 Adder25Bit Adder22(clk,  { x44_1[24: 0] },  {y5[19],y5[19],y5[19],y5[19],y5[19], y5[19: 0]  }, x45); 

 Register26Bit Reg110 (clk,x45, x45_1 );

 Adder26Bit Adder23(clk,  { x45_1[25: 0] },  {y6[22],y6[22],y6[22], y6[22: 0]  }, x46); 

 Register27Bit Reg111 (clk,x46, x46_1 );

 Adder27Bit Adder24(clk,  { x46_1[26: 0] },  {y7[17],y7[17],y7[17],y7[17],y7[17],y7[17],y7[17],y7[17],y7[17], y7[17: 0]  }, x47); 

 Register28Bit Reg112 (clk,x47, x47_1 );

 Adder28Bit Adder25(clk,  { x47_1[27: 0] },  {y8[22],y8[22],y8[22],y8[22],y8[22], y8[22: 0]  }, x48); 

 Register29Bit Reg113 (clk,x48, x48_1 );

 Adder29Bit Adder26(clk,  { x48_1[28: 0] },  {y9[21],y9[21],y9[21],y9[21],y9[21],y9[21],y9[21], y9[21: 0]  }, x49); 

 Register29Bit Reg114 (clk,x49, x49_1 );

 Adder29Bit Adder27(clk,  { x49_1[28: 0] },  {y10[23],y10[23],y10[23],y10[23],y10[23], y10[23: 0]  }, x50); 

 Register29Bit Reg115 (clk,x50, x50_1 );

 Adder29Bit Adder28(clk,  { x50_1[28: 0] },  {y11[22],y11[22],y11[22],y11[22],y11[22],y11[22], y11[22: 0]  }, x51); 

 Register29Bit Reg116 (clk,x51, x51_1 );

 Adder29Bit Adder29(clk,  { x51_1[28: 0] },  {y12[24],y12[24],y12[24],y12[24], y12[24: 0]  }, x52); 

 Register29Bit Reg117 (clk,x52, x52_1 );

 Adder29Bit Adder30(clk,  { x52_1[28: 0] },  {y13[25],y13[25],y13[25], y13[25: 0]  }, x53); 

 Register29Bit Reg118 (clk,x53, x53_1 );

 Adder29Bit Adder31(clk,  { x53_1[28: 0] },  {y14[24],y14[24],y14[24],y14[24], y14[24: 0]  }, x54); 

 Register29Bit Reg119 (clk,x54, x54_1 );

 Adder29Bit Adder32(clk,  { x54_1[28: 0] },  {y15[26],y15[26], y15[26: 0]  }, x55); 

 Register29Bit Reg120 (clk,x55, x55_1 );

 Adder29Bit Adder33(clk,  { x55_1[28: 0] },  {y16[24],y16[24],y16[24],y16[24], y16[24: 0]  }, x56); 

 Register29Bit Reg121 (clk,x56, x56_1 );

 Adder29Bit Adder34(clk,  { x56_1[28: 0] },  {y17[27], y17[27: 0]  }, x57); 

 Register29Bit Reg122 (clk,x57, x57_1 );

 Adder29Bit Adder35(clk,  { x57_1[28: 0] },  {y18[24],y18[24],y18[24],y18[24], y18[24: 0]  }, x58); 

 Register29Bit Reg123 (clk,x58, x58_1 );

 Adder29Bit Adder36(clk,  { x58_1[28: 0] },  {y19[27], y19[27: 0]  }, x59); 

 Register29Bit Reg124 (clk,x59, x59_1 );

 Adder29Bit Adder37(clk,  { x59_1[28: 0] },  {y20[26],y20[26], y20[26: 0]  }, x60); 

 Register29Bit Reg125 (clk,x60, x60_1 );

 Adder29Bit Adder38(clk,  { x60_1[28: 0] },  {y21[27], y21[27: 0]  }, x61); 

 Register29Bit Reg126 (clk,x61, x61_1 );

 Adder29Bit Adder39(clk,  { x61_1[28: 0] },  {y22[24],y22[24],y22[24],y22[24], y22[24: 0]  }, x62); 

 Register29Bit Reg127 (clk,x62, x62_1 );

 Adder29Bit Adder40(clk,  { x62_1[28: 0] },  {y23[27], y23[27: 0]  }, x63); 

 Register29Bit Reg128 (clk,x63, x63_1 );

 Adder29Bit Adder41(clk,  { x63_1[28: 0] },  {y24[24],y24[24],y24[24],y24[24], y24[24: 0]  }, x64); 

 Register29Bit Reg129 (clk,x64, x64_1 );

 Adder29Bit Adder42(clk,  { x64_1[28: 0] },  {y25[26],y25[26], y25[26: 0]  }, x65); 

 Register29Bit Reg130 (clk,x65, x65_1 );

 Adder29Bit Adder43(clk,  { x65_1[28: 0] },  {y26[24],y26[24],y26[24],y26[24], y26[24: 0]  }, x66); 

 Register29Bit Reg131 (clk,x66, x66_1 );

 Adder29Bit Adder44(clk,  { x66_1[28: 0] },  {y27[25],y27[25],y27[25], y27[25: 0]  }, x67); 

 Register29Bit Reg132 (clk,x67, x67_1 );

 Adder29Bit Adder45(clk,  { x67_1[28: 0] },  {y28[24],y28[24],y28[24],y28[24], y28[24: 0]  }, x68); 

 Register29Bit Reg133 (clk,x68, x68_1 );

 Adder29Bit Adder46(clk,  { x68_1[28: 0] },  {y29[22],y29[22],y29[22],y29[22],y29[22],y29[22], y29[22: 0]  }, x69); 

 Register29Bit Reg134 (clk,x69, x69_1 );

 Adder29Bit Adder47(clk,  { x69_1[28: 0] },  {y30[23],y30[23],y30[23],y30[23],y30[23], y30[23: 0]  }, x70); 

 Register29Bit Reg135 (clk,x70, x70_1 );

 Adder29Bit Adder48(clk,  { x70_1[28: 0] },  {y31[21],y31[21],y31[21],y31[21],y31[21],y31[21],y31[21], y31[21: 0]  }, x71); 

 Register29Bit Reg136 (clk,x71, x71_1 );

 Adder29Bit Adder49(clk,  { x71_1[28: 0] },  {y32[22],y32[22],y32[22],y32[22],y32[22],y32[22], y32[22: 0]  }, x72); 

 Register29Bit Reg137 (clk,x72, x72_1 );

 Adder29Bit Adder50(clk,  { x72_1[28: 0] },  {y33[17],y33[17],y33[17],y33[17],y33[17],y33[17],y33[17],y33[17],y33[17],y33[17],y33[17], y33[17: 0]  }, x73); 

 Register29Bit Reg138 (clk,x73, x73_1 );

 Adder29Bit Adder51(clk,  { x73_1[28: 0] },  {y34[22],y34[22],y34[22],y34[22],y34[22],y34[22], y34[22: 0]  }, x74); 

 Register29Bit Reg139 (clk,x74, x74_1 );

 Adder29Bit Adder52(clk,  { x74_1[28: 0] },  {y35[19],y35[19],y35[19],y35[19],y35[19],y35[19],y35[19],y35[19],y35[19], y35[19: 0]  }, x75); 

 Register29Bit Reg140 (clk,x75, x75_1 );

 Adder29Bit Adder53(clk,  { x75_1[28: 0] },  {y36[22],y36[22],y36[22],y36[22],y36[22],y36[22], y36[22: 0]  }, x76); 

 Register29Bit Reg141 (clk,x76, x76_1 );

 Adder29Bit Adder54(clk,  { x76_1[28: 0] },  {y37[20],y37[20],y37[20],y37[20],y37[20],y37[20],y37[20],y37[20], y37[20: 0]  }, x77); 

 Register29Bit Reg142 (clk,x77, x77_1 );

 Adder29Bit Adder55(clk,  { x77_1[28: 0] },  {y38[20],y38[20],y38[20],y38[20],y38[20],y38[20],y38[20],y38[20], y38[20: 0]  }, x78); 

 Register29Bit Reg143 (clk,x78, x78_1 );

 Adder29Bit Adder56(clk,  { x78_1[28: 0] },  {y39[20],y39[20],y39[20],y39[20],y39[20],y39[20],y39[20],y39[20], y39[20: 0]  }, x79); 

 Register29Bit Reg144 (clk,x79, x79_1 );

 Adder29Bit Adder57(clk,  { x79_1[28: 0] },  {y40[20],y40[20],y40[20],y40[20],y40[20],y40[20],y40[20],y40[20], y40[20: 0]  }, Out_Y_wire); 

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

module Subtractor17Bit (clk, In1, In2, SubOut);
 input clk;
 input [16:0] In1, In2;
 output [17 :0] SubOut;

 reg [17 :0] SubOut; 

always @(posedge clk)
begin
SubOut <= (In1 - In2);
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

module Subtractor15Bit (clk, In1, In2, SubOut);
 input clk;
 input [14:0] In1, In2;
 output [15 :0] SubOut;

 reg [15 :0] SubOut; 

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

module Register18Bit (clk, In1, RegOut);
 input clk;
 input [17:0] In1;
 output [17 :0] RegOut;

 reg [17 :0] RegOut; 

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



module Negator27Bit (clk, In1, NegOut);
 input clk;
 input [26:0] In1;
 output [26 :0] NegOut;

 reg [26 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule

module Negator23Bit (clk, In1, NegOut);
 input clk;
 input [22:0] In1;
 output [22 :0] NegOut;

 reg [22 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule

module Negator18Bit (clk, In1, NegOut);
 input clk;
 input [17:0] In1;
 output [17 :0] NegOut;

 reg [17 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule

module Negator17Bit (clk, In1, NegOut);
 input clk;
 input [16:0] In1;
 output [16 :0] NegOut;

 reg [16 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule
