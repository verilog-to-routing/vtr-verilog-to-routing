
`define WORD_SIZE 2
`define MEMORY_DEPTH_POWER 2
`define MEMORY_BIT_SIZE (2**2)*2

module multiclock_reader_writer(
	clock,
	addr_read,
	addr_write,
	in,
	out
);

	//INPUT
	input	clock;
	input	[1:0]	addr_read;
	input	[1:0]	addr_write;
	input	[1:0]	in;

	//OUTPUT
	output	[1:0]	out;

	reg [1:0]	temp_addr_out;
	reg [1:0]	register[1:0];

	assign out = register[temp_addr_out];

	always @(posedge clock)
	begin
		register[addr_write] = in;
		temp_addr_out <= addr_read;
	end

endmodule
