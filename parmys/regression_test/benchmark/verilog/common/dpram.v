module dpram_instance(
	address1,
	address2,
	value_in1,
	value_in2,
	we1,
	we2,
	clock,
	value_out1,
	value_out2
);

	parameter WIDTH = 16;	// Bit width
	parameter DEPTH = 8;	// Bit depth

/*  Input Declaration */
	input	[DEPTH-1:0] 	address1;
	input	[DEPTH-1:0] 	address2;
	input 	[WIDTH-1:0]		value_in1;
	input 	[WIDTH-1:0]		value_in2;
	input 	we1;
	input 	we2;
	input   clock;

	/*  Output Declaration */
	output	[WIDTH-1:0] 	value_out1;
	output	[WIDTH-1:0] 	value_out2;

	defparam inst1.ADDR_WIDTH = DEPTH; 
	defparam inst1.DATA_WIDTH = WIDTH;
	dual_port_ram inst1 (
		.addr1	( address1 ),
		.addr2	( address2 ),
		.data1	( value_in1 ),
		.data2	( value_in2 ),
		.we1	( clock ),
		.we2	( clock ),
		.clk    ( clock ),
		.out1	( value_out1 ),
		.out2	( value_out2 )
	);

endmodule