// DEFINES
// TODO unable to run on buildbot altho other similar s=circuit do. investigate
`define BITS 3       // Bit width of the operands
`define DEPTH 8
module 	bm_simple_memory(
	clock, 
	value_out,
	address_in,
	address_out
);

// SIGNAL DECLARATIONS
input	clock;
input 	[`BITS:0] address_in;
input 	[`BITS:0] address_out;

output 	[`BITS-1:0] value_out;

reg 	[`BITS-1:0] memory [`DEPTH-1:0]; // 4 memory slots of Bits wide


always @(posedge clock)
begin
	memory[address_in] <= address_in;
end

assign value_out = memory[address_out];

endmodule
