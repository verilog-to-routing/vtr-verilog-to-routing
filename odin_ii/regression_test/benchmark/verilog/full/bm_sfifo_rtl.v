/*********************************************************/
// MODULE:		Synchronous FIFO
//
// FILE NAME:	sfifo_rtl.v
// VERSION:		1.0
// DATE:		January 1, 1999
// AUTHOR:		Bob Zeidman, Zeidman Consulting
// 
// CODE TYPE:	Register Transfer Level
//
// DESCRIPTION:	This module defines a Synchronous FIFO. The
// FIFO memory is implemented as a ring buffer. The read
// pointer points to the beginning of the buffer, while the
// write pointer points to the end of the buffer. Note that
// in this RTL version, the memory has one more location than
// the FIFO needs in order to calculate the FIFO count
// correctly.
//
/*********************************************************/

// DEFINES
`define FIFO_DEPTH 15	// Depth of FIFO (number of bytes)
`define FIFO_HALF 8		// Half depth of FIFO
						// (this avoids rounding errors)
`define FIFO_BITS 4		// Number of bits required to
						// represent the FIFO size
`define FIFO_WIDTH 8	// Width of FIFO data

// TOP MODULE
module bm_sfifo_rtl(
		clock,
		reset_n,
		data_in,
		read_n,
		write_n,
		data_out,
		full,
		empty,
		half);

// INPUTS
input					clock;		// Clock input
input					reset_n;	// Active low reset
input [`FIFO_WIDTH-1:0]	data_in; 	// Data input to FIFO
input					read_n;	 	// Read FIFO (active low)
input					write_n;	// Write FIFO (active low)

// OUTPUTS
output [`FIFO_WIDTH-1:0]	data_out;	// FIFO output data
output						full;		// FIFO is full
output						empty;		// FIFO is empty
output						half;		// FIFO is half full
										// or more

// INOUTS

// SIGNAL DECLARATIONS
wire					clock;
wire					reset_n;
wire [`FIFO_WIDTH-1:0]	data_in;
wire					read_n;
wire					write_n;
reg  [`FIFO_WIDTH-1:0]	data_out;
wire					full;
wire					empty;
wire					half;

							// The FIFO memory.
reg [`FIFO_WIDTH-1:0]	fifo_mem[`FIFO_DEPTH-1:0];
							// How many locations in the FIFO
							// are occupied?
reg [`FIFO_BITS-1:0]	counter;
							// FIFO read pointer points to
							// the location in the FIFO to
							// read from next
reg [`FIFO_BITS-1:0]	rd_pointer;
							// FIFO write pointer points to
							// the location in the FIFO to
							// write to next
reg [`FIFO_BITS-1:0]	wr_pointer;

// PARAMETERS

// ASSIGN STATEMENTS
assign full = (counter == `FIFO_DEPTH) ? 1'b1 : 1'b0;
assign empty = (counter == 0) ? 1'b1 : 1'b0;
assign half = (counter >= `FIFO_HALF) ? 1'b1 : 1'b0;

// MAIN CODE

// This block contains all devices affected by the clock 
// and reset inputs
always @(posedge clock) begin
	if (~reset_n) begin
		// Reset the FIFO pointer
		rd_pointer <=  0;
		wr_pointer <=  0;
		counter <=  0;
	end
	else begin
		// If we are doing a simultaneous read and write,
		// there is no change to the counter
		if (~read_n && write_n) begin
			// Decrement the FIFO counter
			counter <=  counter - 1;		
		end
		else if (~write_n && read_n) begin
			// Increment the FIFO counter
			counter <=  counter + 1;		
		end

		if (~read_n) begin
			// Increment the read pointer
			// Check if the read pointer has gone beyond the
			// depth of the FIFO. If so, set it back to the
			// beginning of the FIFO
			if (rd_pointer == `FIFO_DEPTH-1)
				rd_pointer <=  0;
			else
				rd_pointer <=  rd_pointer + 1;
		end
		if (~write_n) begin
			// Increment the write pointer
			// Check if the write pointer has gone beyond the
			// depth of the FIFO. If so, set it back to the
			// beginning of the FIFO
			if (wr_pointer == `FIFO_DEPTH-1)
				wr_pointer <=  0;
			else
				wr_pointer <=  wr_pointer + 1;
		end
	end
end

// This block contains all devices affected by the clock 
// but not reset
always @(posedge clock) begin
	if (~read_n) begin
		// Output the data
		data_out <=  fifo_mem[rd_pointer];
	end
	if (~write_n) begin
		// Store the data
		fifo_mem[wr_pointer] <=  data_in;
	end
end
endmodule		// Sfifo
