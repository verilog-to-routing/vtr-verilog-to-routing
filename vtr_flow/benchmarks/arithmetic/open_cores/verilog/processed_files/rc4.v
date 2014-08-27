
    // RC4 PRGA module implementation
    // Copyright 2012 - Alfredo Ortega
	// aortega@alu.itba.edu.ar
	// aortega@groundworkstech.com

 // This library is free software: you can redistribute it and/or
 // modify it under the terms of the GNU Lesser General Public
 // License as published by the Free Software Foundation, either
 // version 3 of the License, or (at your option) any later version.

 // This library is distributed in the hope that it will be useful,
 // but WITHOUT ANY WARRANTY; without even the implied warranty of
 // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 // Lesser General Public License for more details.

 // You should have received a copy of the GNU Lesser General Public
 // License along with this library.  If not, see <http://www.gnu.org/licenses/>.



	// RC4 PRGA constants
	// Copyright 2012 - Alfredo Ortega
	// aortega@alu.itba.edu.ar

 // This library is free software: you can redistribute it and/or
 // modify it under the terms of the GNU Lesser General Public
 // License as published by the Free Software Foundation, either
 // version 3 of the License, or (at your option) any later version.

 // This library is distributed in the hope that it will be useful,
 // but WITHOUT ANY WARRANTY; without even the implied warranty of
 // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 // Lesser General Public License for more details.

 // You should have received a copy of the GNU Lesser General Public
 // License along with this library.  If not, see <http://www.gnu.org/licenses/>.

//%%GENDEFINE%% (choose_to,   nonblocking, 0, 256 )
//%%GENDEFINE%% (choose_from, blocking,    0, 256 )
//%%GENDEFINE%% (choose_from, nonblocking, 0, 256 )
//%%GENDEFINE%% (choose_to,   nonblocking, 0, 255 )
//%%GENDEFINE%% (choose_from, blocking,    0, 255 )
//%%GENDEFINE%% (choose_from, nonblocking, 0, 255 )

//%%GENDEFINE%% (choose_to,   nonblocking, 0, 6   )
//%%GENDEFINE%% (choose_from, blocking,    0, 6   )
//%%GENDEFINE%% (choose_from, blocking,    0, 6   )

//%%GENDEFINE%% (always_list, 0, 256)
//%%GENDEFINE%% (always_list, 0, 255)
//%%GENDEFINE%% (always_list, 0, 6)

//%%GENDEFINE%% (mod_op, blocking, 7, 0, 255)

`define KEY_SIZE 7


module rc4(clk,rst,output_ready,password_input,K);

input clk; // Clock
input rst; // Reset
input [7:0] password_input; // Password input
output output_ready; // Output valid
output [7:0] K; // Output port


wire clk, rst; // clock, reset
reg output_ready;
wire [7:0] password_input;


 // RC4 PRGA

// Key
reg [7:0] key [`KEY_SIZE -1:0];
// S array
reg [7:0] S [256:0];
reg [10:0] discardCount;

// Key-scheduling state
`define KSS_KEYREAD 4'h0
`define KSS_KEYSCHED1 4'h1
`define KSS_KEYSCHED2 4'h2
`define KSS_KEYSCHED3 4'h3
`define KSS_CRYPTO 	 4'h4
`define KSS_SWAP_REGS_KS2 4'h5
`define KSS_SWAP_REGS_KS3 4'h6
`define KSS_SWAP_REGS_CRYPTO 4'h7

// Variable names from http://en.wikipedia.org/wiki/RC4
reg [3:0] KSState;
reg [7:0] i; // Counter
reg [7:0] j;
reg [7:0] K;


reg [7:0] S_of_i;
reg [7:0] S_of_i_plus_1;
reg [7:0] S_of_j;
wire [7:0] S_of_i_plus_S_of_j;

reg [7:0] prev_i;
reg [7:0] prev_S_of_j;

reg [7:0]  key_of_i_mod_KEYSIZE;
reg [7:0] i_mod_KEYSIZE;

always @ (i or `always_list_0_255(S)) begin
	`choose_from_blocking_0_255(i,S_of_i,S)
end
always @ (i or `always_list_0_256(S)) begin
	`choose_from_blocking_0_256(i+1,S_of_i_plus_1,S)
end
always @ (j or `always_list_0_255(S)) begin
	`choose_from_blocking_0_255(j,S_of_j,S)
end
always @ (i_mod_KEYSIZE or `always_list_0_6(key)) begin
	`choose_from_blocking_0_6(i_mod_KEYSIZE,key_of_i_mod_KEYSIZE,key)
end
always @ (i) begin
	`mod_op_blocking_7_0_255(i,i_mod_KEYSIZE)
end

assign S_of_i_plus_S_of_j = S_of_i + S_of_j;

always @ (posedge clk or posedge rst) begin
	if (rst) begin
		i <= 8'h0;
		KSState <= `KSS_KEYREAD;
		output_ready <= 0;
		j <= 0;
	end else begin
		prev_i <= i;
		prev_S_of_j <= S_of_j;
		case (KSState)
			`KSS_KEYREAD: begin // KSS_KEYREAD state: Read key from input
				if (i == `KEY_SIZE) begin
					KSState <= `KSS_KEYSCHED1;
					i<=8'h00;
				end else begin
					i <= i+1;
					// in place of:
					// key[i] <= password_input;
					`choose_to_nonblocking_0_6(i,key,password_input)
					// $display ("rc4: key[%d] = %08X",i,password_input);
				end
			end

	// for i from 0 to 255
	//     S[i] := i
	// endfor

			`KSS_KEYSCHED1: begin // KSS_KEYSCHED1: Increment counter for S initialization
				// in place of S[i] <= i;
				`choose_to_nonblocking_0_256(i,S,i)
				if (i == 8'hFF) begin
					KSState <= `KSS_KEYSCHED2;
					i <= 8'h00;
				end else begin
					i <= i +1;
				end
			end

	// j := 0
	// for i from 0 to 255
	//     j := (j + S[i] + key[i mod keylength]) mod 256
	//     swap values of S[i] and S[j]
	// endfor

			`KSS_KEYSCHED2: begin // KSS_KEYSCHED2: Initialize S array
				// in place of:
				// j <= (j + S[i] + key[i % `KEY_SIZE]);
				j <= (j + S_of_i + key_of_i_mod_KEYSIZE);
				KSState <= `KSS_KEYSCHED3;
			end

			`KSS_KEYSCHED3: begin // KSS_KEYSCHED3: S array permutation
				// in place of:
				// S[i]<=S[j]; , see KSS_SWAP_REGS*
				// in place of:
				// S[j]<=S[i];
				`choose_to_nonblocking_0_256(j,S,S_of_i)
				if (i == 8'hFF) begin
					KSState <= `KSS_SWAP_REGS_CRYPTO;
					i <= 8'h01;
					j <= S[1];
					discardCount <= 11'h0;
					output_ready <= 0; // K not valid yet
				end else begin
					i <= i + 1;
					KSState <= `KSS_SWAP_REGS_KS2;
				end
			end

	// i := 0
	// j := 0
	// while GeneratingOutput:
	//     i := (i + 1) mod 256
	//     j := (j + S[i]) mod 256
	//     swap values of S[i] and S[j]
	//     K := S[(S[i] + S[j]) mod 256]
	//     output K
	// endwhile

			`KSS_CRYPTO: begin
				// in place of:
				// S[i] <= S[j], see KSS_SWAP_REGS*
				// in place of:
				// S[j] <= S[i]; // We can do this because of verilog.
				`choose_to_nonblocking_0_256(j,S,S_of_i)
				// in place of:
				// K <= S[ S[i]+S[j] ];
				`choose_from_nonblocking_0_256(S_of_i_plus_S_of_j,K,S)
				if (discardCount<11'h600) begin // discard first 1536 values / RFC 4345
					discardCount<=discardCount+1;
				end else begin
					output_ready <= 1; // Valid K at output
				end

				i <= i+1;
				// Here is the secret of 1-clock: we develop all possible values of j in the future
				if (j==i+1) begin
				     // j <= (j + S[i]);
				     j <= (j + S_of_i);
				end else begin
					if (i==255) begin
						j <= (j + S[0]);
					end else begin
						// in place of:
						// j <= (j + S[i+1]);
						j <= (j + S_of_i_plus_1);
					end
				end
				//$display ("rc4: output = %08X",K);
			end

			default: begin
				`choose_to_nonblocking_0_256(prev_i,S,prev_S_of_j)
				case (KSState)
					`KSS_SWAP_REGS_KS2:    KSState <= `KSS_KEYSCHED2;
					`KSS_SWAP_REGS_KS3:    KSState <= `KSS_KEYSCHED3;
					`KSS_SWAP_REGS_CRYPTO: KSState <= `KSS_CRYPTO;
				endcase
			end
		endcase
	end
end

endmodule

