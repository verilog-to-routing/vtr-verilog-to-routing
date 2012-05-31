///////////////////////////////////////////////////////////////////////
// File:  CRC33_D264.v                             
// Date:  Tue Sep  9 23:01:08 2003                                                      
//                                                                     
// Copyright (C) 1999-2003 Easics NV.                 
// This source file may be used and distributed without restriction    
// provided that this copyright statement is not removed from the file 
// and that any derivative work contains the original copyright notice
// and the associated disclaimer.
//
// THIS SOURCE FILE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
// WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Purpose: Verilog module containing a synthesizable CRC function
//   * polynomial: p(0 to 33) := "1111111111111111111111111111111111"
//   * data width: 264
//                                                                     
// Info: janz@easics.be (Jan Zegers)                           
//       http://www.easics.com                                  
///////////////////////////////////////////////////////////////////////


module CRC33_D264(Data, CRC, nextCRC33_D264);

  // polynomial: p(0 to 33) := "1111111111111111111111111111111111"
  // data width: 264
  // convention: the first serial data bit is D[263]
    input [263:0] Data;
    input [32:0] CRC;
    output [32:0] nextCRC33_D264;

    wire [263:0] D;
    wire [32:0] C;
    wire [32:0] NewCRC;

assign    D = Data;
assign    C = CRC;

    assign NewCRC[0] = D[239] ^ D[238] ^ D[205] ^ D[204] ^ D[171] ^ D[170] ^ 
                D[137] ^ D[136] ^ D[103] ^ D[102] ^ D[69] ^ D[68] ^ 
                D[35] ^ D[34] ^ D[1] ^ D[0] ^ C[7] ^ C[8];
    assign NewCRC[1] = D[240] ^ D[238] ^ D[206] ^ D[204] ^ D[172] ^ D[170] ^ 
                D[138] ^ D[136] ^ D[104] ^ D[102] ^ D[70] ^ D[68] ^ 
                D[36] ^ D[34] ^ D[2] ^ D[0] ^ C[7] ^ C[9];
    assign NewCRC[2] = D[241] ^ D[238] ^ D[207] ^ D[204] ^ D[173] ^ D[170] ^ 
                D[139] ^ D[136] ^ D[105] ^ D[102] ^ D[71] ^ D[68] ^ 
                D[37] ^ D[34] ^ D[3] ^ D[0] ^ C[7] ^ C[10];
    assign NewCRC[3] = D[242] ^ D[238] ^ D[208] ^ D[204] ^ D[174] ^ D[170] ^ 
                D[140] ^ D[136] ^ D[106] ^ D[102] ^ D[72] ^ D[68] ^ 
                D[38] ^ D[34] ^ D[4] ^ D[0] ^ C[7] ^ C[11];
    assign NewCRC[4] = D[243] ^ D[238] ^ D[209] ^ D[204] ^ D[175] ^ D[170] ^ 
                D[141] ^ D[136] ^ D[107] ^ D[102] ^ D[73] ^ D[68] ^ 
                D[39] ^ D[34] ^ D[5] ^ D[0] ^ C[7] ^ C[12];
    assign NewCRC[5] = D[244] ^ D[238] ^ D[210] ^ D[204] ^ D[176] ^ D[170] ^ 
                D[142] ^ D[136] ^ D[108] ^ D[102] ^ D[74] ^ D[68] ^ 
                D[40] ^ D[34] ^ D[6] ^ D[0] ^ C[7] ^ C[13];
    assign NewCRC[6] = D[245] ^ D[238] ^ D[211] ^ D[204] ^ D[177] ^ D[170] ^ 
                D[143] ^ D[136] ^ D[109] ^ D[102] ^ D[75] ^ D[68] ^ 
                D[41] ^ D[34] ^ D[7] ^ D[0] ^ C[7] ^ C[14];
    assign NewCRC[7] = D[246] ^ D[238] ^ D[212] ^ D[204] ^ D[178] ^ D[170] ^ 
                D[144] ^ D[136] ^ D[110] ^ D[102] ^ D[76] ^ D[68] ^ 
                D[42] ^ D[34] ^ D[8] ^ D[0] ^ C[7] ^ C[15];
    assign NewCRC[8] = D[247] ^ D[238] ^ D[213] ^ D[204] ^ D[179] ^ D[170] ^ 
                D[145] ^ D[136] ^ D[111] ^ D[102] ^ D[77] ^ D[68] ^ 
                D[43] ^ D[34] ^ D[9] ^ D[0] ^ C[7] ^ C[16];
    assign NewCRC[9] = D[248] ^ D[238] ^ D[214] ^ D[204] ^ D[180] ^ D[170] ^ 
                D[146] ^ D[136] ^ D[112] ^ D[102] ^ D[78] ^ D[68] ^ 
                D[44] ^ D[34] ^ D[10] ^ D[0] ^ C[7] ^ C[17];
    assign NewCRC[10] = D[249] ^ D[238] ^ D[215] ^ D[204] ^ D[181] ^ D[170] ^ 
                 D[147] ^ D[136] ^ D[113] ^ D[102] ^ D[79] ^ D[68] ^ 
                 D[45] ^ D[34] ^ D[11] ^ D[0] ^ C[7] ^ C[18];
    assign NewCRC[11] = D[250] ^ D[238] ^ D[216] ^ D[204] ^ D[182] ^ D[170] ^ 
                 D[148] ^ D[136] ^ D[114] ^ D[102] ^ D[80] ^ D[68] ^ 
                 D[46] ^ D[34] ^ D[12] ^ D[0] ^ C[7] ^ C[19];
    assign NewCRC[12] = D[251] ^ D[238] ^ D[217] ^ D[204] ^ D[183] ^ D[170] ^ 
                 D[149] ^ D[136] ^ D[115] ^ D[102] ^ D[81] ^ D[68] ^ 
                 D[47] ^ D[34] ^ D[13] ^ D[0] ^ C[7] ^ C[20];
    assign NewCRC[13] = D[252] ^ D[238] ^ D[218] ^ D[204] ^ D[184] ^ D[170] ^ 
                 D[150] ^ D[136] ^ D[116] ^ D[102] ^ D[82] ^ D[68] ^ 
                 D[48] ^ D[34] ^ D[14] ^ D[0] ^ C[7] ^ C[21];
    assign NewCRC[14] = D[253] ^ D[238] ^ D[219] ^ D[204] ^ D[185] ^ D[170] ^ 
                 D[151] ^ D[136] ^ D[117] ^ D[102] ^ D[83] ^ D[68] ^ 
                 D[49] ^ D[34] ^ D[15] ^ D[0] ^ C[7] ^ C[22];
    assign NewCRC[15] = D[254] ^ D[238] ^ D[220] ^ D[204] ^ D[186] ^ D[170] ^ 
                 D[152] ^ D[136] ^ D[118] ^ D[102] ^ D[84] ^ D[68] ^ 
                 D[50] ^ D[34] ^ D[16] ^ D[0] ^ C[7] ^ C[23];
    assign NewCRC[16] = D[255] ^ D[238] ^ D[221] ^ D[204] ^ D[187] ^ D[170] ^ 
                 D[153] ^ D[136] ^ D[119] ^ D[102] ^ D[85] ^ D[68] ^ 
                 D[51] ^ D[34] ^ D[17] ^ D[0] ^ C[7] ^ C[24];
    assign NewCRC[17] = D[256] ^ D[238] ^ D[222] ^ D[204] ^ D[188] ^ D[170] ^ 
                 D[154] ^ D[136] ^ D[120] ^ D[102] ^ D[86] ^ D[68] ^ 
                 D[52] ^ D[34] ^ D[18] ^ D[0] ^ C[7] ^ C[25];
    assign NewCRC[18] = D[257] ^ D[238] ^ D[223] ^ D[204] ^ D[189] ^ D[170] ^ 
                 D[155] ^ D[136] ^ D[121] ^ D[102] ^ D[87] ^ D[68] ^ 
                 D[53] ^ D[34] ^ D[19] ^ D[0] ^ C[7] ^ C[26];
    assign NewCRC[19] = D[258] ^ D[238] ^ D[224] ^ D[204] ^ D[190] ^ D[170] ^ 
                 D[156] ^ D[136] ^ D[122] ^ D[102] ^ D[88] ^ D[68] ^ 
                 D[54] ^ D[34] ^ D[20] ^ D[0] ^ C[7] ^ C[27];
    assign NewCRC[20] = D[259] ^ D[238] ^ D[225] ^ D[204] ^ D[191] ^ D[170] ^ 
                 D[157] ^ D[136] ^ D[123] ^ D[102] ^ D[89] ^ D[68] ^ 
                 D[55] ^ D[34] ^ D[21] ^ D[0] ^ C[7] ^ C[28];
    assign NewCRC[21] = D[260] ^ D[238] ^ D[226] ^ D[204] ^ D[192] ^ D[170] ^ 
                 D[158] ^ D[136] ^ D[124] ^ D[102] ^ D[90] ^ D[68] ^ 
                 D[56] ^ D[34] ^ D[22] ^ D[0] ^ C[7] ^ C[29];
    assign NewCRC[22] = D[261] ^ D[238] ^ D[227] ^ D[204] ^ D[193] ^ D[170] ^ 
                 D[159] ^ D[136] ^ D[125] ^ D[102] ^ D[91] ^ D[68] ^ 
                 D[57] ^ D[34] ^ D[23] ^ D[0] ^ C[7] ^ C[30];
    assign NewCRC[23] = D[262] ^ D[238] ^ D[228] ^ D[204] ^ D[194] ^ D[170] ^ 
                 D[160] ^ D[136] ^ D[126] ^ D[102] ^ D[92] ^ D[68] ^ 
                 D[58] ^ D[34] ^ D[24] ^ D[0] ^ C[7] ^ C[31];
    assign NewCRC[24] = D[263] ^ D[238] ^ D[229] ^ D[204] ^ D[195] ^ D[170] ^ 
                 D[161] ^ D[136] ^ D[127] ^ D[102] ^ D[93] ^ D[68] ^ 
                 D[59] ^ D[34] ^ D[25] ^ D[0] ^ C[7] ^ C[32];
    assign NewCRC[25] = D[238] ^ D[230] ^ D[204] ^ D[196] ^ D[170] ^ D[162] ^ 
                 D[136] ^ D[128] ^ D[102] ^ D[94] ^ D[68] ^ D[60] ^ 
                 D[34] ^ D[26] ^ D[0] ^ C[7];
    assign NewCRC[26] = D[238] ^ D[231] ^ D[204] ^ D[197] ^ D[170] ^ D[163] ^ 
                 D[136] ^ D[129] ^ D[102] ^ D[95] ^ D[68] ^ D[61] ^ 
                 D[34] ^ D[27] ^ D[0] ^ C[0] ^ C[7];
    assign NewCRC[27] = D[238] ^ D[232] ^ D[204] ^ D[198] ^ D[170] ^ D[164] ^ 
                 D[136] ^ D[130] ^ D[102] ^ D[96] ^ D[68] ^ D[62] ^ 
                 D[34] ^ D[28] ^ D[0] ^ C[1] ^ C[7];
    assign NewCRC[28] = D[238] ^ D[233] ^ D[204] ^ D[199] ^ D[170] ^ D[165] ^ 
                 D[136] ^ D[131] ^ D[102] ^ D[97] ^ D[68] ^ D[63] ^ 
                 D[34] ^ D[29] ^ D[0] ^ C[2] ^ C[7];
    assign NewCRC[29] = D[238] ^ D[234] ^ D[204] ^ D[200] ^ D[170] ^ D[166] ^ 
                 D[136] ^ D[132] ^ D[102] ^ D[98] ^ D[68] ^ D[64] ^ 
                 D[34] ^ D[30] ^ D[0] ^ C[3] ^ C[7];
    assign NewCRC[30] = D[238] ^ D[235] ^ D[204] ^ D[201] ^ D[170] ^ D[167] ^ 
                 D[136] ^ D[133] ^ D[102] ^ D[99] ^ D[68] ^ D[65] ^ 
                 D[34] ^ D[31] ^ D[0] ^ C[4] ^ C[7];
    assign NewCRC[31] = D[238] ^ D[236] ^ D[204] ^ D[202] ^ D[170] ^ D[168] ^ 
                 D[136] ^ D[134] ^ D[102] ^ D[100] ^ D[68] ^ D[66] ^ 
                 D[34] ^ D[32] ^ D[0] ^ C[5] ^ C[7];
    assign NewCRC[32] = D[238] ^ D[237] ^ D[204] ^ D[203] ^ D[170] ^ D[169] ^ 
                 D[136] ^ D[135] ^ D[102] ^ D[101] ^ D[68] ^ D[67] ^ 
                 D[34] ^ D[33] ^ D[0] ^ C[6] ^ C[7];
    assign nextCRC33_D264 = NewCRC;

endmodule

