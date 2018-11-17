/**
* 	TODO: No support for falling edge yet, it defaults to rising edge (as before)
*	implementation is done throughout and can be outputted, but mixing negedge and posedge in the same always block
*	result in failures, also implicit memories only support rising edge, need a fix.
*/

`define WORD_SIZE 4
`define MEMORY_DEPTH_POWER 3
`define MEMORY_BIT_SIZE 2**3 

module multiclock_reader_writer(
    clock,
	addr_read,
	addr_write,
	in,
	out
);

	//INPUT
	input	clock;
	input	[`MEMORY_DEPTH_POWER-1:0]	addr_read;
	input	[`MEMORY_DEPTH_POWER-1:0]	addr_write;
	input	[`WORD_SIZE-1:0]	in;


	//OUTPUT
	output	[`WORD_SIZE-1:0]	out;

	reg 	[`MEMORY_BIT_SIZE-1:0]	register[`WORD_SIZE-1:0];

	always @(negedge clock)
		out <= register[addr_read];


	always @(posedge clock)
		register[addr_write] <= in;

endmodule