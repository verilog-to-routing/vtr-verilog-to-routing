// DEFINES
`define DEPTH 16 	// Depth of memory
`define ADDR_WIDTH 4
`define DATA_WIDTH 32		// Width of memory

// TOP MODULE
module R_twoW(
		clock,
		addr1,
		addr2,
        rd_data1,
        wr_data1,
        wr_data2,
        en1,
        en2,
        en1_out,
        en2_out
		);

	// INPUTS
	input				   	clock;		// Clock input
	input [`ADDR_WIDTH-1:0]	addr1;	    // address 1
	input [`ADDR_WIDTH-1:0]	addr2;	    // address 2
	input [`DATA_WIDTH-1:0] wr_data1; 	// write data 1
	input [`DATA_WIDTH-1:0] wr_data2; 	// write data 2
	input					en1;	 	// Read enable 1
	input					en2;	 	// Read enable 2

	// OUTPUTS
	output reg [`DATA_WIDTH-1:0] rd_data1; 	// Read data 1
	output en1_out, en2_out;



	// The memory block.
	reg [`DATA_WIDTH-1:0] block_mem [`DEPTH-1:0];

	always @(posedge clock) begin
  		if (en1) begin
  		      block_mem[addr1] <= wr_data1;	
  		end
		rd_data1 <= block_mem[addr1];
	end
	always @(posedge clock) begin
        if (en2) begin
        	block_mem[addr2] <= wr_data2;	
        end
	end

	assign en1_out = en1;
	assign en2_out = en2;

endmodule
