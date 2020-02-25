module mul_tc(A,B, out); 
	input [7:0] A; 
	input [8:0] B; 
	output [16:0] out;
	
	wire sgn; 
	wire [7:0] over_A;
	wire [8:0] over_B;
	wire [16:0] out_1;
	
	assign sgn = (A[7] ^ B[8]);
	assign over_A = (A[7] == 1'b1)? (~A + 1'b1): A;
	assign over_B = (B[8] == 1'b1)? (~B + 1'b1): B;
	assign out_1 = over_A * over_B;
	assign out = (sgn == 1'b1)? (~out_1 + 1'b1): out_1;

endmodule

module fir_scu_rtl_restructured_for_cmm_exp (clk, reset, sample, result); 
input clk, reset; 
input [7:0] sample; 
output [9:0] result; 
reg [9:0] result; 
reg [7:0] samp_latch; 
reg [16:0] pro; 
reg [18:0] acc; 
reg [19:0] clk_cnt; 
reg [7:0] shift_0,shift_1,shift_2,shift_3,shift_4,shift_5,shift_6,shift_7,shift_8,shift_9,shift_10,shift_11,shift_12,shift_13,shift_14,shift_15,shift_16; 

wire [7:0] shift_select;
wire [8:0] coef_select;

`define coefs_0 9'b111111001;
`define coefs_1 9'b111111011;
`define coefs_2 9'b000001101; 
`define coefs_3 9'b000010000;
`define coefs_4 9'b111101101;
`define coefs_5 9'b111010110; 
`define coefs_6 9'b000010111;
`define coefs_7 9'b010011010;
`define coefs_8 9'b011011110; 
`define coefs_9 9'b010011010;
`define coefs_10 9'b000010111;
`define coefs_11 9'b111010110; 
`define coefs_12 9'b111101101;
`define coefs_13 9'b000010000;
`define coefs_14 9'b000001101; 
`define coefs_15 9'b111111011;
`define coefs_16 9'b111111001;
						
/****************start of the fir filter operation*********************/ 
always @(posedge clk) 
begin 
	if (reset) 
	begin 
		clk_cnt<={18'b000000000000000000,1'b1};
		
		result<=10'h000; 
		
		shift_0<=8'h00; 
		shift_1<=8'h00;
		shift_2<=8'h00;
		shift_3<=8'h00;
		shift_4<=8'h00;
		shift_5<=8'h00; 
		shift_6<=8'h00;
		shift_7<=8'h00;
		shift_8<=8'h00;
		shift_9<=8'h00;
		shift_10<=8'h00; 
		shift_11<=8'h00;
		shift_12<=8'h00;
		shift_13<=8'h00;
		shift_14<=8'h00;
		shift_15<=8'h00; 
		shift_16<=8'h00; 
		
		samp_latch<=8'h00; 
		acc<=18'o000000; 
		pro<=17'h00000; 
	end 
	else 
	begin 
		clk_cnt<={clk_cnt[18:0],clk_cnt[19]}; 
		
		if(clk_cnt[0]) 
		begin 
			samp_latch<= sample; 
			acc<=18'h00000; 
		end 
		else if(clk_cnt[1]) 
		begin
			pro <= 1'b0;
			mul_tc mul_1 (samp_latch,coefs_0, pro); 
			acc<=18'h00000; 
		end
		else if (clk_cnt[2]) 
		begin 
			mul_tc mul_2 (shift_15,coefs_16, pro); 
			shift_16<=shift_15; 
		end 
		else if (clk_cnt)
		begin
			// acc moved out here as common factored calculation
			acc<=acc+{ pro[16], pro[16], pro }; 
			if (clk_cnt[3]) 
			begin 
				mul_tc mul_3 (shift_14,coefs_15, pro); 
				shift_15<=shift_14; 
			end 
			else if (clk_cnt[4]) 
			begin 
				mul_tc mul_4 (shift_13,coefs_14, pro); 
				shift_14<=shift_13; 
			end 
			else if (clk_cnt[5]) 
			begin 
				mul_tc mul_5 (shift_12,coefs_13, pro); 
				shift_13<=shift_12; 
			end 
			else if (clk_cnt[6]) 
			begin 
			mul_tc mul_6 (shift_11,coefs_12, pro); 
				shift_12<=shift_11; 
			end 
			else if (clk_cnt[7]) 
			begin 
			mul_tc mul_7 (shift_10,coefs_11, pro); 
				shift_11<=shift_10; 
			end 
			else if (clk_cnt[8]) 
			begin 
			mul_tc mul_8 (shift_9,coefs_10, pro); 
				shift_10<=shift_9; 
			end 
			else if (clk_cnt[9]) 
			begin 
			mul_tc mul_9 (shift_8,coefs_9, pro); 
				shift_9<=shift_8; 
			end 
			else if (clk_cnt[10]) 
			begin 
			mul_tc mul_10 (shift_7,coefs_8, pro); 
				shift_8<=shift_7; 
			end 
			else if (clk_cnt[11]) 
			begin 
			mul_tc mul_11 (shift_6,coefs_7, pro); 
					shift_7<=shift_6; 
			end 
			else if (clk_cnt[12]) 
			begin 
			mul_tc mul_12 (shift_5,coefs_6, pro); 
					shift_6<=shift_5; 
			end 
			else if (clk_cnt[13]) 
			begin 
			mul_tc mul_13 (shift_4,coefs_5, pro); 
					shift_5<=shift_4; 
			end 
			else if (clk_cnt[14]) 
			begin 
			mul_tc mul_14 (shift_3,coefs_4, pro); 
					shift_4<=shift_3; 
			end 
			else if (clk_cnt[15]) 
			begin 
			mul_tc mul_15 (shift_2,coefs_3, pro); 
					shift_3<=shift_2; 
			end 
			else if (clk_cnt[16]) 
			begin 
			mul_tc mul_16 (shift_1,coefs_2, pro); 
					shift_2<=shift_1; 
			end 
			else if (clk_cnt[17]) 
			begin 
			mul_tc mul_17 (shift_0,coefs_1, pro); 
					shift_1<=shift_0; 
					shift_0<=samp_latch; 
			end 
			// else if (clk_cnt[18]) 
			// begin 
			// 		acc<=acc+{pro[16],pro[16],pro}; 
			// end 
			else if (clk_cnt[19])
			begin
				result<=acc[18:9]; 
			end
		end
	end 
end 
endmodule
