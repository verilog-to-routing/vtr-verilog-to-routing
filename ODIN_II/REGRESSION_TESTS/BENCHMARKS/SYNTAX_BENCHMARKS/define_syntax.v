//Testing the ifdef.


// decide if this is an and or an or module
// if neither it should be a xor
`define and_module
//`define or_module


module define_syntax (a,b,c);
	
	input a;
	input b;
	output c;
	wire c_wire;
	assign c = c_wire;
	
	`ifdef and_module
		assign c_wire = a & b;
	`elsif or_module
		assign c_wire = a | b;
	`else
		assign c_wire = a ^ b;
	`endif
	
endmodule
	
	
	
	
	