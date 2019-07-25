`define BITS 4

module binops(
	in_1, in_2, 
	or_o, 
	nor_o, 
	and_o, 
	nand_o, 	
	xor_o,  
	xnor_o,

	add_o, 
	sub_o,
	
	mul_o, 
	//div_o, 

	equ_o, 
	neq_o, 

	gt_o, 
	lt_o,		
	geq_o, 
	leq_o
); 

input  [`BITS-1:0] in_1, in_2; 
output [`BITS-1:0] 
	or_o, 
	nor_o, 
	and_o, 
	nand_o, 	
	xor_o,  
	xnor_o,

	add_o, 
	sub_o,

	mul_o, 
	//div_o, 

	equ_o, 
	neq_o,

	gt_o, 
	lt_o,
	geq_o, 
	leq_o; 		


assign or_o    = in_1  | in_2; 
assign nor_o   = in_1 ~| in_2; 
assign and_o   = in_1  & in_2; 
assign nand_o  = in_1 ~& in_2;  
assign xor_o   = in_1  ^ in_2;
assign xnor_o  = in_1 ~^ in_2;  

assign add_o   = in_1  + in_2; 
assign sub_o   = in_1  - in_2;

assign mul_o   = in_1  * in_2;  
//assign div_o   = in_1 / in_2; 

assign equ_o   = in_1 == in_2; 
assign neq_o   = in_1 != in_2; 

assign gt_o    = in_1 >  in_2; 
assign lt_o    = in_1 <  in_2; 
assign geq_o   = in_1 >= in_2; 
assign leq_o   = in_1 <= in_2; 

endmodule
	
