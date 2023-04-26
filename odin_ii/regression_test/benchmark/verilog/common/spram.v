module spram_instance(
	address,
	value_in,
	we,
	clock,
	value_out
);

	parameter WIDTH = 16;	// Bit width
	parameter DEPTH = 8;	// Bit depth

/*  Input Declaration */
	input	[DEPTH-1:0] 	address;
	input 	[WIDTH-1:0]		value_in;
	input 	we;
	input   clock;

	/*  Output Declaration */
	output	[WIDTH-1:0] 	value_out;

	defparam inst1.ADDR_WIDTH = DEPTH; 
	defparam inst1.DATA_WIDTH = WIDTH;
	single_port_ram inst1 (
		.addr	( address ),
		.data	( value_in ),
		.we		( we ),
		.clk    ( clock ),
		.out	( value_out )
	);

endmodule