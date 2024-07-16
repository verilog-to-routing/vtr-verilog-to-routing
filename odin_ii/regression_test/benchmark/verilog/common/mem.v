// DEFINES
`define DEPTH 16 	// Depth of memory
`define ADDR_WIDTH 4
`define DATA_WIDTH 8		// Width of memory

// TOP MODULE
module mem_read(
		clock,
		rd_addr,
		wr_addr,
        rd_data,
        wr_data,
        en
		);

	// INPUTS
	input				   	clock;		// Clock input
	input [`ADDR_WIDTH-1:0]	rd_addr;	// Read address
	input [`ADDR_WIDTH-1:0]	wr_addr;	// write address
	input [`DATA_WIDTH-1:0] wr_data; 	// write data
	input					en;	 		// Read enable

	// OUTPUTS
	output reg [`DATA_WIDTH-1:0] rd_data; 	// Read data



	// The memory block.
	reg [`DATA_WIDTH-1:0] block_mem [`DEPTH-1:0];

	always @(posedge clock) begin
		rd_data <= block_mem[rd_addr];
		block_mem[wr_addr] <= wr_data;	
	end
endmodule
