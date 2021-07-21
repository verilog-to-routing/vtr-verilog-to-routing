/* 
 * This is a test to verify odin's functionality while 
 * the second_module ports are not in a regular order
 */
 
`define WIDTH 2
module top_module (
	input	[`WIDTH-1:0]	i1,
	input	[`WIDTH-1:0]	i2,
	output	[`WIDTH-1:0]	o1,
	output	[`WIDTH-1:0]	o2
);

   // sample module to check the unordered input
   logical_not uut (
					.i1(i1),
				    .i2(i2),
				    .o2(o2),
				    .o1(o1)
			 	   );

endmodule

module logical_not (
	output	[`WIDTH-1:0]	o1,
	input	[`WIDTH-1:0]	i1,
	input	[`WIDTH-1:0]	i2,
	output	[`WIDTH-1:0]	o2
);
   assign o1 = !i1;
   assign o2 = !i2;
   
endmodule
