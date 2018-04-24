// DEFINES
`define BITS 32         // Bit width of the operands

module 	bm_expr_all_mod(clock, 
		reset_n, 
		a_in, 
		b_in,
		number_in_1,
		number_in_2,
		number_in_3,
		land_1,
		lor_1,
		lor_2,
		land_2,
		
		land_out,
		
		lor_out,
		
		leq_out,
		lneq_out,
		
		lgt_out,
		lge_out,
		llt_out,
		lle_out,
		
		add_out,
		sub_out,
		
		shift_l1_out,
		shift_r1_out,
		shift_l2_out,
		shift_r2_out,
		
		and_out,
		or_out,
		xor_out,
		xnor_out,
		
		lembed_out,
		embed_out,
		bit_sel_out,
		concat_out,
		range_select_out,
		
		number_out,
		number_out2,
		
		not_out,
		lnot_out,
		two_compliment_out
);
		
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input  b_in;

//------------------------------------------------------------
input [`BITS-1:0]	number_in_1;
input [`BITS-1:0]	number_in_2;
input [`BITS-2:0]	number_in_3;
input [`BITS-1:0]land_1;
input [`BITS-1:0]lor_1;
input lor_2;
input land_2;

//------------------------------------------------------------
output land_out;
reg land_out;

output lor_out;
reg lor_out;

output leq_out;
reg leq_out;
output lneq_out;
reg lneq_out;

output lgt_out;
reg lgt_out;
output lge_out;
reg lge_out;
output llt_out;
reg llt_out;
output lle_out;
reg lle_out;

output [`BITS-1:0]	add_out;
reg [`BITS-1:0]	add_out;
output [`BITS-1:0]	sub_out;
reg [`BITS-1:0]	sub_out;

output [`BITS-1:0]	shift_l1_out;
reg [`BITS-1:0]	shift_l1_out;
output [`BITS-1:0]	shift_r1_out;
reg [`BITS-1:0]	shift_r1_out;
output [`BITS-1:0]	shift_l2_out;
reg [`BITS-1:0]	shift_l2_out;
output [`BITS-1:0]	shift_r2_out;
reg [`BITS-1:0]	shift_r2_out;

output [`BITS-1:0]	and_out;
reg [`BITS-1:0]	and_out;
output [`BITS-1:0]	or_out;
reg [`BITS-1:0]	or_out;
output [`BITS-1:0]	xor_out;
reg [`BITS-1:0]	xor_out;
output [`BITS-1:0]	xnor_out;
reg [`BITS-1:0]	xnor_out;

output lembed_out;
reg lembed_out;
output [`BITS-1:0] embed_out;
reg [`BITS-1:0] embed_out;

always @(posedge clock)
begin
/* simple tests of all the binary expressions */
	land_out <= land_1 && land_2;
	lor_out <= lor_1 || lor_2;
	leq_out <= land_1 == lor_1;
	lneq_out <= land_1 != lor_1;
//	leq_out <= land_1 === land_2; /* not synthesizable */
//	lneq_out <= land_1 !== land_2; /* not synthesizable */
	lgt_out <= land_1 > lor_1;
	lge_out <= land_1 >= lor_1;
	lle_out <= land_1 <= lor_1;
	llt_out <= land_1 < lor_1;

	add_out <= number_in_1 + number_in_2;
	sub_out <= number_in_1 - number_in_2;

	shift_l1_out <= number_in_1 << 4'b0010;
	shift_l2_out <= number_in_1 << 3;
	shift_r1_out <= number_in_1 >> 4'b0011;
	shift_r2_out <= number_in_1 >> 1;

	and_out <= number_in_1 & number_in_3;
	or_out <= number_in_1 | number_in_3;
	xor_out <= number_in_1 ^ number_in_3;
	xnor_out <= number_in_1 ~^ number_in_3;
/* tests of embedded */
	lembed_out <= (((land_1 == lor_1) || (land_2 != lor_2) || (number_in_1 > number_in_2)) && (number_in_1 <= number_in_2));
	embed_out <= (((number_in_1 + number_in_2 - number_in_3) & (number_in_3)) | (((number_in_1 ~^ number_in_2) ^ number_in_1)));
end /* binary exp testing */

//------------------------------------------------------------
output bit_sel_out;
reg bit_sel_out;
output [`BITS-1:0] concat_out;
reg [`BITS-1:0] concat_out;
output [`BITS-1:0] range_select_out;
reg [`BITS-1:0] range_select_out;

/* simple selctions and concatenation */
always @(posedge clock)
begin
	bit_sel_out <= number_in_1[2] & land_2;
	concat_out <= {number_in_1[0], number_in_2[0], number_in_3[0], number_in_1[1]} & land_1;
	range_select_out <= number_in_1[2:1] & {land_2, land_2};
end /* bitselect and concatenation */

//------------------------------------------------------------
output [`BITS-1:0] number_out;
reg [`BITS-1:0] number_out;
output [`BITS-1:0] number_out2;
reg [`BITS-1:0] number_out2;

always @(posedge clock)
begin
	number_out <= 4'b1010 & land_1;
	number_out2 <= 4'b1010 & lor_1;
end 

//------------------------------------------------------------
output [`BITS-1:0] not_out;
reg [`BITS-1:0] not_out;
output lnot_out;
reg lnot_out;
output [`BITS-1:0] two_compliment_out;
reg [`BITS-1:0] two_compliment_out;

/* Testing simple unary operations */
always @(posedge clock)
begin
	not_out <= ~number_in_1; 
	lnot_out <= !number_in_1;  
	two_compliment_out <= -number_in_1;
end 

endmodule
