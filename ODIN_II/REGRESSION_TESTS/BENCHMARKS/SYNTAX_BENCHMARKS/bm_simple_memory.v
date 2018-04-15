// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_simple_memory(clock, 
		reset_n, 
		value_out,
		value_in
		);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;
input 	[`BITS-1:0]value_in;

output [`BITS-1:0] value_out;
wire [`BITS-1:0]    value_out;

reg [`BITS-1:0] memory [3:0]; // 4 memory slots of Bits wide
reg [1:0] address_counter;
reg [1:0] address_counter2;
wire [`BITS-1:0] temp;

always @(posedge clock)
begin
	address_counter <= 2'b00;
	address_counter2 <= 2'b01;
	if (reset_n == 1'b1)
		memory[address_counter] <= value_in;
end

assign value_out = memory[address_counter2];

endmodule
