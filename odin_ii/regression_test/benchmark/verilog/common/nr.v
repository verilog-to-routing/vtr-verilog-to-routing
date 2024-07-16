// DEFINES
`define DEPTH 16 	// Depth of memory
`define ADDR_WIDTH 4
`define DATA_WIDTH 8		// Width of memory

// TOP MODULE
module twoR_twoW(
		clock,
		en1,
		en2,
		en3,
		addr1,
		addr2,
		addr3,
        rd_data1,
        rd_data2,
        rd_data3
		);

	// INPUTS
	input				   	clock;		// Clock input
	input				   	en1;		// en1 input
	input				   	en2;		// en2 input
	input				   	en3;		// en3 input
	
	input [`ADDR_WIDTH-1:0]	addr1;	    // address 1
	input [`ADDR_WIDTH-1:0]	addr2;	    // address 2
	input [`ADDR_WIDTH-1:0]	addr3;	    // address 3

	// OUTPUTS
	output reg [`DATA_WIDTH-1:0] rd_data1; 	// Read data 1
 	output reg [`DATA_WIDTH-1:0] rd_data2; 	// Read data 2
 	output reg [`DATA_WIDTH-1:0] rd_data3; 	// Read data 3



	// The memory block.
	reg [`DATA_WIDTH-1:0] block_mem [`DEPTH-1:0];

	always @(posedge clock) begin
		if (en1) begin
			rd_data1 <= block_mem[addr1];
		end
		else if (en2) begin
			rd_data2 <= block_mem[addr2];
		end
		else if (en3) begin
			rd_data3 <= block_mem[addr3];
		end
	end

endmodule
