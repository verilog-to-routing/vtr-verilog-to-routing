
// Copyright (C) 2014 John Leitch (johnleitch@outlook.com)

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


`define CopyChunkWords (__lhs, __rhs) \
  __lhs[0] <= __rhs[0];         \
  __lhs[1] <= __rhs[1];         \
  __lhs[2] <= __rhs[2];         \
  __lhs[3] <= __rhs[3];         \
  __lhs[4] <= __rhs[4];         \
  __lhs[5] <= __rhs[5];         \
  __lhs[6] <= __rhs[6];         \
  __lhs[7] <= __rhs[7];         \
  __lhs[8] <= __rhs[8];         \
  __lhs[9] <= __rhs[9];         \
  __lhs[10] <= __rhs[10];       \
  __lhs[11] <= __rhs[11];       \
  __lhs[12] <= __rhs[12];       \
  __lhs[13] <= __rhs[13];       \
  __lhs[14] <= __rhs[14];       \
  __lhs[15] <= __rhs[15];

`define CopyDigestWords(__lhs1, __rhs1, __lhs2, __rhs2, __lhs3, __rhs3) \
  __lhs1 <= __rhs1;                                                     \
  __lhs2 <= __rhs2;                                                     \
  __lhs3 <= __rhs3;

module Md5Core (
  input wire clk,
  input wire [511:0] wb,
  input wire [31:0] a0,
  input wire [31:0] b0,
  input wire [31:0] c0,
  input wire [31:0] d0,
  output reg [31:0] a64,
  output reg [31:0] b64,
  output reg [31:0] c64,
  output reg [31:0] d64);


  wire [31:0] w0 [0:15];
  assign w0[0] = wb[31:0];
  assign w0[1] = wb[63:32];
  assign w0[2] = wb[95:64];
  assign w0[3] = wb[127:96];
  assign w0[4] = wb[159:128];
  assign w0[5] = wb[191:160];
  assign w0[6] = wb[223:192];
  assign w0[7] = wb[255:224];
  assign w0[8] = wb[287:256];
  assign w0[9] = wb[319:288];
  assign w0[10] = wb[351:320];
  assign w0[11] = wb[383:352];
  assign w0[12] = wb[415:384];
  assign w0[13] = wb[447:416];
  assign w0[14] = wb[479:448];
  assign w0[15] = wb[511:480];

  reg [31:0]
  a1, b1, c1, d1,
  a2, b2, c2, d2,
  a3, b3, c3, d3,
  a4, b4, c4, d4,
  a5, b5, c5, d5,
  a6, b6, c6, d6,
  a7, b7, c7, d7,
  a8, b8, c8, d8,
  a9, b9, c9, d9,
  a10, b10, c10, d10,
  a11, b11, c11, d11,
  a12, b12, c12, d12,
  a13, b13, c13, d13,
  a14, b14, c14, d14,
  a15, b15, c15, d15,
  a16, b16, c16, d16,
  a17, b17, c17, d17,
  a18, b18, c18, d18,
  a19, b19, c19, d19,
  a20, b20, c20, d20,
  a21, b21, c21, d21,
  a22, b22, c22, d22,
  a23, b23, c23, d23,
  a24, b24, c24, d24,
  a25, b25, c25, d25,
  a26, b26, c26, d26,
  a27, b27, c27, d27,
  a28, b28, c28, d28,
  a29, b29, c29, d29,
  a30, b30, c30, d30,
  a31, b31, c31, d31,
  a32, b32, c32, d32,
  a33, b33, c33, d33,
  a34, b34, c34, d34,
  a35, b35, c35, d35,
  a36, b36, c36, d36,
  a37, b37, c37, d37,
  a38, b38, c38, d38,
  a39, b39, c39, d39,
  a40, b40, c40, d40,
  a41, b41, c41, d41,
  a42, b42, c42, d42,
  a43, b43, c43, d43,
  a44, b44, c44, d44,
  a45, b45, c45, d45,
  a46, b46, c46, d46,
  a47, b47, c47, d47,
  a48, b48, c48, d48,
  a49, b49, c49, d49,
  a50, b50, c50, d50,
  a51, b51, c51, d51,
  a52, b52, c52, d52,
  a53, b53, c53, d53,
  a54, b54, c54, d54,
  a55, b55, c55, d55,
  a56, b56, c56, d56,
  a57, b57, c57, d57,
  a58, b58, c58, d58,
  a59, b59, c59, d59,
  a60, b60, c60, d60,
  a61, b61, c61, d61,
  a62, b62, c62, d62,
  a63, b63, c63, d63
  ;

  reg [31:0] w1 [0:15];
  reg [31:0] w2 [0:15];
  reg [31:0] w3 [0:15];
  reg [31:0] w4 [0:15];
  reg [31:0] w5 [0:15];
  reg [31:0] w6 [0:15];
  reg [31:0] w7 [0:15];
  reg [31:0] w8 [0:15];
  reg [31:0] w9 [0:15];
  reg [31:0] w10 [0:15];
  reg [31:0] w11 [0:15];
  reg [31:0] w12 [0:15];
  reg [31:0] w13 [0:15];
  reg [31:0] w14 [0:15];
  reg [31:0] w15 [0:15];
  reg [31:0] w16 [0:15];
  reg [31:0] w17 [0:15];
  reg [31:0] w18 [0:15];
  reg [31:0] w19 [0:15];
  reg [31:0] w20 [0:15];
  reg [31:0] w21 [0:15];
  reg [31:0] w22 [0:15];
  reg [31:0] w23 [0:15];
  reg [31:0] w24 [0:15];
  reg [31:0] w25 [0:15];
  reg [31:0] w26 [0:15];
  reg [31:0] w27 [0:15];
  reg [31:0] w28 [0:15];
  reg [31:0] w29 [0:15];
  reg [31:0] w30 [0:15];
  reg [31:0] w31 [0:15];
  reg [31:0] w32 [0:15];
  reg [31:0] w33 [0:15];
  reg [31:0] w34 [0:15];
  reg [31:0] w35 [0:15];
  reg [31:0] w36 [0:15];
  reg [31:0] w37 [0:15];
  reg [31:0] w38 [0:15];
  reg [31:0] w39 [0:15];
  reg [31:0] w40 [0:15];
  reg [31:0] w41 [0:15];
  reg [31:0] w42 [0:15];
  reg [31:0] w43 [0:15];
  reg [31:0] w44 [0:15];
  reg [31:0] w45 [0:15];
  reg [31:0] w46 [0:15];
  reg [31:0] w47 [0:15];
  reg [31:0] w48 [0:15];
  reg [31:0] w49 [0:15];
  reg [31:0] w50 [0:15];
  reg [31:0] w51 [0:15];
  reg [31:0] w52 [0:15];
  reg [31:0] w53 [0:15];
  reg [31:0] w54 [0:15];
  reg [31:0] w55 [0:15];
  reg [31:0] w56 [0:15];
  reg [31:0] w57 [0:15];
  reg [31:0] w58 [0:15];
  reg [31:0] w59 [0:15];
  reg [31:0] w60 [0:15];
  reg [31:0] w61 [0:15];
  reg [31:0] w62 [0:15];
  reg [31:0] w63 [0:15];

  always @(posedge clk)
    begin
      `CopyDigestWords(a1, d0, d1, c0, c1, b0)
      b1 <= b0 + ((((a0 + ((b0 & c0) | ((~b0) & d0)) + 'hd76aa478 + w0[0]) << 7) | ((a0 + ((b0 & c0) | ((~b0) & d0)) + 'hd76aa478 + w0[0]) >> (32 - 7))));
      `CopyChunkWords(w1, w0)

      `CopyDigestWords(a2, d1, d2, c1, c2, b1)
      b2 <= b1 + (((a1 + ((b1 & c1) | ((~b1) & d1)) + 'he8c7b756 + w1[1]) << 12) | ((a1 + ((b1 & c1) | ((~b1) & d1)) + 'he8c7b756 + w1[1]) >> (32 - 12)));
      `CopyChunkWords(w2, w1)

      `CopyDigestWords(a3, d2, d3, c2, c3, b2)
      b3 <= b2 + (((a2 + ((b2 & c2) | ((~b2) & d2)) + 'h242070db + w2[2]) << 17) | ((a2 + ((b2 & c2) | ((~b2) & d2)) + 'h242070db + w2[2]) >> (32 - 17)));
      `CopyChunkWords(w3, w2)

      `CopyDigestWords(a4, d3, d4, c3, c4, b3)
      b4 <= b3 + (((a3 + ((b3 & c3) | ((~b3) & d3)) + 'hc1bdceee + w3[3]) << 22) | ((a3 + ((b3 & c3) | ((~b3) & d3)) + 'hc1bdceee + w3[3]) >> (32 - 22)));
      `CopyChunkWords(w4, w3)

      `CopyDigestWords(a5, d4, d5, c4, c5, b4)
      b5 <= b4 + (((a4 + ((b4 & c4) | ((~b4) & d4)) + 'hf57c0faf + w4[4]) << 7) | ((a4 + ((b4 & c4) | ((~b4) & d4)) + 'hf57c0faf + w4[4]) >> (32 - 7)));
      `CopyChunkWords(w5, w4)

      `CopyDigestWords(a6, d5, d6, c5, c6, b5)
      b6 <= b5 + (((a5 + ((b5 & c5) | ((~b5) & d5)) + 'h4787c62a + w5[5]) << 12) | ((a5 + ((b5 & c5) | ((~b5) & d5)) + 'h4787c62a + w5[5]) >> (32 - 12)));
      `CopyChunkWords(w6, w5)

      `CopyDigestWords(a7, d6, d7, c6, c7, b6)
      b7 <= b6 + (((a6 + ((b6 & c6) | ((~b6) & d6)) + 'ha8304613 + w6[6]) << 17) | ((a6 + ((b6 & c6) | ((~b6) & d6)) + 'ha8304613 + w6[6]) >> (32 - 17)));
      `CopyChunkWords(w7, w6)

      `CopyDigestWords(a8, d7, d8, c7, c8, b7)
      b8 <= b7 + (((a7 + ((b7 & c7) | ((~b7) & d7)) + 'hfd469501 + w7[7]) << 22) | ((a7 + ((b7 & c7) | ((~b7) & d7)) + 'hfd469501 + w7[7]) >> (32 - 22)));
      `CopyChunkWords(w8, w7)

      `CopyDigestWords(a9, d8, d9, c8, c9, b8)
      b9 <= b8 + (((a8 + ((b8 & c8) | ((~b8) & d8)) + 'h698098d8 + w8[8]) << 7) | ((a8 + ((b8 & c8) | ((~b8) & d8)) + 'h698098d8 + w8[8]) >> (32 - 7)));
      `CopyChunkWords(w9, w8)

      `CopyDigestWords(a10, d9, d10, c9, c10, b9)
      b10 <= b9 + (((a9 + ((b9 & c9) | ((~b9) & d9)) + 'h8b44f7af + w9[9]) << 12) | ((a9 + ((b9 & c9) | ((~b9) & d9)) + 'h8b44f7af + w9[9]) >> (32 - 12)));
      `CopyChunkWords(w10, w9)

      `CopyDigestWords(a11, d10, d11, c10, c11, b10)
      b11 <= b10 + (((a10 + ((b10 & c10) | ((~b10) & d10)) + 'hffff5bb1 + w10[10]) << 17) | ((a10 + ((b10 & c10) | ((~b10) & d10)) + 'hffff5bb1 + w10[10]) >> (32 - 17)));
      `CopyChunkWords(w11, w10)

      `CopyDigestWords(a12, d11, d12, c11, c12, b11)
      b12 <= b11 + (((a11 + ((b11 & c11) | ((~b11) & d11)) + 'h895cd7be + w11[11]) << 22) | ((a11 + ((b11 & c11) | ((~b11) & d11)) + 'h895cd7be + w11[11]) >> (32 - 22)));
      `CopyChunkWords(w12, w11)

      `CopyDigestWords(a13, d12, d13, c12, c13, b12)
      b13 <= b12 + (((a12 + ((b12 & c12) | ((~b12) & d12)) + 'h6b901122 + w12[12]) << 7) | ((a12 + ((b12 & c12) | ((~b12) & d12)) + 'h6b901122 + w12[12]) >> (32 - 7)));
      `CopyChunkWords(w13, w12)

      `CopyDigestWords(a14, d13, d14, c13, c14, b13)
      b14 <= b13 + (((a13 + ((b13 & c13) | ((~b13) & d13)) + 'hfd987193 + w13[13]) << 12) | ((a13 + ((b13 & c13) | ((~b13) & d13)) + 'hfd987193 + w13[13]) >> (32 - 12)));
      `CopyChunkWords(w14, w13)

      `CopyDigestWords(a15, d14, d15, c14, c15, b14)
      b15 <= b14 + (((a14 + ((b14 & c14) | ((~b14) & d14)) + 'ha679438e + w14[14]) << 17) | ((a14 + ((b14 & c14) | ((~b14) & d14)) + 'ha679438e + w14[14]) >> (32 - 17)));
      `CopyChunkWords(w15, w14)

      `CopyDigestWords(a16, d15, d16, c15, c16, b15)
      b16 <= b15 + (((a15 + ((b15 & c15) | ((~b15) & d15)) + 'h49b40821 + w15[15]) << 22) | ((a15 + ((b15 & c15) | ((~b15) & d15)) + 'h49b40821 + w15[15]) >> (32 - 22)));
      `CopyChunkWords(w16, w15)

      `CopyDigestWords(a17, d16, d17, c16, c17, b16)
      b17 <= b16 + (((a16 + ((d16 & b16) | ((~d16) & c16)) + 'hf61e2562 + w16[(5 * 16 + 1) % 16]) << 5) | ((a16 + ((d16 & b16) | ((~d16) & c16)) + 'hf61e2562 + w16[(5 * 16 + 1) % 16]) >> (32 - 5)));
      `CopyChunkWords(w17, w16)

      `CopyDigestWords(a18, d17, d18, c17, c18, b17)
      b18 <= b17 + (((a17 + ((d17 & b17) | ((~d17) & c17)) + 'hc040b340 + w17[(5 * 17 + 1) % 16]) << 9) | ((a17 + ((d17 & b17) | ((~d17) & c17)) + 'hc040b340 + w17[(5 * 17 + 1) % 16]) >> (32 - 9)));
      `CopyChunkWords(w18, w17)

      `CopyDigestWords(a19, d18, d19, c18, c19, b18)
      b19 <= b18 + (((a18 + ((d18 & b18) | ((~d18) & c18)) + 'h265e5a51 + w18[(5 * 18 + 1) % 16]) << 14) | ((a18 + ((d18 & b18) | ((~d18) & c18)) + 'h265e5a51 + w18[(5 * 18 + 1) % 16]) >> (32 - 14)));
      `CopyChunkWords(w19, w18)

      `CopyDigestWords(a20, d19, d20, c19, c20, b19)
      b20 <= b19 + (((a19 + ((d19 & b19) | ((~d19) & c19)) + 'he9b6c7aa + w19[(5 * 19 + 1) % 16]) << 20) | ((a19 + ((d19 & b19) | ((~d19) & c19)) + 'he9b6c7aa + w19[(5 * 19 + 1) % 16]) >> (32 - 20)));
      `CopyChunkWords(w20, w19)

      `CopyDigestWords(a21, d20, d21, c20, c21, b20)
      b21 <= b20 + (((a20 + ((d20 & b20) | ((~d20) & c20)) + 'hd62f105d + w20[(5 * 20 + 1) % 16]) << 5) | ((a20 + ((d20 & b20) | ((~d20) & c20)) + 'hd62f105d + w20[(5 * 20 + 1) % 16]) >> (32 - 5)));
      `CopyChunkWords(w21, w20)

      `CopyDigestWords(a22, d21, d22, c21, c22, b21)
      b22 <= b21 + (((a21 + ((d21 & b21) | ((~d21) & c21)) + 'h02441453 + w21[(5 * 21 + 1) % 16]) << 9) | ((a21 + ((d21 & b21) | ((~d21) & c21)) + 'h02441453 + w21[(5 * 21 + 1) % 16]) >> (32 - 9)));
      `CopyChunkWords(w22, w21)

      `CopyDigestWords(a23, d22, d23, c22, c23, b22)
      b23 <= b22 + (((a22 + ((d22 & b22) | ((~d22) & c22)) + 'hd8a1e681 + w22[(5 * 22 + 1) % 16]) << 14) | ((a22 + ((d22 & b22) | ((~d22) & c22)) + 'hd8a1e681 + w22[(5 * 22 + 1) % 16]) >> (32 - 14)));
      `CopyChunkWords(w23, w22)

      `CopyDigestWords(a24, d23, d24, c23, c24, b23)
      b24 <= b23 + (((a23 + ((d23 & b23) | ((~d23) & c23)) + 'he7d3fbc8 + w23[(5 * 23 + 1) % 16]) << 20) | ((a23 + ((d23 & b23) | ((~d23) & c23)) + 'he7d3fbc8 + w23[(5 * 23 + 1) % 16]) >> (32 - 20)));
      `CopyChunkWords(w24, w23)

      `CopyDigestWords(a25, d24, d25, c24, c25, b24)
      b25 <= b24 + (((a24 + ((d24 & b24) | ((~d24) & c24)) + 'h21e1cde6 + w24[(5 * 24 + 1) % 16]) << 5) | ((a24 + ((d24 & b24) | ((~d24) & c24)) + 'h21e1cde6 + w24[(5 * 24 + 1) % 16]) >> (32 - 5)));
      `CopyChunkWords(w25, w24)

      `CopyDigestWords(a26, d25, d26, c25, c26, b25)
      b26 <= b25 + (((a25 + ((d25 & b25) | ((~d25) & c25)) + 'hc33707d6 + w25[(5 * 25 + 1) % 16]) << 9) | ((a25 + ((d25 & b25) | ((~d25) & c25)) + 'hc33707d6 + w25[(5 * 25 + 1) % 16]) >> (32 - 9)));
      `CopyChunkWords(w26, w25)

      `CopyDigestWords(a27, d26, d27, c26, c27, b26)
      b27 <= b26 + (((a26 + ((d26 & b26) | ((~d26) & c26)) + 'hf4d50d87 + w26[(5 * 26 + 1) % 16]) << 14) | ((a26 + ((d26 & b26) | ((~d26) & c26)) + 'hf4d50d87 + w26[(5 * 26 + 1) % 16]) >> (32 - 14)));
      `CopyChunkWords(w27, w26)

      `CopyDigestWords(a28, d27, d28, c27, c28, b27)
      b28 <= b27 + (((a27 + ((d27 & b27) | ((~d27) & c27)) + 'h455a14ed + w27[(5 * 27 + 1) % 16]) << 20) | ((a27 + ((d27 & b27) | ((~d27) & c27)) + 'h455a14ed + w27[(5 * 27 + 1) % 16]) >> (32 - 20)));
      `CopyChunkWords(w28, w27)

      `CopyDigestWords(a29, d28, d29, c28, c29, b28)
      b29 <= b28 + (((a28 + ((d28 & b28) | ((~d28) & c28)) + 'ha9e3e905 + w28[(5 * 28 + 1) % 16]) << 5) | ((a28 + ((d28 & b28) | ((~d28) & c28)) + 'ha9e3e905 + w28[(5 * 28 + 1) % 16]) >> (32 - 5)));
      `CopyChunkWords(w29, w28)

      `CopyDigestWords(a30, d29, d30, c29, c30, b29)
      b30 <= b29 + (((a29 + ((d29 & b29) | ((~d29) & c29)) + 'hfcefa3f8 + w29[(5 * 29 + 1) % 16]) << 9) | ((a29 + ((d29 & b29) | ((~d29) & c29)) + 'hfcefa3f8 + w29[(5 * 29 + 1) % 16]) >> (32 - 9)));
      `CopyChunkWords(w30, w29)

      `CopyDigestWords(a31, d30, d31, c30, c31, b30)
      b31 <= b30 + (((a30 + ((d30 & b30) | ((~d30) & c30)) + 'h676f02d9 + w30[(5 * 30 + 1) % 16]) << 14) | ((a30 + ((d30 & b30) | ((~d30) & c30)) + 'h676f02d9 + w30[(5 * 30 + 1) % 16]) >> (32 - 14)));
      `CopyChunkWords(w31, w30)

      `CopyDigestWords(a32, d31, d32, c31, c32, b31)
      b32 <= b31 + (((a31 + ((d31 & b31) | ((~d31) & c31)) + 'h8d2a4c8a + w31[(5 * 31 + 1) % 16]) << 20) | ((a31 + ((d31 & b31) | ((~d31) & c31)) + 'h8d2a4c8a + w31[(5 * 31 + 1) % 16]) >> (32 - 20)));
      `CopyChunkWords(w32, w31)

      `CopyDigestWords(a33, d32, d33, c32, c33, b32)
      b33 <= b32 + (((a32 + (b32 ^ c32 ^ d32) + 'hfffa3942 + w32[(3 * 32 + 5) % 16]) << 4) | ((a32 + (b32 ^ c32 ^ d32) + 'hfffa3942 + w32[(3 * 32 + 5) % 16]) >> (32 - 4)));
      `CopyChunkWords(w33, w32)

      `CopyDigestWords(a34, d33, d34, c33, c34, b33)
      b34 <= b33 + (((a33 + (b33 ^ c33 ^ d33) + 'h8771f681 + w33[(3 * 33 + 5) % 16]) << 11) | ((a33 + (b33 ^ c33 ^ d33) + 'h8771f681 + w33[(3 * 33 + 5) % 16]) >> (32 - 11)));
      `CopyChunkWords(w34, w33)

      `CopyDigestWords(a35, d34, d35, c34, c35, b34)
      b35 <= b34 + (((a34 + (b34 ^ c34 ^ d34) + 'h6d9d6122 + w34[(3 * 34 + 5) % 16]) << 16) | ((a34 + (b34 ^ c34 ^ d34) + 'h6d9d6122 + w34[(3 * 34 + 5) % 16]) >> (32 - 16)));
      `CopyChunkWords(w35, w34)

      `CopyDigestWords(a36, d35, d36, c35, c36, b35)
      b36 <= b35 + (((a35 + (b35 ^ c35 ^ d35) + 'hfde5380c + w35[(3 * 35 + 5) % 16]) << 23) | ((a35 + (b35 ^ c35 ^ d35) + 'hfde5380c + w35[(3 * 35 + 5) % 16]) >> (32 - 23)));
      `CopyChunkWords(w36, w35)

      `CopyDigestWords(a37, d36, d37, c36, c37, b36)
      b37 <= b36 + (((a36 + (b36 ^ c36 ^ d36) + 'ha4beea44 + w36[(3 * 36 + 5) % 16]) << 4) | ((a36 + (b36 ^ c36 ^ d36) + 'ha4beea44 + w36[(3 * 36 + 5) % 16]) >> (32 - 4)));
      `CopyChunkWords(w37, w36)

      `CopyDigestWords(a38, d37, d38, c37, c38, b37)
      b38 <= b37 + (((a37 + (b37 ^ c37 ^ d37) + 'h4bdecfa9 + w37[(3 * 37 + 5) % 16]) << 11) | ((a37 + (b37 ^ c37 ^ d37) + 'h4bdecfa9 + w37[(3 * 37 + 5) % 16]) >> (32 - 11)));
      `CopyChunkWords(w38, w37)

      `CopyDigestWords(a39, d38, d39, c38, c39, b38)
      b39 <= b38 + (((a38 + (b38 ^ c38 ^ d38) + 'hf6bb4b60 + w38[(3 * 38 + 5) % 16]) << 16) | ((a38 + (b38 ^ c38 ^ d38) + 'hf6bb4b60 + w38[(3 * 38 + 5) % 16]) >> (32 - 16)));
      `CopyChunkWords(w39, w38)

      `CopyDigestWords(a40, d39, d40, c39, c40, b39)
      b40 <= b39 + (((a39 + (b39 ^ c39 ^ d39) + 'hbebfbc70 + w39[(3 * 39 + 5) % 16]) << 23) | ((a39 + (b39 ^ c39 ^ d39) + 'hbebfbc70 + w39[(3 * 39 + 5) % 16]) >> (32 - 23)));
      `CopyChunkWords(w40, w39)

      `CopyDigestWords(a41, d40, d41, c40, c41, b40)
      b41 <= b40 + (((a40 + (b40 ^ c40 ^ d40) + 'h289b7ec6 + w40[(3 * 40 + 5) % 16]) << 4) | ((a40 + (b40 ^ c40 ^ d40) + 'h289b7ec6 + w40[(3 * 40 + 5) % 16]) >> (32 - 4)));
      `CopyChunkWords(w41, w40)

      `CopyDigestWords(a42, d41, d42, c41, c42, b41)
      b42 <= b41 + (((a41 + (b41 ^ c41 ^ d41) + 'heaa127fa + w41[(3 * 41 + 5) % 16]) << 11) | ((a41 + (b41 ^ c41 ^ d41) + 'heaa127fa + w41[(3 * 41 + 5) % 16]) >> (32 - 11)));
      `CopyChunkWords(w42, w41)

      `CopyDigestWords(a43, d42, d43, c42, c43, b42)
      b43 <= b42 + (((a42 + (b42 ^ c42 ^ d42) + 'hd4ef3085 + w42[(3 * 42 + 5) % 16]) << 16) | ((a42 + (b42 ^ c42 ^ d42) + 'hd4ef3085 + w42[(3 * 42 + 5) % 16]) >> (32 - 16)));
      `CopyChunkWords(w43, w42)

      `CopyDigestWords(a44, d43, d44, c43, c44, b43)
      b44 <= b43 + (((a43 + (b43 ^ c43 ^ d43) + 'h04881d05 + w43[(3 * 43 + 5) % 16]) << 23) | ((a43 + (b43 ^ c43 ^ d43) + 'h04881d05 + w43[(3 * 43 + 5) % 16]) >> (32 - 23)));
      `CopyChunkWords(w44, w43)

      `CopyDigestWords(a45, d44, d45, c44, c45, b44)
      b45 <= b44 + (((a44 + (b44 ^ c44 ^ d44) + 'hd9d4d039 + w44[(3 * 44 + 5) % 16]) << 4) | ((a44 + (b44 ^ c44 ^ d44) + 'hd9d4d039 + w44[(3 * 44 + 5) % 16]) >> (32 - 4)));
      `CopyChunkWords(w45, w44)

      `CopyDigestWords(a46, d45, d46, c45, c46, b45)
      b46 <= b45 + (((a45 + (b45 ^ c45 ^ d45) + 'he6db99e5 + w45[(3 * 45 + 5) % 16]) << 11) | ((a45 + (b45 ^ c45 ^ d45) + 'he6db99e5 + w45[(3 * 45 + 5) % 16]) >> (32 - 11)));
      `CopyChunkWords(w46, w45)

      `CopyDigestWords(a47, d46, d47, c46, c47, b46)
      b47 <= b46 + (((a46 + (b46 ^ c46 ^ d46) + 'h1fa27cf8 + w46[(3 * 46 + 5) % 16]) << 16) | ((a46 + (b46 ^ c46 ^ d46) + 'h1fa27cf8 + w46[(3 * 46 + 5) % 16]) >> (32 - 16)));
      `CopyChunkWords(w47, w46)

      `CopyDigestWords(a48, d47, d48, c47, c48, b47)
      b48 <= b47 + (((a47 + (b47 ^ c47 ^ d47) + 'hc4ac5665 + w47[(3 * 47 + 5) % 16]) << 23) | ((a47 + (b47 ^ c47 ^ d47) + 'hc4ac5665 + w47[(3 * 47 + 5) % 16]) >> (32 - 23)));
      `CopyChunkWords(w48, w47)

      `CopyDigestWords(a49, d48, d49, c48, c49, b48)
      b49 <= b48 + (((a48 + (c48 ^ (b48 | (~d48))) + 'hf4292244 + w48[(7 * 48) % 16]) << 6) | ((a48 + (c48 ^ (b48 | (~d48))) + 'hf4292244 + w48[(7 * 48) % 16]) >> (32 - 6)));
      `CopyChunkWords(w49, w48)

      `CopyDigestWords(a50, d49, d50, c49, c50, b49)
      b50 <= b49 + (((a49 + (c49 ^ (b49 | (~d49))) + 'h432aff97 + w49[(7 * 49) % 16]) << 10) | ((a49 + (c49 ^ (b49 | (~d49))) + 'h432aff97 + w49[(7 * 49) % 16]) >> (32 - 10)));
      `CopyChunkWords(w50, w49)

      `CopyDigestWords(a51, d50, d51, c50, c51, b50)
      b51 <= b50 + (((a50 + (c50 ^ (b50 | (~d50))) + 'hab9423a7 + w50[(7 * 50) % 16]) << 15) | ((a50 + (c50 ^ (b50 | (~d50))) + 'hab9423a7 + w50[(7 * 50) % 16]) >> (32 - 15)));
      `CopyChunkWords(w51, w50)

      `CopyDigestWords(a52, d51, d52, c51, c52, b51)
      b52 <= b51 + (((a51 + (c51 ^ (b51 | (~d51))) + 'hfc93a039 + w51[(7 * 51) % 16]) << 21) | ((a51 + (c51 ^ (b51 | (~d51))) + 'hfc93a039 + w51[(7 * 51) % 16]) >> (32 - 21)));
      `CopyChunkWords(w52, w51)

      `CopyDigestWords(a53, d52, d53, c52, c53, b52)
      b53 <= b52 + (((a52 + (c52 ^ (b52 | (~d52))) + 'h655b59c3 + w52[(7 * 52) % 16]) << 6) | ((a52 + (c52 ^ (b52 | (~d52))) + 'h655b59c3 + w52[(7 * 52) % 16]) >> (32 - 6)));
      `CopyChunkWords(w53, w52)

      `CopyDigestWords(a54, d53, d54, c53, c54, b53)
      b54 <= b53 + (((a53 + (c53 ^ (b53 | (~d53))) + 'h8f0ccc92 + w53[(7 * 53) % 16]) << 10) | ((a53 + (c53 ^ (b53 | (~d53))) + 'h8f0ccc92 + w53[(7 * 53) % 16]) >> (32 - 10)));
      `CopyChunkWords(w54, w53)

      `CopyDigestWords(a55, d54, d55, c54, c55, b54)
      b55 <= b54 + (((a54 + (c54 ^ (b54 | (~d54))) + 'hffeff47d + w54[(7 * 54) % 16]) << 15) | ((a54 + (c54 ^ (b54 | (~d54))) + 'hffeff47d + w54[(7 * 54) % 16]) >> (32 - 15)));
      `CopyChunkWords(w55, w54)

      `CopyDigestWords(a56, d55, d56, c55, c56, b55)
      b56 <= b55 + (((a55 + (c55 ^ (b55 | (~d55))) + 'h85845dd1 + w55[(7 * 55) % 16]) << 21) | ((a55 + (c55 ^ (b55 | (~d55))) + 'h85845dd1 + w55[(7 * 55) % 16]) >> (32 - 21)));
      `CopyChunkWords(w56, w55)

      `CopyDigestWords(a57, d56, d57, c56, c57, b56)
      b57 <= b56 + (((a56 + (c56 ^ (b56 | (~d56))) + 'h6fa87e4f + w56[(7 * 56) % 16]) << 6) | ((a56 + (c56 ^ (b56 | (~d56))) + 'h6fa87e4f + w56[(7 * 56) % 16]) >> (32 - 6)));
      `CopyChunkWords(w57, w56)

      `CopyDigestWords(a58, d57, d58, c57, c58, b57)
      b58 <= b57 + (((a57 + (c57 ^ (b57 | (~d57))) + 'hfe2ce6e0 + w57[(7 * 57) % 16]) << 10) | ((a57 + (c57 ^ (b57 | (~d57))) + 'hfe2ce6e0 + w57[(7 * 57) % 16]) >> (32 - 10)));
      `CopyChunkWords(w58, w57)

      `CopyDigestWords(a59, d58, d59, c58, c59, b58)
      b59 <= b58 + (((a58 + (c58 ^ (b58 | (~d58))) + 'ha3014314 + w58[(7 * 58) % 16]) << 15) | ((a58 + (c58 ^ (b58 | (~d58))) + 'ha3014314 + w58[(7 * 58) % 16]) >> (32 - 15)));
      `CopyChunkWords(w59, w58)

      `CopyDigestWords(a60, d59, d60, c59, c60, b59)
      b60 <= b59 + (((a59 + (c59 ^ (b59 | (~d59))) + 'h4e0811a1 + w59[(7 * 59) % 16]) << 21) | ((a59 + (c59 ^ (b59 | (~d59))) + 'h4e0811a1 + w59[(7 * 59) % 16]) >> (32 - 21)));
      `CopyChunkWords(w60, w59)

      `CopyDigestWords(a61, d60, d61, c60, c61, b60)
      b61 <= b60 + (((a60 + (c60 ^ (b60 | (~d60))) + 'hf7537e82 + w60[(7 * 60) % 16]) << 6) | ((a60 + (c60 ^ (b60 | (~d60))) + 'hf7537e82 + w60[(7 * 60) % 16]) >> (32 - 6)));
      `CopyChunkWords(w61, w60)

      `CopyDigestWords(a62, d61, d62, c61, c62, b61)
      b62 <= b61 + (((a61 + (c61 ^ (b61 | (~d61))) + 'hbd3af235 + w61[(7 * 61) % 16]) << 10) | ((a61 + (c61 ^ (b61 | (~d61))) + 'hbd3af235 + w61[(7 * 61) % 16]) >> (32 - 10)));
      `CopyChunkWords(w62, w61)

      `CopyDigestWords(a63, d62, d63, c62, c63, b62)
      b63 <= b62 + (((a62 + (c62 ^ (b62 | (~d62))) + 'h2ad7d2bb + w62[(7 * 62) % 16]) << 15) | ((a62 + (c62 ^ (b62 | (~d62))) + 'h2ad7d2bb + w62[(7 * 62) % 16]) >> (32 - 15)));
      `CopyChunkWords(w63, w62)

      `CopyDigestWords(a64, d63, d64, c63, c64, b63)
      b64 <= b63 + (((a63 + (c63 ^ (b63 | (~d63))) + 'heb86d391 + w63[(7 * 63) % 16]) << 21) | ((a63 + (c63 ^ (b63 | (~d63))) + 'heb86d391 + w63[(7 * 63) % 16]) >> (32 - 21)));
    end
endmodule
