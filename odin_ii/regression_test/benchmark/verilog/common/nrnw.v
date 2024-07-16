// DEFINES
`define DEPTH 16 	// Depth of memory
`define ADDR_WIDTH 4
`define DATA_WIDTH 8		// Width of memory

// TOP MODULE
module twoR_twoW(
		clock,
		addr1,
		addr2,
		addr3,
        rd_data1,
        rd_data2,
        rd_data3,
        wr_data1,
        wr_data2,
        wr_data3,
        en1,
        en2,
        en3
		);

	// INPUTS
	input				   	clock;		// Clock input
	input [`ADDR_WIDTH-1:0]	addr1;	    // address 1
	input [`ADDR_WIDTH-1:0]	addr2;	    // address 2
	input [1:0]	addr3;	    // address 3
	input [`DATA_WIDTH-1:0] wr_data1; 	// write data 1
	input [`DATA_WIDTH-1:0] wr_data2; 	// write data 2
	input [`DATA_WIDTH-1:0] wr_data3; 	// write data 3
	input					en1;	 	// Read enable 1
	input					en2;	 	// Read enable 2
	input					en3;	 	// Read enable 3

	// OUTPUTS
	output reg [`DATA_WIDTH-1:0] rd_data1; 	// Read data 1
 	output reg [`DATA_WIDTH-1:0] rd_data2; 	// Read data 2
 	output reg [`DATA_WIDTH-1:0] rd_data3; 	// Read data 3



	// The memory block.
	reg [`DATA_WIDTH-1:0] block_mem [`DEPTH-1:0];

	always @(posedge clock) begin
		if (en1) begin
			rd_data1 <= block_mem[addr1];
		    block_mem[addr1] <= wr_data1;	
		end
		else if(en2) begin
			rd_data2 <= block_mem[addr2];
   		    block_mem[addr2] <= wr_data2;
    	end
    	else if(en3) begin    
	        rd_data3 <= block_mem[addr3];
   		    block_mem[addr3] <= wr_data3;	
	    end
	end

endmodule
