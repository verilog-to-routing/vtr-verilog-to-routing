// DEFINES
`define BITS 4
`define MEMORY_WORDS 16

module 	bm_base_memory(clock, 
		we, 
		address_in,
		address_out,
		value_out,
		out1,
		out2,
		value_in
		);

// SIGNAL DECLARATIONS
input	clock;
input 	we;
input 	[`BITS-1:0]value_in;
input 	[`BITS-1:0]address_in;
input 	[`BITS-1:0]address_out;

output [`BITS-1:0] value_out;
wire [`BITS-1:0]   value_out;
output [`BITS-1:0] out1;
output [`BITS-1:0] out2;
reg [`BITS-1:0]    out1;
reg [`BITS-1:0]    out2;

reg [`BITS-1:0] address;
reg [`BITS-1:0] memory [`MEMORY_WORDS-1:0]; // 4 memory slots of Bits wide
wire [`BITS-1:0] temp;

always @(posedge clock)
begin
	address <= address_in;
	if (we == 1'b1)
		memory[address] <= value_in;
end

assign value_out = memory[address_out];

always @(posedge clock)
begin
	out1 <= value_out & address_in;
	out2 <= out1 & 1'b0;
end

endmodule
