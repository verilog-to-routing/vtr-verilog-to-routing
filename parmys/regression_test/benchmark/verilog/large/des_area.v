/////////////////////////////////////////////////////////////////////
////                                                             ////
////  KEY_SEL                                                    ////
////  Select one of 16 sub-keys for round                        ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  key_sel(K_sub, K, roundSel, decrypt);
output	[48:1]	K_sub;
input	[55:0]	K;
input	[3:0]	roundSel;
input		decrypt;

reg  [48:1]	K_sub;
wire [48:1]	K1, K2, K3, K4, K5, K6, K7, K8, K9;
wire [48:1]	K10, K11, K12, K13, K14, K15, K16;

always @(K1 or K2 or K3 or K4 or K5 or K6 or K7 or K8 or K9 or K10
              or K11 or K12 or K13 or K14 or K15 or K16 or roundSel)
	case (roundSel)		// synopsys full_case parallel_case
            0:  K_sub = K1;
            1:  K_sub = K2;
            2:  K_sub = K3;
            3:  K_sub = K4;
            4:  K_sub = K5;
            5:  K_sub = K6;
            6:  K_sub = K7;
            7:  K_sub = K8;
            8:  K_sub = K9;
            9:  K_sub = K10;
            10: K_sub = K11;
            11: K_sub = K12;
            12: K_sub = K13;
            13: K_sub = K14;
            14: K_sub = K15;
            15: K_sub = K16;
	endcase


assign K16[1] = decrypt ? K[47] : K[40];
assign K16[2] = decrypt ? K[11] : K[4];
assign K16[3] = decrypt ? K[26] : K[19];
assign K16[4] = decrypt ? K[3] : K[53];
assign K16[5] = decrypt ? K[13] : K[6];
assign K16[6] = decrypt ? K[41] : K[34];
assign K16[7] = decrypt ? K[27] : K[20];
assign K16[8] = decrypt ? K[6] : K[24];
assign K16[9] = decrypt ? K[54] : K[47];
assign K16[10] = decrypt ? K[48] : K[41];
assign K16[11] = decrypt ? K[39] : K[32];
assign K16[12] = decrypt ? K[19] : K[12];
assign K16[13] = decrypt ? K[53] : K[46];
assign K16[14] = decrypt ? K[25] : K[18];
assign K16[15] = decrypt ? K[33] : K[26];
assign K16[16] = decrypt ? K[34] : K[27];
assign K16[17] = decrypt ? K[17] : K[10];
assign K16[18] = decrypt ? K[5] : K[55];
assign K16[19] = decrypt ? K[4] : K[54];
assign K16[20] = decrypt ? K[55] : K[48];
assign K16[21] = decrypt ? K[24] : K[17];
assign K16[22] = decrypt ? K[32] : K[25];
assign K16[23] = decrypt ? K[40] : K[33];
assign K16[24] = decrypt ? K[20] : K[13];
assign K16[25] = decrypt ? K[36] : K[29];
assign K16[26] = decrypt ? K[31] : K[51];
assign K16[27] = decrypt ? K[21] : K[14];
assign K16[28] = decrypt ? K[8] : K[1];
assign K16[29] = decrypt ? K[23] : K[16];
assign K16[30] = decrypt ? K[52] : K[45];
assign K16[31] = decrypt ? K[14] : K[7];
assign K16[32] = decrypt ? K[29] : K[22];
assign K16[33] = decrypt ? K[51] : K[44];
assign K16[34] = decrypt ? K[9] : K[2];
assign K16[35] = decrypt ? K[35] : K[28];
assign K16[36] = decrypt ? K[30] : K[23];
assign K16[37] = decrypt ? K[2] : K[50];
assign K16[38] = decrypt ? K[37] : K[30];
assign K16[39] = decrypt ? K[22] : K[15];
assign K16[40] = decrypt ? K[0] : K[52];
assign K16[41] = decrypt ? K[42] : K[35];
assign K16[42] = decrypt ? K[38] : K[31];
assign K16[43] = decrypt ? K[16] : K[9];
assign K16[44] = decrypt ? K[43] : K[36];
assign K16[45] = decrypt ? K[44] : K[37];
assign K16[46] = decrypt ? K[1] : K[49];
assign K16[47] = decrypt ? K[7] : K[0];
assign K16[48] = decrypt ? K[28] : K[21];

assign K15[1] = decrypt ? K[54] : K[33];
assign K15[2] = decrypt ? K[18] : K[54];
assign K15[3] = decrypt ? K[33] : K[12];
assign K15[4] = decrypt ? K[10] : K[46];
assign K15[5] = decrypt ? K[20] : K[24];
assign K15[6] = decrypt ? K[48] : K[27];
assign K15[7] = decrypt ? K[34] : K[13];
assign K15[8] = decrypt ? K[13] : K[17];
assign K15[9] = decrypt ? K[4] : K[40];
assign K15[10] = decrypt ? K[55] : K[34];
assign K15[11] = decrypt ? K[46] : K[25];
assign K15[12] = decrypt ? K[26] : K[5];
assign K15[13] = decrypt ? K[3] : K[39];
assign K15[14] = decrypt ? K[32] : K[11];
assign K15[15] = decrypt ? K[40] : K[19];
assign K15[16] = decrypt ? K[41] : K[20];
assign K15[17] = decrypt ? K[24] : K[3];
assign K15[18] = decrypt ? K[12] : K[48];
assign K15[19] = decrypt ? K[11] : K[47];
assign K15[20] = decrypt ? K[5] : K[41];
assign K15[21] = decrypt ? K[6] : K[10];
assign K15[22] = decrypt ? K[39] : K[18];
assign K15[23] = decrypt ? K[47] : K[26];
assign K15[24] = decrypt ? K[27] : K[6];
assign K15[25] = decrypt ? K[43] : K[22];
assign K15[26] = decrypt ? K[38] : K[44];
assign K15[27] = decrypt ? K[28] : K[7];
assign K15[28] = decrypt ? K[15] : K[49];
assign K15[29] = decrypt ? K[30] : K[9];
assign K15[30] = decrypt ? K[0] : K[38];
assign K15[31] = decrypt ? K[21] : K[0];
assign K15[32] = decrypt ? K[36] : K[15];
assign K15[33] = decrypt ? K[31] : K[37];
assign K15[34] = decrypt ? K[16] : K[50];
assign K15[35] = decrypt ? K[42] : K[21];
assign K15[36] = decrypt ? K[37] : K[16];
assign K15[37] = decrypt ? K[9] : K[43];
assign K15[38] = decrypt ? K[44] : K[23];
assign K15[39] = decrypt ? K[29] : K[8];
assign K15[40] = decrypt ? K[7] : K[45];
assign K15[41] = decrypt ? K[49] : K[28];
assign K15[42] = decrypt ? K[45] : K[51];
assign K15[43] = decrypt ? K[23] : K[2];
assign K15[44] = decrypt ? K[50] : K[29];
assign K15[45] = decrypt ? K[51] : K[30];
assign K15[46] = decrypt ? K[8] : K[42];
assign K15[47] = decrypt ? K[14] : K[52];
assign K15[48] = decrypt ? K[35] : K[14];

assign K14[1] = decrypt ? K[11] : K[19];
assign K14[2] = decrypt ? K[32] : K[40];
assign K14[3] = decrypt ? K[47] : K[55];
assign K14[4] = decrypt ? K[24] : K[32];
assign K14[5] = decrypt ? K[34] : K[10];
assign K14[6] = decrypt ? K[5] : K[13];
assign K14[7] = decrypt ? K[48] : K[24];
assign K14[8] = decrypt ? K[27] : K[3];
assign K14[9] = decrypt ? K[18] : K[26];
assign K14[10] = decrypt ? K[12] : K[20];
assign K14[11] = decrypt ? K[3] : K[11];
assign K14[12] = decrypt ? K[40] : K[48];
assign K14[13] = decrypt ? K[17] : K[25];
assign K14[14] = decrypt ? K[46] : K[54];
assign K14[15] = decrypt ? K[54] : K[5];
assign K14[16] = decrypt ? K[55] : K[6];
assign K14[17] = decrypt ? K[13] : K[46];
assign K14[18] = decrypt ? K[26] : K[34];
assign K14[19] = decrypt ? K[25] : K[33];
assign K14[20] = decrypt ? K[19] : K[27];
assign K14[21] = decrypt ? K[20] : K[53];
assign K14[22] = decrypt ? K[53] : K[4];
assign K14[23] = decrypt ? K[4] : K[12];
assign K14[24] = decrypt ? K[41] : K[17];
assign K14[25] = decrypt ? K[2] : K[8];
assign K14[26] = decrypt ? K[52] : K[30];
assign K14[27] = decrypt ? K[42] : K[52];
assign K14[28] = decrypt ? K[29] : K[35];
assign K14[29] = decrypt ? K[44] : K[50];
assign K14[30] = decrypt ? K[14] : K[51];
assign K14[31] = decrypt ? K[35] : K[45];
assign K14[32] = decrypt ? K[50] : K[1];
assign K14[33] = decrypt ? K[45] : K[23];
assign K14[34] = decrypt ? K[30] : K[36];
assign K14[35] = decrypt ? K[1] : K[7];
assign K14[36] = decrypt ? K[51] : K[2];
assign K14[37] = decrypt ? K[23] : K[29];
assign K14[38] = decrypt ? K[31] : K[9];
assign K14[39] = decrypt ? K[43] : K[49];
assign K14[40] = decrypt ? K[21] : K[31];
assign K14[41] = decrypt ? K[8] : K[14];
assign K14[42] = decrypt ? K[0] : K[37];
assign K14[43] = decrypt ? K[37] : K[43];
assign K14[44] = decrypt ? K[9] : K[15];
assign K14[45] = decrypt ? K[38] : K[16];
assign K14[46] = decrypt ? K[22] : K[28];
assign K14[47] = decrypt ? K[28] : K[38];
assign K14[48] = decrypt ? K[49] : K[0];

assign K13[1] = decrypt ? K[25] : K[5];
assign K13[2] = decrypt ? K[46] : K[26];
assign K13[3] = decrypt ? K[4] : K[41];
assign K13[4] = decrypt ? K[13] : K[18];
assign K13[5] = decrypt ? K[48] : K[53];
assign K13[6] = decrypt ? K[19] : K[24];
assign K13[7] = decrypt ? K[5] : K[10];
assign K13[8] = decrypt ? K[41] : K[46];
assign K13[9] = decrypt ? K[32] : K[12];
assign K13[10] = decrypt ? K[26] : K[6];
assign K13[11] = decrypt ? K[17] : K[54];
assign K13[12] = decrypt ? K[54] : K[34];
assign K13[13] = decrypt ? K[6] : K[11];
assign K13[14] = decrypt ? K[3] : K[40];
assign K13[15] = decrypt ? K[11] : K[48];
assign K13[16] = decrypt ? K[12] : K[17];
assign K13[17] = decrypt ? K[27] : K[32];
assign K13[18] = decrypt ? K[40] : K[20];
assign K13[19] = decrypt ? K[39] : K[19];
assign K13[20] = decrypt ? K[33] : K[13];
assign K13[21] = decrypt ? K[34] : K[39];
assign K13[22] = decrypt ? K[10] : K[47];
assign K13[23] = decrypt ? K[18] : K[55];
assign K13[24] = decrypt ? K[55] : K[3];
assign K13[25] = decrypt ? K[16] : K[49];
assign K13[26] = decrypt ? K[7] : K[16];
assign K13[27] = decrypt ? K[1] : K[38];
assign K13[28] = decrypt ? K[43] : K[21];
assign K13[29] = decrypt ? K[31] : K[36];
assign K13[30] = decrypt ? K[28] : K[37];
assign K13[31] = decrypt ? K[49] : K[31];
assign K13[32] = decrypt ? K[9] : K[42];
assign K13[33] = decrypt ? K[0] : K[9];
assign K13[34] = decrypt ? K[44] : K[22];
assign K13[35] = decrypt ? K[15] : K[52];
assign K13[36] = decrypt ? K[38] : K[43];
assign K13[37] = decrypt ? K[37] : K[15];
assign K13[38] = decrypt ? K[45] : K[50];
assign K13[39] = decrypt ? K[2] : K[35];
assign K13[40] = decrypt ? K[35] : K[44];
assign K13[41] = decrypt ? K[22] : K[0];
assign K13[42] = decrypt ? K[14] : K[23];
assign K13[43] = decrypt ? K[51] : K[29];
assign K13[44] = decrypt ? K[23] : K[1];
assign K13[45] = decrypt ? K[52] : K[2];
assign K13[46] = decrypt ? K[36] : K[14];
assign K13[47] = decrypt ? K[42] : K[51];
assign K13[48] = decrypt ? K[8] : K[45];

assign K12[1] = decrypt ? K[39] : K[48];
assign K12[2] = decrypt ? K[3] : K[12];
assign K12[3] = decrypt ? K[18] : K[27];
assign K12[4] = decrypt ? K[27] : K[4];
assign K12[5] = decrypt ? K[5] : K[39];
assign K12[6] = decrypt ? K[33] : K[10];
assign K12[7] = decrypt ? K[19] : K[53];
assign K12[8] = decrypt ? K[55] : K[32];
assign K12[9] = decrypt ? K[46] : K[55];
assign K12[10] = decrypt ? K[40] : K[17];
assign K12[11] = decrypt ? K[6] : K[40];
assign K12[12] = decrypt ? K[11] : K[20];
assign K12[13] = decrypt ? K[20] : K[54];
assign K12[14] = decrypt ? K[17] : K[26];
assign K12[15] = decrypt ? K[25] : K[34];
assign K12[16] = decrypt ? K[26] : K[3];
assign K12[17] = decrypt ? K[41] : K[18];
assign K12[18] = decrypt ? K[54] : K[6];
assign K12[19] = decrypt ? K[53] : K[5];
assign K12[20] = decrypt ? K[47] : K[24];
assign K12[21] = decrypt ? K[48] : K[25];
assign K12[22] = decrypt ? K[24] : K[33];
assign K12[23] = decrypt ? K[32] : K[41];
assign K12[24] = decrypt ? K[12] : K[46];
assign K12[25] = decrypt ? K[30] : K[35];
assign K12[26] = decrypt ? K[21] : K[2];
assign K12[27] = decrypt ? K[15] : K[51];
assign K12[28] = decrypt ? K[2] : K[7];
assign K12[29] = decrypt ? K[45] : K[22];
assign K12[30] = decrypt ? K[42] : K[23];
assign K12[31] = decrypt ? K[8] : K[44];
assign K12[32] = decrypt ? K[23] : K[28];
assign K12[33] = decrypt ? K[14] : K[50];
assign K12[34] = decrypt ? K[31] : K[8];
assign K12[35] = decrypt ? K[29] : K[38];
assign K12[36] = decrypt ? K[52] : K[29];
assign K12[37] = decrypt ? K[51] : K[1];
assign K12[38] = decrypt ? K[0] : K[36];
assign K12[39] = decrypt ? K[16] : K[21];
assign K12[40] = decrypt ? K[49] : K[30];
assign K12[41] = decrypt ? K[36] : K[45];
assign K12[42] = decrypt ? K[28] : K[9];
assign K12[43] = decrypt ? K[38] : K[15];
assign K12[44] = decrypt ? K[37] : K[42];
assign K12[45] = decrypt ? K[7] : K[43];
assign K12[46] = decrypt ? K[50] : K[0];
assign K12[47] = decrypt ? K[1] : K[37];
assign K12[48] = decrypt ? K[22] : K[31];

assign K11[1] = decrypt ? K[53] : K[34];
assign K11[2] = decrypt ? K[17] : K[55];
assign K11[3] = decrypt ? K[32] : K[13];
assign K11[4] = decrypt ? K[41] : K[47];
assign K11[5] = decrypt ? K[19] : K[25];
assign K11[6] = decrypt ? K[47] : K[53];
assign K11[7] = decrypt ? K[33] : K[39];
assign K11[8] = decrypt ? K[12] : K[18];
assign K11[9] = decrypt ? K[3] : K[41];
assign K11[10] = decrypt ? K[54] : K[3];
assign K11[11] = decrypt ? K[20] : K[26];
assign K11[12] = decrypt ? K[25] : K[6];
assign K11[13] = decrypt ? K[34] : K[40];
assign K11[14] = decrypt ? K[6] : K[12];
assign K11[15] = decrypt ? K[39] : K[20];
assign K11[16] = decrypt ? K[40] : K[46];
assign K11[17] = decrypt ? K[55] : K[4];
assign K11[18] = decrypt ? K[11] : K[17];
assign K11[19] = decrypt ? K[10] : K[48];
assign K11[20] = decrypt ? K[4] : K[10];
assign K11[21] = decrypt ? K[5] : K[11];
assign K11[22] = decrypt ? K[13] : K[19];
assign K11[23] = decrypt ? K[46] : K[27];
assign K11[24] = decrypt ? K[26] : K[32];
assign K11[25] = decrypt ? K[44] : K[21];
assign K11[26] = decrypt ? K[35] : K[43];
assign K11[27] = decrypt ? K[29] : K[37];
assign K11[28] = decrypt ? K[16] : K[52];
assign K11[29] = decrypt ? K[0] : K[8];
assign K11[30] = decrypt ? K[1] : K[9];
assign K11[31] = decrypt ? K[22] : K[30];
assign K11[32] = decrypt ? K[37] : K[14];
assign K11[33] = decrypt ? K[28] : K[36];
assign K11[34] = decrypt ? K[45] : K[49];
assign K11[35] = decrypt ? K[43] : K[51];
assign K11[36] = decrypt ? K[7] : K[15];
assign K11[37] = decrypt ? K[38] : K[42];
assign K11[38] = decrypt ? K[14] : K[22];
assign K11[39] = decrypt ? K[30] : K[7];
assign K11[40] = decrypt ? K[8] : K[16];
assign K11[41] = decrypt ? K[50] : K[31];
assign K11[42] = decrypt ? K[42] : K[50];
assign K11[43] = decrypt ? K[52] : K[1];
assign K11[44] = decrypt ? K[51] : K[28];
assign K11[45] = decrypt ? K[21] : K[29];
assign K11[46] = decrypt ? K[9] : K[45];
assign K11[47] = decrypt ? K[15] : K[23];
assign K11[48] = decrypt ? K[36] : K[44];

assign K10[1] = decrypt ? K[10] : K[20];
assign K10[2] = decrypt ? K[6] : K[41];
assign K10[3] = decrypt ? K[46] : K[24];
assign K10[4] = decrypt ? K[55] : K[33];
assign K10[5] = decrypt ? K[33] : K[11];
assign K10[6] = decrypt ? K[4] : K[39];
assign K10[7] = decrypt ? K[47] : K[25];
assign K10[8] = decrypt ? K[26] : K[4];
assign K10[9] = decrypt ? K[17] : K[27];
assign K10[10] = decrypt ? K[11] : K[46];
assign K10[11] = decrypt ? K[34] : K[12];
assign K10[12] = decrypt ? K[39] : K[17];
assign K10[13] = decrypt ? K[48] : K[26];
assign K10[14] = decrypt ? K[20] : K[55];
assign K10[15] = decrypt ? K[53] : K[6];
assign K10[16] = decrypt ? K[54] : K[32];
assign K10[17] = decrypt ? K[12] : K[47];
assign K10[18] = decrypt ? K[25] : K[3];
assign K10[19] = decrypt ? K[24] : K[34];
assign K10[20] = decrypt ? K[18] : K[53];
assign K10[21] = decrypt ? K[19] : K[54];
assign K10[22] = decrypt ? K[27] : K[5];
assign K10[23] = decrypt ? K[3] : K[13];
assign K10[24] = decrypt ? K[40] : K[18];
assign K10[25] = decrypt ? K[31] : K[7];
assign K10[26] = decrypt ? K[49] : K[29];
assign K10[27] = decrypt ? K[43] : K[23];
assign K10[28] = decrypt ? K[30] : K[38];
assign K10[29] = decrypt ? K[14] : K[49];
assign K10[30] = decrypt ? K[15] : K[50];
assign K10[31] = decrypt ? K[36] : K[16];
assign K10[32] = decrypt ? K[51] : K[0];
assign K10[33] = decrypt ? K[42] : K[22];
assign K10[34] = decrypt ? K[0] : K[35];
assign K10[35] = decrypt ? K[2] : K[37];
assign K10[36] = decrypt ? K[21] : K[1];
assign K10[37] = decrypt ? K[52] : K[28];
assign K10[38] = decrypt ? K[28] : K[8];
assign K10[39] = decrypt ? K[44] : K[52];
assign K10[40] = decrypt ? K[22] : K[2];
assign K10[41] = decrypt ? K[9] : K[44];
assign K10[42] = decrypt ? K[1] : K[36];
assign K10[43] = decrypt ? K[7] : K[42];
assign K10[44] = decrypt ? K[38] : K[14];
assign K10[45] = decrypt ? K[35] : K[15];
assign K10[46] = decrypt ? K[23] : K[31];
assign K10[47] = decrypt ? K[29] : K[9];
assign K10[48] = decrypt ? K[50] : K[30];

assign K9[1] = decrypt ? K[24] : K[6];
assign K9[2] = decrypt ? K[20] : K[27];
assign K9[3] = decrypt ? K[3] : K[10];
assign K9[4] = decrypt ? K[12] : K[19];
assign K9[5] = decrypt ? K[47] : K[54];
assign K9[6] = decrypt ? K[18] : K[25];
assign K9[7] = decrypt ? K[4] : K[11];
assign K9[8] = decrypt ? K[40] : K[47];
assign K9[9] = decrypt ? K[6] : K[13];
assign K9[10] = decrypt ? K[25] : K[32];
assign K9[11] = decrypt ? K[48] : K[55];
assign K9[12] = decrypt ? K[53] : K[3];
assign K9[13] = decrypt ? K[5] : K[12];
assign K9[14] = decrypt ? K[34] : K[41];
assign K9[15] = decrypt ? K[10] : K[17];
assign K9[16] = decrypt ? K[11] : K[18];
assign K9[17] = decrypt ? K[26] : K[33];
assign K9[18] = decrypt ? K[39] : K[46];
assign K9[19] = decrypt ? K[13] : K[20];
assign K9[20] = decrypt ? K[32] : K[39];
assign K9[21] = decrypt ? K[33] : K[40];
assign K9[22] = decrypt ? K[41] : K[48];
assign K9[23] = decrypt ? K[17] : K[24];
assign K9[24] = decrypt ? K[54] : K[4];
assign K9[25] = decrypt ? K[45] : K[52];
assign K9[26] = decrypt ? K[8] : K[15];
assign K9[27] = decrypt ? K[2] : K[9];
assign K9[28] = decrypt ? K[44] : K[51];
assign K9[29] = decrypt ? K[28] : K[35];
assign K9[30] = decrypt ? K[29] : K[36];
assign K9[31] = decrypt ? K[50] : K[2];
assign K9[32] = decrypt ? K[38] : K[45];
assign K9[33] = decrypt ? K[1] : K[8];
assign K9[34] = decrypt ? K[14] : K[21];
assign K9[35] = decrypt ? K[16] : K[23];
assign K9[36] = decrypt ? K[35] : K[42];
assign K9[37] = decrypt ? K[7] : K[14];
assign K9[38] = decrypt ? K[42] : K[49];
assign K9[39] = decrypt ? K[31] : K[38];
assign K9[40] = decrypt ? K[36] : K[43];
assign K9[41] = decrypt ? K[23] : K[30];
assign K9[42] = decrypt ? K[15] : K[22];
assign K9[43] = decrypt ? K[21] : K[28];
assign K9[44] = decrypt ? K[52] : K[0];
assign K9[45] = decrypt ? K[49] : K[1];
assign K9[46] = decrypt ? K[37] : K[44];
assign K9[47] = decrypt ? K[43] : K[50];
assign K9[48] = decrypt ? K[9] : K[16];

assign K8[1] = decrypt ? K[6] : K[24];
assign K8[2] = decrypt ? K[27] : K[20];
assign K8[3] = decrypt ? K[10] : K[3];
assign K8[4] = decrypt ? K[19] : K[12];
assign K8[5] = decrypt ? K[54] : K[47];
assign K8[6] = decrypt ? K[25] : K[18];
assign K8[7] = decrypt ? K[11] : K[4];
assign K8[8] = decrypt ? K[47] : K[40];
assign K8[9] = decrypt ? K[13] : K[6];
assign K8[10] = decrypt ? K[32] : K[25];
assign K8[11] = decrypt ? K[55] : K[48];
assign K8[12] = decrypt ? K[3] : K[53];
assign K8[13] = decrypt ? K[12] : K[5];
assign K8[14] = decrypt ? K[41] : K[34];
assign K8[15] = decrypt ? K[17] : K[10];
assign K8[16] = decrypt ? K[18] : K[11];
assign K8[17] = decrypt ? K[33] : K[26];
assign K8[18] = decrypt ? K[46] : K[39];
assign K8[19] = decrypt ? K[20] : K[13];
assign K8[20] = decrypt ? K[39] : K[32];
assign K8[21] = decrypt ? K[40] : K[33];
assign K8[22] = decrypt ? K[48] : K[41];
assign K8[23] = decrypt ? K[24] : K[17];
assign K8[24] = decrypt ? K[4] : K[54];
assign K8[25] = decrypt ? K[52] : K[45];
assign K8[26] = decrypt ? K[15] : K[8];
assign K8[27] = decrypt ? K[9] : K[2];
assign K8[28] = decrypt ? K[51] : K[44];
assign K8[29] = decrypt ? K[35] : K[28];
assign K8[30] = decrypt ? K[36] : K[29];
assign K8[31] = decrypt ? K[2] : K[50];
assign K8[32] = decrypt ? K[45] : K[38];
assign K8[33] = decrypt ? K[8] : K[1];
assign K8[34] = decrypt ? K[21] : K[14];
assign K8[35] = decrypt ? K[23] : K[16];
assign K8[36] = decrypt ? K[42] : K[35];
assign K8[37] = decrypt ? K[14] : K[7];
assign K8[38] = decrypt ? K[49] : K[42];
assign K8[39] = decrypt ? K[38] : K[31];
assign K8[40] = decrypt ? K[43] : K[36];
assign K8[41] = decrypt ? K[30] : K[23];
assign K8[42] = decrypt ? K[22] : K[15];
assign K8[43] = decrypt ? K[28] : K[21];
assign K8[44] = decrypt ? K[0] : K[52];
assign K8[45] = decrypt ? K[1] : K[49];
assign K8[46] = decrypt ? K[44] : K[37];
assign K8[47] = decrypt ? K[50] : K[43];
assign K8[48] = decrypt ? K[16] : K[9];
   
assign K7[1] = decrypt ? K[20] : K[10];
assign K7[2] = decrypt ? K[41] : K[6];
assign K7[3] = decrypt ? K[24] : K[46];
assign K7[4] = decrypt ? K[33] : K[55];
assign K7[5] = decrypt ? K[11] : K[33];
assign K7[6] = decrypt ? K[39] : K[4];
assign K7[7] = decrypt ? K[25] : K[47];
assign K7[8] = decrypt ? K[4] : K[26];
assign K7[9] = decrypt ? K[27] : K[17];
assign K7[10] = decrypt ? K[46] : K[11];
assign K7[11] = decrypt ? K[12] : K[34];
assign K7[12] = decrypt ? K[17] : K[39];
assign K7[13] = decrypt ? K[26] : K[48];
assign K7[14] = decrypt ? K[55] : K[20];
assign K7[15] = decrypt ? K[6] : K[53];
assign K7[16] = decrypt ? K[32] : K[54];
assign K7[17] = decrypt ? K[47] : K[12];
assign K7[18] = decrypt ? K[3] : K[25];
assign K7[19] = decrypt ? K[34] : K[24];
assign K7[20] = decrypt ? K[53] : K[18];
assign K7[21] = decrypt ? K[54] : K[19];
assign K7[22] = decrypt ? K[5] : K[27];
assign K7[23] = decrypt ? K[13] : K[3];
assign K7[24] = decrypt ? K[18] : K[40];
assign K7[25] = decrypt ? K[7] : K[31];
assign K7[26] = decrypt ? K[29] : K[49];
assign K7[27] = decrypt ? K[23] : K[43];
assign K7[28] = decrypt ? K[38] : K[30];
assign K7[29] = decrypt ? K[49] : K[14];
assign K7[30] = decrypt ? K[50] : K[15];
assign K7[31] = decrypt ? K[16] : K[36];
assign K7[32] = decrypt ? K[0] : K[51];
assign K7[33] = decrypt ? K[22] : K[42];
assign K7[34] = decrypt ? K[35] : K[0];
assign K7[35] = decrypt ? K[37] : K[2];
assign K7[36] = decrypt ? K[1] : K[21];
assign K7[37] = decrypt ? K[28] : K[52];
assign K7[38] = decrypt ? K[8] : K[28];
assign K7[39] = decrypt ? K[52] : K[44];
assign K7[40] = decrypt ? K[2] : K[22];
assign K7[41] = decrypt ? K[44] : K[9];
assign K7[42] = decrypt ? K[36] : K[1];
assign K7[43] = decrypt ? K[42] : K[7];
assign K7[44] = decrypt ? K[14] : K[38];
assign K7[45] = decrypt ? K[15] : K[35];
assign K7[46] = decrypt ? K[31] : K[23];
assign K7[47] = decrypt ? K[9] : K[29];
assign K7[48] = decrypt ? K[30] : K[50];

assign K6[1] = decrypt ? K[34] : K[53];
assign K6[2] = decrypt ? K[55] : K[17];
assign K6[3] = decrypt ? K[13] : K[32];
assign K6[4] = decrypt ? K[47] : K[41];
assign K6[5] = decrypt ? K[25] : K[19];
assign K6[6] = decrypt ? K[53] : K[47];
assign K6[7] = decrypt ? K[39] : K[33];
assign K6[8] = decrypt ? K[18] : K[12];
assign K6[9] = decrypt ? K[41] : K[3];
assign K6[10] = decrypt ? K[3] : K[54];
assign K6[11] = decrypt ? K[26] : K[20];
assign K6[12] = decrypt ? K[6] : K[25];
assign K6[13] = decrypt ? K[40] : K[34];
assign K6[14] = decrypt ? K[12] : K[6];
assign K6[15] = decrypt ? K[20] : K[39];
assign K6[16] = decrypt ? K[46] : K[40];
assign K6[17] = decrypt ? K[4] : K[55];
assign K6[18] = decrypt ? K[17] : K[11];
assign K6[19] = decrypt ? K[48] : K[10];
assign K6[20] = decrypt ? K[10] : K[4];
assign K6[21] = decrypt ? K[11] : K[5];
assign K6[22] = decrypt ? K[19] : K[13];
assign K6[23] = decrypt ? K[27] : K[46];
assign K6[24] = decrypt ? K[32] : K[26];
assign K6[25] = decrypt ? K[21] : K[44];
assign K6[26] = decrypt ? K[43] : K[35];
assign K6[27] = decrypt ? K[37] : K[29];
assign K6[28] = decrypt ? K[52] : K[16];
assign K6[29] = decrypt ? K[8] : K[0];
assign K6[30] = decrypt ? K[9] : K[1];
assign K6[31] = decrypt ? K[30] : K[22];
assign K6[32] = decrypt ? K[14] : K[37];
assign K6[33] = decrypt ? K[36] : K[28];
assign K6[34] = decrypt ? K[49] : K[45];
assign K6[35] = decrypt ? K[51] : K[43];
assign K6[36] = decrypt ? K[15] : K[7];
assign K6[37] = decrypt ? K[42] : K[38];
assign K6[38] = decrypt ? K[22] : K[14];
assign K6[39] = decrypt ? K[7] : K[30];
assign K6[40] = decrypt ? K[16] : K[8];
assign K6[41] = decrypt ? K[31] : K[50];
assign K6[42] = decrypt ? K[50] : K[42];
assign K6[43] = decrypt ? K[1] : K[52];
assign K6[44] = decrypt ? K[28] : K[51];
assign K6[45] = decrypt ? K[29] : K[21];
assign K6[46] = decrypt ? K[45] : K[9];
assign K6[47] = decrypt ? K[23] : K[15];
assign K6[48] = decrypt ? K[44] : K[36];

assign K5[1] = decrypt ? K[48] : K[39];
assign K5[2] = decrypt ? K[12] : K[3];
assign K5[3] = decrypt ? K[27] : K[18];
assign K5[4] = decrypt ? K[4] : K[27];
assign K5[5] = decrypt ? K[39] : K[5];
assign K5[6] = decrypt ? K[10] : K[33];
assign K5[7] = decrypt ? K[53] : K[19];
assign K5[8] = decrypt ? K[32] : K[55];
assign K5[9] = decrypt ? K[55] : K[46];
assign K5[10] = decrypt ? K[17] : K[40];
assign K5[11] = decrypt ? K[40] : K[6];
assign K5[12] = decrypt ? K[20] : K[11];
assign K5[13] = decrypt ? K[54] : K[20];
assign K5[14] = decrypt ? K[26] : K[17];
assign K5[15] = decrypt ? K[34] : K[25];
assign K5[16] = decrypt ? K[3] : K[26];
assign K5[17] = decrypt ? K[18] : K[41];
assign K5[18] = decrypt ? K[6] : K[54];
assign K5[19] = decrypt ? K[5] : K[53];
assign K5[20] = decrypt ? K[24] : K[47];
assign K5[21] = decrypt ? K[25] : K[48];
assign K5[22] = decrypt ? K[33] : K[24];
assign K5[23] = decrypt ? K[41] : K[32];
assign K5[24] = decrypt ? K[46] : K[12];
assign K5[25] = decrypt ? K[35] : K[30];
assign K5[26] = decrypt ? K[2] : K[21];
assign K5[27] = decrypt ? K[51] : K[15];
assign K5[28] = decrypt ? K[7] : K[2];
assign K5[29] = decrypt ? K[22] : K[45];
assign K5[30] = decrypt ? K[23] : K[42];
assign K5[31] = decrypt ? K[44] : K[8];
assign K5[32] = decrypt ? K[28] : K[23];
assign K5[33] = decrypt ? K[50] : K[14];
assign K5[34] = decrypt ? K[8] : K[31];
assign K5[35] = decrypt ? K[38] : K[29];
assign K5[36] = decrypt ? K[29] : K[52];
assign K5[37] = decrypt ? K[1] : K[51];
assign K5[38] = decrypt ? K[36] : K[0];
assign K5[39] = decrypt ? K[21] : K[16];
assign K5[40] = decrypt ? K[30] : K[49];
assign K5[41] = decrypt ? K[45] : K[36];
assign K5[42] = decrypt ? K[9] : K[28];
assign K5[43] = decrypt ? K[15] : K[38];
assign K5[44] = decrypt ? K[42] : K[37];
assign K5[45] = decrypt ? K[43] : K[7];
assign K5[46] = decrypt ? K[0] : K[50];
assign K5[47] = decrypt ? K[37] : K[1];
assign K5[48] = decrypt ? K[31] : K[22];

assign K4[1] = decrypt ? K[5] : K[25];
assign K4[2] = decrypt ? K[26] : K[46];
assign K4[3] = decrypt ? K[41] : K[4];
assign K4[4] = decrypt ? K[18] : K[13];
assign K4[5] = decrypt ? K[53] : K[48];
assign K4[6] = decrypt ? K[24] : K[19];
assign K4[7] = decrypt ? K[10] : K[5];
assign K4[8] = decrypt ? K[46] : K[41];
assign K4[9] = decrypt ? K[12] : K[32];
assign K4[10] = decrypt ? K[6] : K[26];
assign K4[11] = decrypt ? K[54] : K[17];
assign K4[12] = decrypt ? K[34] : K[54];
assign K4[13] = decrypt ? K[11] : K[6];
assign K4[14] = decrypt ? K[40] : K[3];
assign K4[15] = decrypt ? K[48] : K[11];
assign K4[16] = decrypt ? K[17] : K[12];
assign K4[17] = decrypt ? K[32] : K[27];
assign K4[18] = decrypt ? K[20] : K[40];
assign K4[19] = decrypt ? K[19] : K[39];
assign K4[20] = decrypt ? K[13] : K[33];
assign K4[21] = decrypt ? K[39] : K[34];
assign K4[22] = decrypt ? K[47] : K[10];
assign K4[23] = decrypt ? K[55] : K[18];
assign K4[24] = decrypt ? K[3] : K[55];
assign K4[25] = decrypt ? K[49] : K[16];
assign K4[26] = decrypt ? K[16] : K[7];
assign K4[27] = decrypt ? K[38] : K[1];
assign K4[28] = decrypt ? K[21] : K[43];
assign K4[29] = decrypt ? K[36] : K[31];
assign K4[30] = decrypt ? K[37] : K[28];
assign K4[31] = decrypt ? K[31] : K[49];
assign K4[32] = decrypt ? K[42] : K[9];
assign K4[33] = decrypt ? K[9] : K[0];
assign K4[34] = decrypt ? K[22] : K[44];
assign K4[35] = decrypt ? K[52] : K[15];
assign K4[36] = decrypt ? K[43] : K[38];
assign K4[37] = decrypt ? K[15] : K[37];
assign K4[38] = decrypt ? K[50] : K[45];
assign K4[39] = decrypt ? K[35] : K[2];
assign K4[40] = decrypt ? K[44] : K[35];
assign K4[41] = decrypt ? K[0] : K[22];
assign K4[42] = decrypt ? K[23] : K[14];
assign K4[43] = decrypt ? K[29] : K[51];
assign K4[44] = decrypt ? K[1] : K[23];
assign K4[45] = decrypt ? K[2] : K[52];
assign K4[46] = decrypt ? K[14] : K[36];
assign K4[47] = decrypt ? K[51] : K[42];
assign K4[48] = decrypt ? K[45] : K[8];

assign K3[1] = decrypt ? K[19] : K[11];
assign K3[2] = decrypt ? K[40] : K[32];
assign K3[3] = decrypt ? K[55] : K[47];
assign K3[4] = decrypt ? K[32] : K[24];
assign K3[5] = decrypt ? K[10] : K[34];
assign K3[6] = decrypt ? K[13] : K[5];
assign K3[7] = decrypt ? K[24] : K[48];
assign K3[8] = decrypt ? K[3] : K[27];
assign K3[9] = decrypt ? K[26] : K[18];
assign K3[10] = decrypt ? K[20] : K[12];
assign K3[11] = decrypt ? K[11] : K[3];
assign K3[12] = decrypt ? K[48] : K[40];
assign K3[13] = decrypt ? K[25] : K[17];
assign K3[14] = decrypt ? K[54] : K[46];
assign K3[15] = decrypt ? K[5] : K[54];
assign K3[16] = decrypt ? K[6] : K[55];
assign K3[17] = decrypt ? K[46] : K[13];
assign K3[18] = decrypt ? K[34] : K[26];
assign K3[19] = decrypt ? K[33] : K[25];
assign K3[20] = decrypt ? K[27] : K[19];
assign K3[21] = decrypt ? K[53] : K[20];
assign K3[22] = decrypt ? K[4] : K[53];
assign K3[23] = decrypt ? K[12] : K[4];
assign K3[24] = decrypt ? K[17] : K[41];
assign K3[25] = decrypt ? K[8] : K[2];
assign K3[26] = decrypt ? K[30] : K[52];
assign K3[27] = decrypt ? K[52] : K[42];
assign K3[28] = decrypt ? K[35] : K[29];
assign K3[29] = decrypt ? K[50] : K[44];
assign K3[30] = decrypt ? K[51] : K[14];
assign K3[31] = decrypt ? K[45] : K[35];
assign K3[32] = decrypt ? K[1] : K[50];
assign K3[33] = decrypt ? K[23] : K[45];
assign K3[34] = decrypt ? K[36] : K[30];
assign K3[35] = decrypt ? K[7] : K[1];
assign K3[36] = decrypt ? K[2] : K[51];
assign K3[37] = decrypt ? K[29] : K[23];
assign K3[38] = decrypt ? K[9] : K[31];
assign K3[39] = decrypt ? K[49] : K[43];
assign K3[40] = decrypt ? K[31] : K[21];
assign K3[41] = decrypt ? K[14] : K[8];
assign K3[42] = decrypt ? K[37] : K[0];
assign K3[43] = decrypt ? K[43] : K[37];
assign K3[44] = decrypt ? K[15] : K[9];
assign K3[45] = decrypt ? K[16] : K[38];
assign K3[46] = decrypt ? K[28] : K[22];
assign K3[47] = decrypt ? K[38] : K[28];
assign K3[48] = decrypt ? K[0] : K[49];

assign K2[1] = decrypt ? K[33] : K[54];
assign K2[2] = decrypt ? K[54] : K[18];
assign K2[3] = decrypt ? K[12] : K[33];
assign K2[4] = decrypt ? K[46] : K[10];
assign K2[5] = decrypt ? K[24] : K[20];
assign K2[6] = decrypt ? K[27] : K[48];
assign K2[7] = decrypt ? K[13] : K[34];
assign K2[8] = decrypt ? K[17] : K[13];
assign K2[9] = decrypt ? K[40] : K[4];
assign K2[10] = decrypt ? K[34] : K[55];
assign K2[11] = decrypt ? K[25] : K[46];
assign K2[12] = decrypt ? K[5] : K[26];
assign K2[13] = decrypt ? K[39] : K[3];
assign K2[14] = decrypt ? K[11] : K[32];
assign K2[15] = decrypt ? K[19] : K[40];
assign K2[16] = decrypt ? K[20] : K[41];
assign K2[17] = decrypt ? K[3] : K[24];
assign K2[18] = decrypt ? K[48] : K[12];
assign K2[19] = decrypt ? K[47] : K[11];
assign K2[20] = decrypt ? K[41] : K[5];
assign K2[21] = decrypt ? K[10] : K[6];
assign K2[22] = decrypt ? K[18] : K[39];
assign K2[23] = decrypt ? K[26] : K[47];
assign K2[24] = decrypt ? K[6] : K[27];
assign K2[25] = decrypt ? K[22] : K[43];
assign K2[26] = decrypt ? K[44] : K[38];
assign K2[27] = decrypt ? K[7] : K[28];
assign K2[28] = decrypt ? K[49] : K[15];
assign K2[29] = decrypt ? K[9] : K[30];
assign K2[30] = decrypt ? K[38] : K[0];
assign K2[31] = decrypt ? K[0] : K[21];
assign K2[32] = decrypt ? K[15] : K[36];
assign K2[33] = decrypt ? K[37] : K[31];
assign K2[34] = decrypt ? K[50] : K[16];
assign K2[35] = decrypt ? K[21] : K[42];
assign K2[36] = decrypt ? K[16] : K[37];
assign K2[37] = decrypt ? K[43] : K[9];
assign K2[38] = decrypt ? K[23] : K[44];
assign K2[39] = decrypt ? K[8] : K[29];
assign K2[40] = decrypt ? K[45] : K[7];
assign K2[41] = decrypt ? K[28] : K[49];
assign K2[42] = decrypt ? K[51] : K[45];
assign K2[43] = decrypt ? K[2] : K[23];
assign K2[44] = decrypt ? K[29] : K[50];
assign K2[45] = decrypt ? K[30] : K[51];
assign K2[46] = decrypt ? K[42] : K[8];
assign K2[47] = decrypt ? K[52] : K[14];
assign K2[48] = decrypt ? K[14] : K[35];

assign K1[1] = decrypt ? K[40]  : K[47];
assign K1[2] = decrypt ? K[4]   : K[11];
assign K1[3] = decrypt ? K[19]  : K[26];
assign K1[4] = decrypt ? K[53]  : K[3];
assign K1[5] = decrypt ? K[6]   : K[13];
assign K1[6] = decrypt ? K[34]  : K[41];
assign K1[7] = decrypt ? K[20]  : K[27];
assign K1[8] = decrypt ? K[24]  : K[6];
assign K1[9] = decrypt ? K[47]  : K[54];
assign K1[10] = decrypt ? K[41] : K[48];
assign K1[11] = decrypt ? K[32] : K[39];
assign K1[12] = decrypt ? K[12] : K[19];
assign K1[13] = decrypt ? K[46] : K[53];
assign K1[14] = decrypt ? K[18] : K[25];
assign K1[15] = decrypt ? K[26] : K[33];
assign K1[16] = decrypt ? K[27] : K[34];
assign K1[17] = decrypt ? K[10] : K[17];
assign K1[18] = decrypt ? K[55] : K[5];
assign K1[19] = decrypt ? K[54] : K[4];
assign K1[20] = decrypt ? K[48] : K[55];
assign K1[21] = decrypt ? K[17] : K[24];
assign K1[22] = decrypt ? K[25] : K[32];
assign K1[23] = decrypt ? K[33] : K[40];
assign K1[24] = decrypt ? K[13] : K[20];
assign K1[25] = decrypt ? K[29] : K[36];
assign K1[26] = decrypt ? K[51] : K[31];
assign K1[27] = decrypt ? K[14] : K[21];
assign K1[28] = decrypt ? K[1]  : K[8];
assign K1[29] = decrypt ? K[16] : K[23];
assign K1[30] = decrypt ? K[45] : K[52];
assign K1[31] = decrypt ? K[7]  : K[14];
assign K1[32] = decrypt ? K[22] : K[29];
assign K1[33] = decrypt ? K[44] : K[51];
assign K1[34] = decrypt ? K[2]  : K[9];
assign K1[35] = decrypt ? K[28] : K[35];
assign K1[36] = decrypt ? K[23] : K[30];
assign K1[37] = decrypt ? K[50] : K[2];
assign K1[38] = decrypt ? K[30] : K[37];
assign K1[39] = decrypt ? K[15] : K[22];
assign K1[40] = decrypt ? K[52] : K[0];
assign K1[41] = decrypt ? K[35] : K[42];
assign K1[42] = decrypt ? K[31] : K[38];
assign K1[43] = decrypt ? K[9]  : K[16];
assign K1[44] = decrypt ? K[36] : K[43];
assign K1[45] = decrypt ? K[37] : K[44];
assign K1[46] = decrypt ? K[49] : K[1];
assign K1[47] = decrypt ? K[0]  : K[7];
assign K1[48] = decrypt ? K[21] : K[28];

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  DES                                                        ////
////  DES Top Level module                                       ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module des_area(desOut, desIn, key, decrypt, roundSel, clk);
output	[63:0]	desOut;
input	[63:0]	desIn;
input	[55:0]	key;
input		decrypt;
input	[3:0]	roundSel;
input		clk;

wire	[48:1]	K_sub;
wire	[64:1]	IP, FP;
reg	[32:1]	L, R;
wire	[32:1]	Xin;
wire	[32:1]	Lout, Rout;
wire	[32:1]	out;

assign Lout = (roundSel == 0) ? IP[64:33] : R;
assign Xin  = (roundSel == 0) ? IP[32:1] : L;
assign Rout = Xin ^ out;
assign FP = { Rout, Lout};

crp u0( .P(out), .R(Lout), .K_sub(K_sub) );

always @(posedge clk)
        L <= Lout;

always @(posedge clk)
        R <= Rout;
        
// Select a subkey from key.
key_sel u1(
	.K_sub(		K_sub		),
	.K(		key		),
	.roundSel(	roundSel	),
	.decrypt(	decrypt		)
	);

// Perform initial permutation
assign IP[64:1] = {	desIn[06], desIn[14], desIn[22], desIn[30], desIn[38], desIn[46],
			desIn[54], desIn[62], desIn[04], desIn[12], desIn[20], desIn[28],
			desIn[36], desIn[44], desIn[52], desIn[60], desIn[02], desIn[10], 
			desIn[18], desIn[26], desIn[34], desIn[42], desIn[50], desIn[58], 
			desIn[0], desIn[08], desIn[16], desIn[24], desIn[32], desIn[40], 
			desIn[48], desIn[56], desIn[07], desIn[15], desIn[23], desIn[31], 
			desIn[39], desIn[47], desIn[55], desIn[63], desIn[05], desIn[13],
			desIn[21], desIn[29], desIn[37], desIn[45], desIn[53], desIn[61],
			desIn[03], desIn[11], desIn[19], desIn[27], desIn[35], desIn[43],
			desIn[51], desIn[59], desIn[01], desIn[09], desIn[17], desIn[25],
			desIn[33], desIn[41], desIn[49], desIn[57] };

// Perform final permutation
assign desOut = {	FP[40], FP[08], FP[48], FP[16], FP[56], FP[24], FP[64], FP[32], 
			FP[39], FP[07], FP[47], FP[15], FP[55], FP[23], FP[63], FP[31], 
			FP[38], FP[06], FP[46], FP[14], FP[54], FP[22], FP[62], FP[30], 
			FP[37], FP[05], FP[45], FP[13], FP[53], FP[21], FP[61], FP[29], 
			FP[36], FP[04], FP[44], FP[12], FP[52], FP[20], FP[60], FP[28], 
			FP[35], FP[03], FP[43], FP[11], FP[51], FP[19], FP[59], FP[27],
			FP[34], FP[02], FP[42], FP[10], FP[50], FP[18], FP[58], FP[26], 
			FP[33], FP[01], FP[41], FP[09], FP[49], FP[17], FP[57], FP[25] };


endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  CRP                                                        ////
////  DES Crypt Module                                           ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  crp(P, R, K_sub);
output	[32:1]	P;
input	[32:1]	R;
input	[48:1]	K_sub;

wire	[48:1] E;
wire	[48:1] X;
wire	[32:1] S;

assign E[48:1] = {	R[32], R[1], R[2], R[3], R[4], R[5], R[4], R[5],
			R[6], R[7], R[8], R[9], R[8], R[9], R[10], R[11],
			R[12], R[13], R[12], R[13], R[14], R[15], R[16],
			R[17], R[16], R[17], R[18], R[19], R[20], R[21],
			R[20], R[21], R[22], R[23], R[24], R[25], R[24],
			R[25], R[26], R[27], R[28], R[29], R[28], R[29],
			R[30], R[31], R[32], R[1]};

assign X = E ^ K_sub;

sbox1 u0( .addr(X[06:01]), .dout(S[04:01]) );
sbox2 u1( .addr(X[12:07]), .dout(S[08:05]) );
sbox3 u2( .addr(X[18:13]), .dout(S[12:09]) );
sbox4 u3( .addr(X[24:19]), .dout(S[16:13]) );
sbox5 u4( .addr(X[30:25]), .dout(S[20:17]) );
sbox6 u5( .addr(X[36:31]), .dout(S[24:21]) );
sbox7 u6( .addr(X[42:37]), .dout(S[28:25]) );
sbox8 u7( .addr(X[48:43]), .dout(S[32:29]) );

assign P[32:1] = {	S[16], S[7], S[20], S[21], S[29], S[12], S[28],
			S[17], S[1], S[15], S[23], S[26], S[5], S[18],
			S[31], S[10], S[2], S[8], S[24], S[14], S[32],
			S[27], S[3], S[9], S[19], S[13], S[30], S[6],
			S[22], S[11], S[4], S[25]};

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox1(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout =  14;
         1:  dout =   4;
         2:  dout =  13;
         3:  dout =   1;
         4:  dout =   2;
         5:  dout =  15;
         6:  dout =  11;
         7:  dout =   8;
         8:  dout =   3;
         9:  dout =  10;
        10:  dout =   6;
        11:  dout =  12;
        12:  dout =   5;
        13:  dout =   9;
        14:  dout =   0;
        15:  dout =   7;

        16:  dout =   0;
        17:  dout =  15;
        18:  dout =   7;
        19:  dout =   4;
        20:  dout =  14;
        21:  dout =   2;
        22:  dout =  13;
        23:  dout =   1;
        24:  dout =  10;
        25:  dout =   6;
        26:  dout =  12;
        27:  dout =  11;
        28:  dout =   9;
        29:  dout =   5;
        30:  dout =   3;
        31:  dout =   8;

        32:  dout =   4;
        33:  dout =   1;
        34:  dout =  14;
        35:  dout =   8;
        36:  dout =  13;
        37:  dout =   6;
        38:  dout =   2;
        39:  dout =  11;
        40:  dout =  15;
        41:  dout =  12;
        42:  dout =   9;
        43:  dout =   7;
        44:  dout =   3;
        45:  dout =  10;
        46:  dout =   5;
        47:  dout =   0;

        48:  dout =  15;
        49:  dout =  12;
        50:  dout =   8;
        51:  dout =   2;
        52:  dout =   4;
        53:  dout =   9;
        54:  dout =   1;
        55:  dout =   7;
        56:  dout =   5;
        57:  dout =  11;
        58:  dout =   3;
        59:  dout =  14;
        60:  dout =  10;
        61:  dout =   0;
        62:  dout =   6;
        63:  dout =  13;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox2(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout = 15;
         1:  dout =  1;
         2:  dout =  8;
         3:  dout = 14;
         4:  dout =  6;
         5:  dout = 11;
         6:  dout =  3;
         7:  dout =  4;
         8:  dout =  9;
         9:  dout =  7;
        10:  dout =  2;
        11:  dout = 13;
        12:  dout = 12;
        13:  dout =  0;
        14:  dout =  5;
        15:  dout = 10;

        16:  dout =  3;
        17:  dout = 13;
        18:  dout =  4;
        19:  dout =  7;
        20:  dout = 15;
        21:  dout =  2;
        22:  dout =  8;
        23:  dout = 14;
        24:  dout = 12;
        25:  dout =  0;
        26:  dout =  1;
        27:  dout = 10;
        28:  dout =  6;
        29:  dout =  9;
        30:  dout = 11;
        31:  dout =  5;

        32:  dout =  0;
        33:  dout = 14;
        34:  dout =  7;
        35:  dout = 11;
        36:  dout = 10;
        37:  dout =  4;
        38:  dout = 13;
        39:  dout =  1;
        40:  dout =  5;
        41:  dout =  8;
        42:  dout = 12;
        43:  dout =  6;
        44:  dout =  9;
        45:  dout =  3;
        46:  dout =  2;
        47:  dout = 15;

        48:  dout = 13;
        49:  dout =  8;
        50:  dout = 10;
        51:  dout =  1;
        52:  dout =  3;
        53:  dout = 15;
        54:  dout =  4;
        55:  dout =  2;
        56:  dout = 11;
        57:  dout =  6;
        58:  dout =  7;
        59:  dout = 12;
        60:  dout =  0;
        61:  dout =  5;
        62:  dout = 14;
        63:  dout =  9;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox3(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout = 10;
         1:  dout =  0;
         2:  dout =  9;
         3:  dout = 14;
         4:  dout =  6;
         5:  dout =  3;
         6:  dout = 15;
         7:  dout =  5;
         8:  dout =  1;
         9:  dout = 13;
        10:  dout = 12;
        11:  dout =  7;
        12:  dout = 11;
        13:  dout =  4;
        14:  dout =  2;
        15:  dout =  8;

        16:  dout = 13;
        17:  dout =  7;
        18:  dout =  0;
        19:  dout =  9;
        20:  dout =  3;
        21:  dout =  4;
        22:  dout =  6;
        23:  dout = 10;
        24:  dout =  2;
        25:  dout =  8;
        26:  dout =  5;
        27:  dout = 14;
        28:  dout = 12;
        29:  dout = 11;
        30:  dout = 15;
        31:  dout =  1;

        32:  dout = 13;
        33:  dout =  6;
        34:  dout =  4;
        35:  dout =  9;
        36:  dout =  8;
        37:  dout = 15;
        38:  dout =  3;
        39:  dout =  0;
        40:  dout = 11;
        41:  dout =  1;
        42:  dout =  2;
        43:  dout = 12;
        44:  dout =  5;
        45:  dout = 10;
        46:  dout = 14;
        47:  dout =  7;

        48:  dout =  1;
        49:  dout = 10;
        50:  dout = 13;
        51:  dout =  0;
        52:  dout =  6;
        53:  dout =  9;
        54:  dout =  8;
        55:  dout =  7;
        56:  dout =  4;
        57:  dout = 15;
        58:  dout = 14;
        59:  dout =  3;
        60:  dout = 11;
        61:  dout =  5;
        62:  dout =  2;
        63:  dout = 12;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox4(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout =  7;
         1:  dout = 13;
         2:  dout = 14;
         3:  dout =  3;
         4:  dout =  0;
         5:  dout =  6;
         6:  dout =  9;
         7:  dout = 10;
         8:  dout =  1;
         9:  dout =  2;
        10:  dout =  8;
        11:  dout =  5;
        12:  dout = 11;
        13:  dout = 12;
        14:  dout =  4;
        15:  dout = 15;

        16:  dout = 13;
        17:  dout =  8;
        18:  dout = 11;
        19:  dout =  5;
        20:  dout =  6;
        21:  dout = 15;
        22:  dout =  0;
        23:  dout =  3;
        24:  dout =  4;
        25:  dout =  7;
        26:  dout =  2;
        27:  dout = 12;
        28:  dout =  1;
        29:  dout = 10;
        30:  dout = 14;
        31:  dout =  9;

        32:  dout = 10;
        33:  dout =  6;
        34:  dout =  9;
        35:  dout =  0;
        36:  dout = 12;
        37:  dout = 11;
        38:  dout =  7;
        39:  dout = 13;
        40:  dout = 15;
        41:  dout =  1;
        42:  dout =  3;
        43:  dout = 14;
        44:  dout =  5;
        45:  dout =  2;
        46:  dout =  8;
        47:  dout =  4;

        48:  dout =  3;
        49:  dout = 15;
        50:  dout =  0;
        51:  dout =  6;
        52:  dout = 10;
        53:  dout =  1;
        54:  dout = 13;
        55:  dout =  8;
        56:  dout =  9;
        57:  dout =  4;
        58:  dout =  5;
        59:  dout = 11;
        60:  dout = 12;
        61:  dout =  7;
        62:  dout =  2;
        63:  dout = 14;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox5(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout =  2;
         1:  dout = 12;
         2:  dout =  4;
         3:  dout =  1;
         4:  dout =  7;
         5:  dout = 10;
         6:  dout = 11;
         7:  dout =  6;
         8:  dout =  8;
         9:  dout =  5;
        10:  dout =  3;
        11:  dout = 15;
        12:  dout = 13;
        13:  dout =  0;
        14:  dout = 14;
        15:  dout =  9;

        16:  dout = 14;
        17:  dout = 11;
        18:  dout =  2;
        19:  dout = 12;
        20:  dout =  4;
        21:  dout =  7;
        22:  dout = 13;
        23:  dout =  1;
        24:  dout =  5;
        25:  dout =  0;
        26:  dout = 15;
        27:  dout = 10;
        28:  dout =  3;
        29:  dout =  9;
        30:  dout =  8;
        31:  dout =  6;

        32:  dout =  4;
        33:  dout =  2;
        34:  dout =  1;
        35:  dout = 11;
        36:  dout = 10;
        37:  dout = 13;
        38:  dout =  7;
        39:  dout =  8;
        40:  dout = 15;
        41:  dout =  9;
        42:  dout = 12;
        43:  dout =  5;
        44:  dout =  6;
        45:  dout =  3;
        46:  dout =  0;
        47:  dout = 14;

        48:  dout = 11;
        49:  dout =  8;
        50:  dout = 12;
        51:  dout =  7;
        52:  dout =  1;
        53:  dout = 14;
        54:  dout =  2;
        55:  dout = 13;
        56:  dout =  6;
        57:  dout = 15;
        58:  dout =  0;
        59:  dout =  9;
        60:  dout = 10;
        61:  dout =  4;
        62:  dout =  5;
        63:  dout =  3;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox6(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout = 12;
         1:  dout =  1;
         2:  dout = 10;
         3:  dout = 15;
         4:  dout =  9;
         5:  dout =  2;
         6:  dout =  6;
         7:  dout =  8;
         8:  dout =  0;
         9:  dout = 13;
        10:  dout =  3;
        11:  dout =  4;
        12:  dout = 14;
        13:  dout =  7;
        14:  dout =  5;
        15:  dout = 11;

        16:  dout = 10;
        17:  dout = 15;
        18:  dout =  4;
        19:  dout =  2;
        20:  dout =  7;
        21:  dout = 12;
        22:  dout =  9;
        23:  dout =  5;
        24:  dout =  6;
        25:  dout =  1;
        26:  dout = 13;
        27:  dout = 14;
        28:  dout =  0;
        29:  dout = 11;
        30:  dout =  3;
        31:  dout =  8;

        32:  dout =  9;
        33:  dout = 14;
        34:  dout = 15;
        35:  dout =  5;
        36:  dout =  2;
        37:  dout =  8;
        38:  dout = 12;
        39:  dout =  3;
        40:  dout =  7;
        41:  dout =  0;
        42:  dout =  4;
        43:  dout = 10;
        44:  dout =  1;
        45:  dout = 13;
        46:  dout = 11;
        47:  dout =  6;

        48:  dout =  4;
        49:  dout =  3;
        50:  dout =  2;
        51:  dout = 12;
        52:  dout =  9;
        53:  dout =  5;
        54:  dout = 15;
        55:  dout = 10;
        56:  dout = 11;
        57:  dout = 14;
        58:  dout =  1;
        59:  dout =  7;
        60:  dout =  6;
        61:  dout =  0;
        62:  dout =  8;
        63:  dout = 13;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox7(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout =  4;
         1:  dout = 11;
         2:  dout =  2;
         3:  dout = 14;
         4:  dout = 15;
         5:  dout =  0;
         6:  dout =  8;
         7:  dout = 13;
         8:  dout =  3;
         9:  dout = 12;
        10:  dout =  9;
        11:  dout =  7;
        12:  dout =  5;
        13:  dout = 10;
        14:  dout =  6;
        15:  dout =  1;

        16:  dout = 13;
        17:  dout =  0;
        18:  dout = 11;
        19:  dout =  7;
        20:  dout =  4;
        21:  dout =  9;
        22:  dout =  1;
        23:  dout = 10;
        24:  dout = 14;
        25:  dout =  3;
        26:  dout =  5;
        27:  dout = 12;
        28:  dout =  2;
        29:  dout = 15;
        30:  dout =  8;
        31:  dout =  6;

        32:  dout =  1;
        33:  dout =  4;
        34:  dout = 11;
        35:  dout = 13;
        36:  dout = 12;
        37:  dout =  3;
        38:  dout =  7;
        39:  dout = 14;
        40:  dout = 10;
        41:  dout = 15;
        42:  dout =  6;
        43:  dout =  8;
        44:  dout =  0;
        45:  dout =  5;
        46:  dout =  9;
        47:  dout =  2;

        48:  dout =  6;
        49:  dout = 11;
        50:  dout = 13;
        51:  dout =  8;
        52:  dout =  1;
        53:  dout =  4;
        54:  dout = 10;
        55:  dout =  7;
        56:  dout =  9;
        57:  dout =  5;
        58:  dout =  0;
        59:  dout = 15;
        60:  dout = 14;
        61:  dout =  2;
        62:  dout =  3;
        63:  dout = 12;

    endcase
    end

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  SBOX                                                       ////
////  The SBOX is essentially a 64x4 ROM                         ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Rudolf Usselmann                         ////
////                    rudi@asics.ws                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

module  sbox8(addr, dout);
input	[6:1] addr;
output	[4:1] dout;
reg	[4:1] dout;

always @(addr) begin
    case ({addr[1], addr[6], addr[5:2]})	//synopsys full_case parallel_case
         0:  dout = 13;
         1:  dout =  2;
         2:  dout =  8;
         3:  dout =  4;
         4:  dout =  6;
         5:  dout = 15;
         6:  dout = 11;
         7:  dout =  1;
         8:  dout = 10;
         9:  dout =  9;
        10:  dout =  3;
        11:  dout = 14;
        12:  dout =  5;
        13:  dout =  0;
        14:  dout = 12;
        15:  dout =  7;

        16:  dout =  1;
        17:  dout = 15;
        18:  dout = 13;
        19:  dout =  8;
        20:  dout = 10;
        21:  dout =  3;
        22:  dout =  7;
        23:  dout =  4;
        24:  dout = 12;
        25:  dout =  5;
        26:  dout =  6;
        27:  dout = 11;
        28:  dout =  0;
        29:  dout = 14;
        30:  dout =  9;
        31:  dout =  2;

        32:  dout =  7;
        33:  dout = 11;
        34:  dout =  4;
        35:  dout =  1;
        36:  dout =  9;
        37:  dout = 12;
        38:  dout = 14;
        39:  dout =  2;
        40:  dout =  0;
        41:  dout =  6;
        42:  dout = 10;
        43:  dout = 13;
        44:  dout = 15;
        45:  dout =  3;
        46:  dout =  5;
        47:  dout =  8;

        48:  dout =  2;
        49:  dout =  1;
        50:  dout = 14;
        51:  dout =  7;
        52:  dout =  4;
        53:  dout = 10;
        54:  dout =  8;
        55:  dout = 13;
        56:  dout = 15;
        57:  dout = 12;
        58:  dout =  9;
        59:  dout =  0;
        60:  dout =  3;
        61:  dout =  5;
        62:  dout =  6;
        63:  dout = 11;

    endcase
    end

endmodule
