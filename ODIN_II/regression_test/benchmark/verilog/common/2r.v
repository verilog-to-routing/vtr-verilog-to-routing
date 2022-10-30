// DEFINES
`define DEPTH 16 	// Depth of memory
`define ADDR_WIDTH $clog2(`DEPTH)
`define DATA_WIDTH 8		// Width of memory

// TOP MODULE
module twoR_twoW(
		clock,
		addr1,
		addr2,
        rd_data1,
        rd_data2
		);

	// INPUTS
	input				   	clock;		// Clock input
	input [`ADDR_WIDTH-1:0]	addr1;	    // address 1
	input [`ADDR_WIDTH-1:0]	addr2;	    // address 2

	// OUTPUTS
	output reg [`DATA_WIDTH-1:0] rd_data1; 	// Read data 1
 	output reg [`DATA_WIDTH-1:0] rd_data2; 	// Read data 2



	// The memory block.
	reg [`DATA_WIDTH-1:0] block_mem [`DEPTH-1:0];

	always @(posedge clock) begin
		rd_data1 <= block_mem[addr1];
	end
	always @(posedge clock) begin
		rd_data2 <= block_mem[addr2];
	end

endmodule
