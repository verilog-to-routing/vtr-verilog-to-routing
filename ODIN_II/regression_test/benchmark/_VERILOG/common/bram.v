// DEFINES
`define DEPTH 16 	// Depth of memory
`define ADDR_WIDTH 8
`define DATA_WIDTH 8		// Width of memory

// TOP MODULE
module block_ram(
		clock,
		rd_addr,
        rd_data,
        rd_en,
        wr_addr,
        wr_data,
        wr_en
		);

	// INPUTS
	input				   	clock;		// Clock input
	input [`ADDR_WIDTH-1:0]	rd_addr;	// Read address
	input [`DATA_WIDTH-1:0]	wr_data;	// Write data
	input					rd_en;	 	// Read enable
	input [`ADDR_WIDTH-1:0]	wr_addr;	// Write address
	input					wr_en;		// Write enable

	// OUTPUTS
	output reg [`DATA_WIDTH-1:0] rd_data; 	// Read data


	// The memory block.
	reg [`DATA_WIDTH-1:0] block_mem [`DEPTH-1:0];

	always @(posedge clock) begin
		if (rd_en) begin
			rd_data <= block_mem[rd_addr];	
		end
		else if (wr_en) begin
			block_mem[wr_addr] <= wr_data;
		end
		
	end
endmodule
