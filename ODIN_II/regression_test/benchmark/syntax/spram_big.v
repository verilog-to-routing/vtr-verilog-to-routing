// DEFINES
`define WIDTH 16         // Bit width
`define DEPTH 8         // Bit depth

module  spram_big(
	clock,
	reset_n,
	value_out,
	value_in
);

/*  Input Declaration */
	input   clock;
	input   reset_n;
	input 	[`WIDTH-1:0]	value_in;

	/*  Output Declaration */
	output	[`WIDTH-1:0] 	value_out;

	reg		[`DEPTH-1:0] address_counter;
	reg		temp_reset;

	single_port_ram inst1(
		.clk	( clock ),
		.we		( clock ),
		.data	( value_in ),
		.out	( value_out ),
		.addr	( address_counter )
	);

	always @(posedge clock)
	begin
		if(	temp_reset != 0 )
		begin
			temp_reset <= 0;
			address_counter <= 0;
		end
		else if (reset_n == 1)
			address_counter <= 0;
		else
			address_counter <= address_counter + 1;
		
	end

endmodule

