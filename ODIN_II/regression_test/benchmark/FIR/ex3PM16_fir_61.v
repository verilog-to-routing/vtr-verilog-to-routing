
 module ex3PM16_fir(clk, In_X, Out_Y); 

 input clk;
 input [11:0] In_X; 
 output [28 :0] Out_Y; 

 reg [11:0] x0 ;

 wire [14:0] x1 ;
 wire [17:0] x11 ;
 wire [24:0] x41 ;
 wire [24:0] x41_0_1 ;
 wire [26:0] y28 ;
 wire [24:0] x41_1_1 ;
 wire [26:0] y32 ;
 wire [21:0] x12 ;
 wire [21:0] x49 ;
 wire [21:0] x49_0_1 ;
 wire [22:0] y10 ;
 wire [21:0] x49_1_1 ;
 wire [22:0] y50 ;
 wire [21:0] x13 ;
 wire [22:0] x38 ;
 wire [22:0] x38_0_1 ;
 wire [26:0] y24 ;
 wire [22:0] x38_1_1 ;
 wire [26:0] y36 ;
 wire [19:0] x14 ;
 wire [23:0] x32 ;
 wire [23:0] x52 ;
 wire [23:0] y23 ;
 wire [23:0] y37 ;
 wire [20:0] x15 ;
 wire [20:0] x51 ;
 wire [20:0] x51_0_1 ;
 wire [20:0] y21 ;
 wire [20:0] x51_1_1 ;
 wire [20:0] y39 ;
 wire [25:0] x21 ;
 wire [25:0] x21_0_1 ;
 wire [25:0] x21_0_2 ;
 wire [25:0] y26 ;
 wire [25:0] x21_1_1 ;
 wire [25:0] x21_1_2 ;
 wire [25:0] y34 ;
 wire [19:0] x22 ;
 wire [19:0] x46 ;
 wire [19:0] x46_0_1 ;
 wire [19:0] y3 ;
 wire [19:0] x46_1_1 ;
 wire [19:0] y57 ;
 wire [20:0] x23 ;
 wire [20:0] x23_0_1 ;
 wire [20:0] x23_0_2 ;
 wire [20:0] y12 ;
 wire [20:0] x23_1_1 ;
 wire [20:0] x23_1_2 ;
 wire [20:0] y48 ;
 wire [23:0] x24 ;
 wire [23:0] x24_0_1 ;
 wire [23:0] x24_0_2 ;
 wire [23:0] y16 ;
 wire [23:0] x24_1_1 ;
 wire [23:0] x24_1_2 ;
 wire [23:0] y44 ;
 wire [22:0] x25 ;
 wire [22:0] x25_0_1 ;
 wire [22:0] x25_0_2 ;
 wire [23:0] y18 ;
 wire [22:0] x25_1_1 ;
 wire [22:0] x25_1_2 ;
 wire [23:0] y42 ;
 wire [22:0] x26 ;
 wire [22:0] x26_0_1 ;
 wire [22:0] x26_0_2 ;
 wire [22:0] y2 ;
 wire [22:0] x26_1_1 ;
 wire [22:0] x26_1_2 ;
 wire [22:0] y58 ;
 wire [23:0] x27 ;
 wire [23:0] x27_0_1 ;
 wire [23:0] x27_0_2 ;
 wire [24:0] y22 ;
 wire [23:0] x27_1_1 ;
 wire [23:0] x27_1_2 ;
 wire [24:0] y38 ;
 wire [22:0] x28 ;
 wire [22:0] x28_0_1 ;
 wire [22:0] x28_0_2 ;
 wire [22:0] y7 ;
 wire [22:0] x28_1_1 ;
 wire [22:0] x28_1_2 ;
 wire [22:0] y53 ;
 wire [23:0] x29 ;
 wire [23:0] x47 ;
 wire [23:0] y6 ;
 wire [23:0] y54 ;
 wire [23:0] x30 ;
 wire [23:0] x30_0_1 ;
 wire [23:0] x30_0_2 ;
 wire [23:0] y15 ;
 wire [23:0] x30_1_1 ;
 wire [23:0] x30_1_2 ;
 wire [23:0] y45 ;
 wire [23:0] x31 ;
 wire [23:0] x31_0_1 ;
 wire [23:0] x31_0_2 ;
 wire [23:0] y17 ;
 wire [23:0] x31_1_1 ;
 wire [23:0] x31_1_2 ;
 wire [23:0] y43 ;
 wire [14:0] x2 ;
 wire [22:0] x6 ;
 wire [23:0] x35 ;
 wire [23:0] x48 ;
 wire [23:0] y9 ;
 wire [23:0] y51 ;
 wire [23:0] x44 ;
 wire [23:0] x44_0_1 ;
 wire [23:0] y4 ;
 wire [23:0] x45 ;
 wire [23:0] x45_0_1 ;
 wire [23:0] y56 ;
 wire [23:0] x16 ;
 wire [24:0] x43 ;
 wire [24:0] x43_0_1 ;
 wire [26:0] y27 ;
 wire [24:0] x43_1_1 ;
 wire [26:0] y33 ;
 wire [20:0] x33 ;
 wire [20:0] x33_0_1 ;
 wire [20:0] x33_0_2 ;
 wire [20:0] y5 ;
 wire [20:0] x33_1_1 ;
 wire [20:0] x33_1_2 ;
 wire [20:0] y55 ;
 wire [20:0] x34 ;
 wire [20:0] x34_0_1 ;
 wire [20:0] x34_0_2 ;
 wire [22:0] y19 ;
 wire [20:0] x34_1_1 ;
 wire [20:0] x34_1_2 ;
 wire [22:0] y41 ;
 wire [21:0] x36 ;
 wire [21:0] x36_0_1 ;
 wire [21:0] x36_0_2 ;
 wire [22:0] y0 ;
 wire [21:0] x36_1_1 ;
 wire [21:0] x36_1_2 ;
 wire [22:0] y60 ;
 wire [20:0] x37 ;
 wire [20:0] x37_0_1 ;
 wire [20:0] x37_0_2 ;
 wire [21:0] y14 ;
 wire [20:0] x37_1_1 ;
 wire [20:0] x37_1_2 ;
 wire [21:0] y46 ;
 wire [15:0] x3 ;
 wire [22:0] x17 ;
 wire [22:0] x17_0_1 ;
 wire [22:0] x17_0_2 ;
 wire [22:0] y8 ;
 wire [22:0] x17_1_1 ;
 wire [22:0] x17_1_2 ;
 wire [22:0] y52 ;
 wire [23:0] x39 ;
 wire [23:0] x39_0_1 ;
 wire [23:0] x39_0_2 ;
 wire [24:0] y25 ;
 wire [23:0] x39_1_1 ;
 wire [23:0] x39_1_2 ;
 wire [24:0] y35 ;
 wire [26:0] x40 ;
 wire [26:0] x40_0_1 ;
 wire [26:0] x40_0_2 ;
 wire [27:0] y29 ;
 wire [26:0] x40_1_1 ;
 wire [26:0] x40_1_2 ;
 wire [27:0] y31 ;
 wire [15:0] x4 ;
 wire [19:0] x18 ;
 wire [19:0] x18_0_1 ;
 wire [19:0] x18_0_2 ;
 wire [20:0] y1 ;
 wire [19:0] x18_1_1 ;
 wire [19:0] x18_1_2 ;
 wire [20:0] y59 ;
 wire [21:0] x19 ;
 wire [21:0] x19_0_1 ;
 wire [21:0] x19_0_2 ;
 wire [24:0] y20 ;
 wire [21:0] x19_1_1 ;
 wire [21:0] x19_1_2 ;
 wire [24:0] y40 ;
 wire [23:0] x42 ;
 wire [23:0] x50 ;
 wire [23:0] x50_0_1 ;
 wire [23:0] y11 ;
 wire [23:0] x50_1_1 ;
 wire [23:0] y49 ;
 wire [17:0] x5 ;
 wire [23:0] x20 ;
 wire [23:0] x20_0_1 ;
 wire [23:0] x20_0_2 ;
 wire [23:0] y13 ;
 wire [23:0] x20_1_1 ;
 wire [23:0] x20_1_2 ;
 wire [23:0] y47 ;
 wire [20:0] x7 ;
 wire [16:0] x8 ;
 wire [19:0] x9 ;
 wire [22:0] x10 ;
 wire [11:0] x44_29_1 ;
 wire [11:0] x44_29_2 ;
 wire [11:0] x45_30_1 ;
 wire [11:0] x45_30_2 ;
 wire [11:0] x0_31_1 ;
 wire [11:0] x0_31_2 ;
 wire [11:0] x0_31_3 ;
 wire [11:0] x0_31_4 ;
 wire [26:0] y30 ;
 wire [22:0] x53 ;
 wire [23:0] x54 ;
 wire [23:0] x54_1 ;
 wire [24:0] x55 ;
 wire [24:0] x55_1 ;
 wire [25:0] x56 ;
 wire [25:0] x56_1 ;
 wire [26:0] x57 ;
 wire [26:0] x57_1 ;
 wire [27:0] x58 ;
 wire [27:0] x58_1 ;
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
 wire [28:0] x80 ;
 wire [28:0] x80_1 ;
 wire [28:0] x81 ;
 wire [28:0] x81_1 ;
 wire [28:0] x82 ;
 wire [28:0] x82_1 ;
 wire [28:0] x83 ;
 wire [28:0] x83_1 ;
 wire [28:0] x84 ;
 wire [28:0] x84_1 ;
 wire [28:0] x85 ;
 wire [28:0] x85_1 ;
 wire [28:0] x86 ;
 wire [28:0] x86_1 ;
 wire [28:0] x87 ;
 wire [28:0] x87_1 ;
 wire [28:0] x88 ;
 wire [28:0] x88_1 ;
 wire [28:0] x89 ;
 wire [28:0] x89_1 ;
 wire [28:0] x90 ;
 wire [28:0] x90_1 ;
 wire [28:0] x91 ;
 wire [28:0] x91_1 ;
 wire [28:0] x92 ;
 wire [28:0] x92_1 ;
 wire [28:0] x93 ;
 wire [28:0] x93_1 ;
 wire [28:0] x94 ;
 wire [28:0] x94_1 ;
 wire [28:0] x95 ;
 wire [28:0] x95_1 ;
 wire [28:0] x96 ;
 wire [28:0] x96_1 ;
 wire [28:0] x97 ;
 wire [28:0] x97_1 ;
 wire [28:0] x98 ;
 wire [28:0] x98_1 ;
 wire [28:0] x99 ;
 wire [28:0] x99_1 ;
 wire [28:0] x100 ;
 wire [28:0] x100_1 ;
 wire [28:0] x101 ;
 wire [28:0] x101_1 ;
 wire [28:0] x102 ;
 wire [28:0] x102_1 ;
 wire [28:0] x103 ;
 wire [28:0] x103_1 ;
 wire [28:0] x104 ;
 wire [28:0] x104_1 ;
 wire [28:0] x105 ;
 wire [28:0] x105_1 ;
 wire [28:0] x106 ;
 wire [28:0] x106_1 ;
 wire [28:0] x107 ;
 wire [28:0] x107_1 ;
 wire [28:0] x108 ;
 wire [28:0] x108_1 ;
 wire [28:0] x109 ;
 wire [28:0] x109_1 ;
 wire [28:0] x110 ;
 wire [28:0] x110_1 ;
 wire [28:0] x111 ;
 wire [28:0] x111_1 ;
 wire [28:0] x112 ;
 wire [28:0] x112_1 ;
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


 Adder17Bit Adder0(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 2'b0 },x11); 


 Adder21Bit Adder1(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 6'b0 },x12); 

 Negator22Bit Neg0(clk, x12, x49 );
 Register22Bit Reg0 (clk,x49, x49_0_1 );
 Register23Bit Reg1 (clk, {x49_0_1, 1'b0}, y10 );
 Register22Bit Reg2 (clk,x49, x49_1_1 );
 Register23Bit Reg3 (clk, {x49_1_1, 1'b0}, y50 );

 Subtractor21Bit Sub1(clk, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 6'b0 },x13); 


 Adder19Bit Adder2(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 4'b0 },x14); 


 Adder23Bit Adder3(clk,  { x1[14: 0], 8'b0 }, {x14[19],x14[19],x14[19], x14[19: 0]  }, x32); 

 Negator24Bit Neg1(clk, x32, x52 );
 Register24Bit Reg4 (clk,x52, y23 );
 Register24Bit Reg5 (clk,x52, y37 );

 Adder20Bit Adder4(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x1[14: 0], 5'b0 },x15); 

 Negator21Bit Neg2(clk, x15, x51 );
 Register21Bit Reg6 (clk,x51, x51_0_1 );
 Register21Bit Reg7 (clk,x51_0_1, y21 );
 Register21Bit Reg8 (clk,x51, x51_1_1 );
 Register21Bit Reg9 (clk,x51_1_1, y39 );

 Adder25Bit Adder5(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x1[14: 0], 10'b0 },x21); 

 Register26Bit Reg10 (clk,x21, x21_0_1 );
 Register26Bit Reg11 (clk,x21_0_1, x21_0_2 );
 Register26Bit Reg12 (clk,x21_0_2, y26 );
 Register26Bit Reg13 (clk,x21, x21_1_1 );
 Register26Bit Reg14 (clk,x21_1_1, x21_1_2 );
 Register26Bit Reg15 (clk,x21_1_2, y34 );

 Adder14Bit Adder6(clk,  {x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 2'b0 },x2); 


 Subtractor22Bit Sub2(clk, { x2[14: 0], 7'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x6); 


 Adder23Bit Adder7(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0], 3'b0 }, { x6[22: 0]  }, x29); 

 Negator24Bit Neg3(clk, x29, x47 );
 Register24Bit Reg16 (clk,x47, y6 );
 Register24Bit Reg17 (clk,x47, y54 );

 Adder23Bit Adder8(clk,  {x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0], 3'b0 }, { x6[22: 0]  }, x35); 

 Negator24Bit Neg4(clk, x35, x48 );
 Register24Bit Reg18 (clk,x48, y9 );
 Register24Bit Reg19 (clk,x48, y51 );

 Adder23Bit Adder9(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x2[14: 0], 8'b0 },x16); 


 Adder19Bit Adder10(clk,  {x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x2[14: 0], 4'b0 },x22); 

 Negator20Bit Neg5(clk, x22, x46 );
 Register20Bit Reg20 (clk,x46, x46_0_1 );
 Register20Bit Reg21 (clk,x46_0_1, y3 );
 Register20Bit Reg22 (clk,x46, x46_1_1 );
 Register20Bit Reg23 (clk,x46_1_1, y57 );

 Subtractor20Bit Sub3(clk, { x1[14: 0], 5'b0 }, {x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] }, x23); 

 Register21Bit Reg24 (clk,x23, x23_0_1 );
 Register21Bit Reg25 (clk,x23_0_1, x23_0_2 );
 Register21Bit Reg26 (clk,x23_0_2, y12 );
 Register21Bit Reg27 (clk,x23, x23_1_1 );
 Register21Bit Reg28 (clk,x23_1_1, x23_1_2 );
 Register21Bit Reg29 (clk,x23_1_2, y48 );

 Subtractor23Bit Sub4(clk, {x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] },  { x1[14: 0], 8'b0 },x24); 

 Register24Bit Reg30 (clk,x24, x24_0_1 );
 Register24Bit Reg31 (clk,x24_0_1, x24_0_2 );
 Register24Bit Reg32 (clk,x24_0_2, y16 );
 Register24Bit Reg33 (clk,x24, x24_1_1 );
 Register24Bit Reg34 (clk,x24_1_1, x24_1_2 );
 Register24Bit Reg35 (clk,x24_1_2, y44 );

 Subtractor22Bit Sub5(clk, { x2[14: 0], 7'b0 }, {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] }, x25); 

 Register23Bit Reg36 (clk,x25, x25_0_1 );
 Register23Bit Reg37 (clk,x25_0_1, x25_0_2 );
 Register24Bit Reg38 (clk, {x25_0_2, 1'b0}, y18 );
 Register23Bit Reg39 (clk,x25, x25_1_1 );
 Register23Bit Reg40 (clk,x25_1_1, x25_1_2 );
 Register24Bit Reg41 (clk, {x25_1_2, 1'b0}, y42 );

 Subtractor20Bit Sub6(clk, {x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] },  { x2[14: 0], 5'b0 },x33); 

 Register21Bit Reg42 (clk,x33, x33_0_1 );
 Register21Bit Reg43 (clk,x33_0_1, x33_0_2 );
 Register21Bit Reg44 (clk,x33_0_2, y5 );
 Register21Bit Reg45 (clk,x33, x33_1_1 );
 Register21Bit Reg46 (clk,x33_1_1, x33_1_2 );
 Register21Bit Reg47 (clk,x33_1_2, y55 );

 Adder20Bit Adder11(clk,  {x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0] },  { x2[14: 0], 5'b0 },x34); 

 Register21Bit Reg48 (clk,x34, x34_0_1 );
 Register21Bit Reg49 (clk,x34_0_1, x34_0_2 );
 Register23Bit Reg50 (clk, {x34_0_2, 2'b0}, y19 );
 Register21Bit Reg51 (clk,x34, x34_1_1 );
 Register21Bit Reg52 (clk,x34_1_1, x34_1_2 );
 Register23Bit Reg53 (clk, {x34_1_2, 2'b0}, y41 );

 Adder22Bit Adder12(clk,  {x2[14],x2[14],x2[14],x2[14],x2[14], x2[14: 0], 2'b0 }, { x13[21: 0]  }, x38); 

 Register23Bit Reg54 (clk,x38, x38_0_1 );
 Register27Bit Reg55 (clk, {x38_0_1, 4'b0}, y24 );
 Register23Bit Reg56 (clk,x38, x38_1_1 );
 Register27Bit Reg57 (clk, {x38_1_1, 4'b0}, y36 );

 Adder15Bit Adder13(clk,  {x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 3'b0 },x3); 


 Subtractor22Bit Sub7(clk, { x3[15: 0], 6'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x17); 

 Register23Bit Reg58 (clk,x17, x17_0_1 );
 Register23Bit Reg59 (clk,x17_0_1, x17_0_2 );
 Register23Bit Reg60 (clk,x17_0_2, y8 );
 Register23Bit Reg61 (clk,x17, x17_1_1 );
 Register23Bit Reg62 (clk,x17_1_1, x17_1_2 );
 Register23Bit Reg63 (clk,x17_1_2, y52 );

 Subtractor22Bit Sub8(clk, {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x3[15: 0], 6'b0 },x26); 

 Register23Bit Reg64 (clk,x26, x26_0_1 );
 Register23Bit Reg65 (clk,x26_0_1, x26_0_2 );
 Register23Bit Reg66 (clk,x26_0_2, y2 );
 Register23Bit Reg67 (clk,x26, x26_1_1 );
 Register23Bit Reg68 (clk,x26_1_1, x26_1_2 );
 Register23Bit Reg69 (clk,x26_1_2, y58 );

 Subtractor23Bit Sub9(clk, { x3[15: 0], 7'b0 }, {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] }, x27); 

 Register24Bit Reg70 (clk,x27, x27_0_1 );
 Register24Bit Reg71 (clk,x27_0_1, x27_0_2 );
 Register25Bit Reg72 (clk, {x27_0_2, 1'b0}, y22 );
 Register24Bit Reg73 (clk,x27, x27_1_1 );
 Register24Bit Reg74 (clk,x27_1_1, x27_1_2 );
 Register25Bit Reg75 (clk, {x27_1_2, 1'b0}, y38 );

 Adder23Bit Adder14(clk,  {x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15], x3[15: 0] },  { x3[15: 0], 7'b0 },x39); 

 Register24Bit Reg76 (clk,x39, x39_0_1 );
 Register24Bit Reg77 (clk,x39_0_1, x39_0_2 );
 Register25Bit Reg78 (clk, {x39_0_2, 1'b0}, y25 );
 Register24Bit Reg79 (clk,x39, x39_1_1 );
 Register24Bit Reg80 (clk,x39_1_1, x39_1_2 );
 Register25Bit Reg81 (clk, {x39_1_2, 1'b0}, y35 );

 Subtractor26Bit Sub10(clk, { x3[15: 0], 10'b0 }, {x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15], x3[15: 0] }, x40); 

 Register27Bit Reg82 (clk,x40, x40_0_1 );
 Register27Bit Reg83 (clk,x40_0_1, x40_0_2 );
 Register28Bit Reg84 (clk, {x40_0_2, 1'b0}, y29 );
 Register27Bit Reg85 (clk,x40, x40_1_1 );
 Register27Bit Reg86 (clk,x40_1_1, x40_1_2 );
 Register28Bit Reg87 (clk, {x40_1_2, 1'b0}, y31 );

 Subtractor24Bit Sub11(clk, {x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15],x3[15], x3[15: 0] },  { x11[17: 0], 6'b0 },x41); 

 Register25Bit Reg88 (clk,x41, x41_0_1 );
 Register27Bit Reg89 (clk, {x41_0_1, 2'b0}, y28 );
 Register25Bit Reg90 (clk,x41, x41_1_1 );
 Register27Bit Reg91 (clk, {x41_1_1, 2'b0}, y32 );

 Subtractor15Bit Sub12(clk, {x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 3'b0 },x4); 


 Subtractor19Bit Sub13(clk, { x0[11: 0], 7'b0 }, {x4[15],x4[15],x4[15], x4[15: 0] }, x18); 

 Register20Bit Reg92 (clk,x18, x18_0_1 );
 Register20Bit Reg93 (clk,x18_0_1, x18_0_2 );
 Register21Bit Reg94 (clk, {x18_0_2, 1'b0}, y1 );
 Register20Bit Reg95 (clk,x18, x18_1_1 );
 Register20Bit Reg96 (clk,x18_1_1, x18_1_2 );
 Register21Bit Reg97 (clk, {x18_1_2, 1'b0}, y59 );

 Subtractor21Bit Sub14(clk, { x4[15: 0], 5'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x19); 

 Register22Bit Reg98 (clk,x19, x19_0_1 );
 Register22Bit Reg99 (clk,x19_0_1, x19_0_2 );
 Register25Bit Reg100 (clk, {x19_0_2, 3'b0}, y20 );
 Register22Bit Reg101 (clk,x19, x19_1_1 );
 Register22Bit Reg102 (clk,x19_1_1, x19_1_2 );
 Register25Bit Reg103 (clk, {x19_1_2, 3'b0}, y40 );

 Adder23Bit Adder15(clk,  {x4[15],x4[15],x4[15],x4[15],x4[15],x4[15],x4[15], x4[15: 0] },  { x4[15: 0], 7'b0 },x42); 

 Negator24Bit Neg6(clk, x42, x50 );
 Register24Bit Reg104 (clk,x50, x50_0_1 );
 Register24Bit Reg105 (clk,x50_0_1, y11 );
 Register24Bit Reg106 (clk,x50, x50_1_1 );
 Register24Bit Reg107 (clk,x50_1_1, y49 );

 Subtractor24Bit Sub15(clk, {x4[15],x4[15],x4[15],x4[15],x4[15], x4[15: 0], 3'b0 }, { x16[23: 0] }, x43); 

 Register25Bit Reg108 (clk,x43, x43_0_1 );
 Register27Bit Reg109 (clk, {x43_0_1, 2'b0}, y27 );
 Register25Bit Reg110 (clk,x43, x43_1_1 );
 Register27Bit Reg111 (clk, {x43_1_1, 2'b0}, y33 );

 Adder17Bit Adder16(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 5'b0 },x5); 


 Subtractor23Bit Sub16(clk, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x5[17: 0], 5'b0 },x20); 

 Register24Bit Reg112 (clk,x20, x20_0_1 );
 Register24Bit Reg113 (clk,x20_0_1, x20_0_2 );
 Register24Bit Reg114 (clk,x20_0_2, y13 );
 Register24Bit Reg115 (clk,x20, x20_1_1 );
 Register24Bit Reg116 (clk,x20_1_1, x20_1_2 );
 Register24Bit Reg117 (clk,x20_1_2, y47 );

 Adder22Bit Adder17(clk,  { x1[14: 0], 7'b0 }, {x5[17],x5[17],x5[17],x5[17], x5[17: 0]  }, x28); 

 Register23Bit Reg118 (clk,x28, x28_0_1 );
 Register23Bit Reg119 (clk,x28_0_1, x28_0_2 );
 Register23Bit Reg120 (clk,x28_0_2, y7 );
 Register23Bit Reg121 (clk,x28, x28_1_1 );
 Register23Bit Reg122 (clk,x28_1_1, x28_1_2 );
 Register23Bit Reg123 (clk,x28_1_2, y53 );

 Adder20Bit Adder18(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 8'b0 },x7); 


 Subtractor21Bit Sub17(clk, { x7[20: 0] },  {x2[14],x2[14],x2[14], x2[14: 0], 3'b0 },x36); 

 Register22Bit Reg124 (clk,x36, x36_0_1 );
 Register22Bit Reg125 (clk,x36_0_1, x36_0_2 );
 Register23Bit Reg126 (clk, {x36_0_2, 1'b0}, y0 );
 Register22Bit Reg127 (clk,x36, x36_1_1 );
 Register22Bit Reg128 (clk,x36_1_1, x36_1_2 );
 Register23Bit Reg129 (clk, {x36_1_2, 1'b0}, y60 );

 Adder16Bit Adder19(clk,  {x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 4'b0 },x8); 


 Adder23Bit Adder20(clk,  {x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14],x1[14], x1[14: 0] },  { x8[16: 0], 6'b0 },x30); 

 Register24Bit Reg130 (clk,x30, x30_0_1 );
 Register24Bit Reg131 (clk,x30_0_1, x30_0_2 );
 Register24Bit Reg132 (clk,x30_0_2, y15 );
 Register24Bit Reg133 (clk,x30, x30_1_1 );
 Register24Bit Reg134 (clk,x30_1_1, x30_1_2 );
 Register24Bit Reg135 (clk,x30_1_2, y45 );

 Adder19Bit Adder21(clk,  {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] },  { x0[11: 0], 7'b0 },x9); 


 Adder20Bit Adder22(clk,  {x2[14],x2[14],x2[14], x2[14: 0], 2'b0 }, { x9[19: 0]  }, x37); 

 Register21Bit Reg136 (clk,x37, x37_0_1 );
 Register21Bit Reg137 (clk,x37_0_1, x37_0_2 );
 Register22Bit Reg138 (clk, {x37_0_2, 1'b0}, y14 );
 Register21Bit Reg139 (clk,x37, x37_1_1 );
 Register21Bit Reg140 (clk,x37_1_1, x37_1_2 );
 Register22Bit Reg141 (clk, {x37_1_2, 1'b0}, y46 );

 Subtractor22Bit Sub18(clk, { x0[11: 0], 10'b0 }, {x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11],x0[11], x0[11: 0] }, x10); 


 Subtractor23Bit Sub19(clk, {x1[14],x1[14],x1[14],x1[14], x1[14: 0], 4'b0 }, { x10[22: 0] }, x31); 

 Register24Bit Reg142 (clk,x31, x31_0_1 );
 Register24Bit Reg143 (clk,x31_0_1, x31_0_2 );
 Register24Bit Reg144 (clk,x31_0_2, y17 );
 Register24Bit Reg145 (clk,x31, x31_1_1 );
 Register24Bit Reg146 (clk,x31_1_1, x31_1_2 );
 Register24Bit Reg147 (clk,x31_1_2, y43 );
 Register12Bit Reg148 (clk,x0, x44_29_1 );
 Register12Bit Reg149 (clk,x44_29_1, x44_29_2 );

 Adder23Bit Adder23(clk,  {x44_29_2[11],x44_29_2[11],x44_29_2[11],x44_29_2[11],x44_29_2[11],x44_29_2[11],x44_29_2[11], x44_29_2[11: 0], 4'b0 }, { x6[22: 0]  }, x44); 

 Register24Bit Reg150 (clk,x44, x44_0_1 );
 Register24Bit Reg151 (clk,x44_0_1, y4 );
 Register12Bit Reg152 (clk,x0, x45_30_1 );
 Register12Bit Reg153 (clk,x45_30_1, x45_30_2 );

 Adder23Bit Adder24(clk,  {x45_30_2[11],x45_30_2[11],x45_30_2[11],x45_30_2[11],x45_30_2[11],x45_30_2[11],x45_30_2[11], x45_30_2[11: 0], 4'b0 }, { x6[22: 0]  }, x45); 

 Register24Bit Reg154 (clk,x45, x45_0_1 );
 Register24Bit Reg155 (clk,x45_0_1, y56 );
 Register12Bit Reg156 (clk,x0, x0_31_1 );
 Register12Bit Reg157 (clk,x0_31_1, x0_31_2 );
 Register12Bit Reg158 (clk,x0_31_2, x0_31_3 );
 Register12Bit Reg159 (clk,x0_31_3, x0_31_4 );
 Register27Bit Reg160 (clk, {x0_31_4, 15'b0}, y30 );
 Register23Bit Reg161 (clk,y0, x53 );

 Adder23Bit Adder25(clk,  { x53[22: 0] },  {y1[20],y1[20], y1[20: 0]  }, x54); 

 Register24Bit Reg162 (clk,x54, x54_1 );

 Adder24Bit Adder26(clk,  { x54_1[23: 0] },  {y2[22], y2[22: 0]  }, x55); 

 Register25Bit Reg163 (clk,x55, x55_1 );

 Adder25Bit Adder27(clk,  { x55_1[24: 0] },  {y3[19],y3[19],y3[19],y3[19],y3[19], y3[19: 0]  }, x56); 

 Register26Bit Reg164 (clk,x56, x56_1 );

 Adder26Bit Adder28(clk,  { x56_1[25: 0] },  {y4[23],y4[23], y4[23: 0]  }, x57); 

 Register27Bit Reg165 (clk,x57, x57_1 );

 Adder27Bit Adder29(clk,  { x57_1[26: 0] },  {y5[20],y5[20],y5[20],y5[20],y5[20],y5[20], y5[20: 0]  }, x58); 

 Register28Bit Reg166 (clk,x58, x58_1 );

 Adder28Bit Adder30(clk,  { x58_1[27: 0] },  {y6[23],y6[23],y6[23],y6[23], y6[23: 0]  }, x59); 

 Register29Bit Reg167 (clk,x59, x59_1 );

 Adder29Bit Adder31(clk,  { x59_1[28: 0] },  {y7[22],y7[22],y7[22],y7[22],y7[22],y7[22], y7[22: 0]  }, x60); 

 Register29Bit Reg168 (clk,x60, x60_1 );

 Adder29Bit Adder32(clk,  { x60_1[28: 0] },  {y8[22],y8[22],y8[22],y8[22],y8[22],y8[22], y8[22: 0]  }, x61); 

 Register29Bit Reg169 (clk,x61, x61_1 );

 Adder29Bit Adder33(clk,  { x61_1[28: 0] },  {y9[23],y9[23],y9[23],y9[23],y9[23], y9[23: 0]  }, x62); 

 Register29Bit Reg170 (clk,x62, x62_1 );

 Adder29Bit Adder34(clk,  { x62_1[28: 0] },  {y10[22],y10[22],y10[22],y10[22],y10[22],y10[22], y10[22: 0]  }, x63); 

 Register29Bit Reg171 (clk,x63, x63_1 );

 Adder29Bit Adder35(clk,  { x63_1[28: 0] },  {y11[23],y11[23],y11[23],y11[23],y11[23], y11[23: 0]  }, x64); 

 Register29Bit Reg172 (clk,x64, x64_1 );

 Adder29Bit Adder36(clk,  { x64_1[28: 0] },  {y12[20],y12[20],y12[20],y12[20],y12[20],y12[20],y12[20],y12[20], y12[20: 0]  }, x65); 

 Register29Bit Reg173 (clk,x65, x65_1 );

 Adder29Bit Adder37(clk,  { x65_1[28: 0] },  {y13[23],y13[23],y13[23],y13[23],y13[23], y13[23: 0]  }, x66); 

 Register29Bit Reg174 (clk,x66, x66_1 );

 Adder29Bit Adder38(clk,  { x66_1[28: 0] },  {y14[21],y14[21],y14[21],y14[21],y14[21],y14[21],y14[21], y14[21: 0]  }, x67); 

 Register29Bit Reg175 (clk,x67, x67_1 );

 Adder29Bit Adder39(clk,  { x67_1[28: 0] },  {y15[23],y15[23],y15[23],y15[23],y15[23], y15[23: 0]  }, x68); 

 Register29Bit Reg176 (clk,x68, x68_1 );

 Adder29Bit Adder40(clk,  { x68_1[28: 0] },  {y16[23],y16[23],y16[23],y16[23],y16[23], y16[23: 0]  }, x69); 

 Register29Bit Reg177 (clk,x69, x69_1 );

 Adder29Bit Adder41(clk,  { x69_1[28: 0] },  {y17[23],y17[23],y17[23],y17[23],y17[23], y17[23: 0]  }, x70); 

 Register29Bit Reg178 (clk,x70, x70_1 );

 Adder29Bit Adder42(clk,  { x70_1[28: 0] },  {y18[23],y18[23],y18[23],y18[23],y18[23], y18[23: 0]  }, x71); 

 Register29Bit Reg179 (clk,x71, x71_1 );

 Adder29Bit Adder43(clk,  { x71_1[28: 0] },  {y19[22],y19[22],y19[22],y19[22],y19[22],y19[22], y19[22: 0]  }, x72); 

 Register29Bit Reg180 (clk,x72, x72_1 );

 Adder29Bit Adder44(clk,  { x72_1[28: 0] },  {y20[24],y20[24],y20[24],y20[24], y20[24: 0]  }, x73); 

 Register29Bit Reg181 (clk,x73, x73_1 );

 Adder29Bit Adder45(clk,  { x73_1[28: 0] },  {y21[20],y21[20],y21[20],y21[20],y21[20],y21[20],y21[20],y21[20], y21[20: 0]  }, x74); 

 Register29Bit Reg182 (clk,x74, x74_1 );

 Adder29Bit Adder46(clk,  { x74_1[28: 0] },  {y22[24],y22[24],y22[24],y22[24], y22[24: 0]  }, x75); 

 Register29Bit Reg183 (clk,x75, x75_1 );

 Adder29Bit Adder47(clk,  { x75_1[28: 0] },  {y23[23],y23[23],y23[23],y23[23],y23[23], y23[23: 0]  }, x76); 

 Register29Bit Reg184 (clk,x76, x76_1 );

 Adder29Bit Adder48(clk,  { x76_1[28: 0] },  {y24[26],y24[26], y24[26: 0]  }, x77); 

 Register29Bit Reg185 (clk,x77, x77_1 );

 Adder29Bit Adder49(clk,  { x77_1[28: 0] },  {y25[24],y25[24],y25[24],y25[24], y25[24: 0]  }, x78); 

 Register29Bit Reg186 (clk,x78, x78_1 );

 Adder29Bit Adder50(clk,  { x78_1[28: 0] },  {y26[25],y26[25],y26[25], y26[25: 0]  }, x79); 

 Register29Bit Reg187 (clk,x79, x79_1 );

 Adder29Bit Adder51(clk,  { x79_1[28: 0] },  {y27[26],y27[26], y27[26: 0]  }, x80); 

 Register29Bit Reg188 (clk,x80, x80_1 );

 Adder29Bit Adder52(clk,  { x80_1[28: 0] },  {y28[26],y28[26], y28[26: 0]  }, x81); 

 Register29Bit Reg189 (clk,x81, x81_1 );

 Adder29Bit Adder53(clk,  { x81_1[28: 0] },  {y29[27], y29[27: 0]  }, x82); 

 Register29Bit Reg190 (clk,x82, x82_1 );

 Adder29Bit Adder54(clk,  { x82_1[28: 0] },  {y30[26],y30[26], y30[26: 0]  }, x83); 

 Register29Bit Reg191 (clk,x83, x83_1 );

 Adder29Bit Adder55(clk,  { x83_1[28: 0] },  {y31[27], y31[27: 0]  }, x84); 

 Register29Bit Reg192 (clk,x84, x84_1 );

 Adder29Bit Adder56(clk,  { x84_1[28: 0] },  {y32[26],y32[26], y32[26: 0]  }, x85); 

 Register29Bit Reg193 (clk,x85, x85_1 );

 Adder29Bit Adder57(clk,  { x85_1[28: 0] },  {y33[26],y33[26], y33[26: 0]  }, x86); 

 Register29Bit Reg194 (clk,x86, x86_1 );

 Adder29Bit Adder58(clk,  { x86_1[28: 0] },  {y34[25],y34[25],y34[25], y34[25: 0]  }, x87); 

 Register29Bit Reg195 (clk,x87, x87_1 );

 Adder29Bit Adder59(clk,  { x87_1[28: 0] },  {y35[24],y35[24],y35[24],y35[24], y35[24: 0]  }, x88); 

 Register29Bit Reg196 (clk,x88, x88_1 );

 Adder29Bit Adder60(clk,  { x88_1[28: 0] },  {y36[26],y36[26], y36[26: 0]  }, x89); 

 Register29Bit Reg197 (clk,x89, x89_1 );

 Adder29Bit Adder61(clk,  { x89_1[28: 0] },  {y37[23],y37[23],y37[23],y37[23],y37[23], y37[23: 0]  }, x90); 

 Register29Bit Reg198 (clk,x90, x90_1 );

 Adder29Bit Adder62(clk,  { x90_1[28: 0] },  {y38[24],y38[24],y38[24],y38[24], y38[24: 0]  }, x91); 

 Register29Bit Reg199 (clk,x91, x91_1 );

 Adder29Bit Adder63(clk,  { x91_1[28: 0] },  {y39[20],y39[20],y39[20],y39[20],y39[20],y39[20],y39[20],y39[20], y39[20: 0]  }, x92); 

 Register29Bit Reg200 (clk,x92, x92_1 );

 Adder29Bit Adder64(clk,  { x92_1[28: 0] },  {y40[24],y40[24],y40[24],y40[24], y40[24: 0]  }, x93); 

 Register29Bit Reg201 (clk,x93, x93_1 );

 Adder29Bit Adder65(clk,  { x93_1[28: 0] },  {y41[22],y41[22],y41[22],y41[22],y41[22],y41[22], y41[22: 0]  }, x94); 

 Register29Bit Reg202 (clk,x94, x94_1 );

 Adder29Bit Adder66(clk,  { x94_1[28: 0] },  {y42[23],y42[23],y42[23],y42[23],y42[23], y42[23: 0]  }, x95); 

 Register29Bit Reg203 (clk,x95, x95_1 );

 Adder29Bit Adder67(clk,  { x95_1[28: 0] },  {y43[23],y43[23],y43[23],y43[23],y43[23], y43[23: 0]  }, x96); 

 Register29Bit Reg204 (clk,x96, x96_1 );

 Adder29Bit Adder68(clk,  { x96_1[28: 0] },  {y44[23],y44[23],y44[23],y44[23],y44[23], y44[23: 0]  }, x97); 

 Register29Bit Reg205 (clk,x97, x97_1 );

 Adder29Bit Adder69(clk,  { x97_1[28: 0] },  {y45[23],y45[23],y45[23],y45[23],y45[23], y45[23: 0]  }, x98); 

 Register29Bit Reg206 (clk,x98, x98_1 );

 Adder29Bit Adder70(clk,  { x98_1[28: 0] },  {y46[21],y46[21],y46[21],y46[21],y46[21],y46[21],y46[21], y46[21: 0]  }, x99); 

 Register29Bit Reg207 (clk,x99, x99_1 );

 Adder29Bit Adder71(clk,  { x99_1[28: 0] },  {y47[23],y47[23],y47[23],y47[23],y47[23], y47[23: 0]  }, x100); 

 Register29Bit Reg208 (clk,x100, x100_1 );

 Adder29Bit Adder72(clk,  { x100_1[28: 0] },  {y48[20],y48[20],y48[20],y48[20],y48[20],y48[20],y48[20],y48[20], y48[20: 0]  }, x101); 

 Register29Bit Reg209 (clk,x101, x101_1 );

 Adder29Bit Adder73(clk,  { x101_1[28: 0] },  {y49[23],y49[23],y49[23],y49[23],y49[23], y49[23: 0]  }, x102); 

 Register29Bit Reg210 (clk,x102, x102_1 );

 Adder29Bit Adder74(clk,  { x102_1[28: 0] },  {y50[22],y50[22],y50[22],y50[22],y50[22],y50[22], y50[22: 0]  }, x103); 

 Register29Bit Reg211 (clk,x103, x103_1 );

 Adder29Bit Adder75(clk,  { x103_1[28: 0] },  {y51[23],y51[23],y51[23],y51[23],y51[23], y51[23: 0]  }, x104); 

 Register29Bit Reg212 (clk,x104, x104_1 );

 Adder29Bit Adder76(clk,  { x104_1[28: 0] },  {y52[22],y52[22],y52[22],y52[22],y52[22],y52[22], y52[22: 0]  }, x105); 

 Register29Bit Reg213 (clk,x105, x105_1 );

 Adder29Bit Adder77(clk,  { x105_1[28: 0] },  {y53[22],y53[22],y53[22],y53[22],y53[22],y53[22], y53[22: 0]  }, x106); 

 Register29Bit Reg214 (clk,x106, x106_1 );

 Adder29Bit Adder78(clk,  { x106_1[28: 0] },  {y54[23],y54[23],y54[23],y54[23],y54[23], y54[23: 0]  }, x107); 

 Register29Bit Reg215 (clk,x107, x107_1 );

 Adder29Bit Adder79(clk,  { x107_1[28: 0] },  {y55[20],y55[20],y55[20],y55[20],y55[20],y55[20],y55[20],y55[20], y55[20: 0]  }, x108); 

 Register29Bit Reg216 (clk,x108, x108_1 );

 Adder29Bit Adder80(clk,  { x108_1[28: 0] },  {y56[23],y56[23],y56[23],y56[23],y56[23], y56[23: 0]  }, x109); 

 Register29Bit Reg217 (clk,x109, x109_1 );

 Adder29Bit Adder81(clk,  { x109_1[28: 0] },  {y57[19],y57[19],y57[19],y57[19],y57[19],y57[19],y57[19],y57[19],y57[19], y57[19: 0]  }, x110); 

 Register29Bit Reg218 (clk,x110, x110_1 );

 Adder29Bit Adder82(clk,  { x110_1[28: 0] },  {y58[22],y58[22],y58[22],y58[22],y58[22],y58[22], y58[22: 0]  }, x111); 

 Register29Bit Reg219 (clk,x111, x111_1 );

 Adder29Bit Adder83(clk,  { x111_1[28: 0] },  {y59[20],y59[20],y59[20],y59[20],y59[20],y59[20],y59[20],y59[20], y59[20: 0]  }, x112); 

 Register29Bit Reg220 (clk,x112, x112_1 );

 Adder29Bit Adder84(clk,  { x112_1[28: 0] },  {y60[22],y60[22],y60[22],y60[22],y60[22],y60[22], y60[22: 0]  }, Out_Y_wire); 

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



module Negator22Bit (clk, In1, NegOut);
 input clk;
 input [21:0] In1;
 output [21 :0] NegOut;

 reg [21 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule

module Negator24Bit (clk, In1, NegOut);
 input clk;
 input [23:0] In1;
 output [23 :0] NegOut;

 reg [23 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule

module Negator21Bit (clk, In1, NegOut);
 input clk;
 input [20:0] In1;
 output [20 :0] NegOut;

 reg [20 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule

module Negator20Bit (clk, In1, NegOut);
 input clk;
 input [19:0] In1;
 output [19 :0] NegOut;

 reg [19 :0] NegOut; 

always @(posedge clk)
begin
NegOut <= -(In1);
end
endmodule
