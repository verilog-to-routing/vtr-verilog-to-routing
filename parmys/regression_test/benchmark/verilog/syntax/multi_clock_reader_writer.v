/**
* 	TODO: implicit memories with multiclock input (one for read and one for write)
* 	is broken, We use manualy driven register here to test multiple clock for this reason
*	using 2D array will break multiclock test bed
*/

`define WORD_SIZE 4
`define MEMORY_DEPTH_POWER 3
`define MEMORY_BIT_SIZE 32 //(2**3)*4

module multiclock_reader_writer(
    clock_reader_head,
    clock_writer_head,
	reset,
	in,
	out
);

	//INPUT
	input	clock_reader_head;
	input	clock_writer_head;
	input	reset;
	input	[`WORD_SIZE-1:0]	in;


	//OUTPUT
	output	[`WORD_SIZE-1:0]	out;

	reg	[`MEMORY_DEPTH_POWER-1:0]	reader_head;
	reg	[`MEMORY_DEPTH_POWER-1:0]	writer_head;
	reg [`MEMORY_BIT_SIZE-1:0]	register;

	always @(posedge clock_reader_head)
	begin
		case (reset)
			1'b0: begin
				case (reader_head)	
					3'b000: begin 
						register[(1*`MEMORY_DEPTH_POWER)-1:(0*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b001;
						end
					3'b001: begin 
						register[(2*`MEMORY_DEPTH_POWER)-1:(1*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b010;
						end
					3'b010: begin 
						register[(3*`MEMORY_DEPTH_POWER)-1:(2*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b011;
						end
					3'b011: begin 
						register[(4*`MEMORY_DEPTH_POWER)-1:(3*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b100;
						end
					3'b100: begin 
						register[(5*`MEMORY_DEPTH_POWER)-1:(4*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b101;
						end
					3'b101: begin 
						register[(6*`MEMORY_DEPTH_POWER)-1:(5*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b110;
						end
					3'b110: begin 
						register[(7*`MEMORY_DEPTH_POWER)-1:(6*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b111;
						end
					3'b111: begin 
						register[(8*`MEMORY_DEPTH_POWER)-1:(7*`MEMORY_DEPTH_POWER)] <= in;
						reader_head <= 3'b0;
						end
					default: begin 
						register[0] <= 1'b0;
						reader_head <= 3'b0;
						end
				endcase
				end
			default: begin
				register = 8'b0;
				reader_head <= 3'b0;
				end
		endcase
	end

	always @(posedge clock_writer_head)
	begin
		case (reset)
			1'b0: begin
				case (writer_head)	
					3'b000: begin 
						out <= register[(1*`MEMORY_DEPTH_POWER)-1:(0*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b001;
						end
					3'b001: begin 
						out <= register[(2*`MEMORY_DEPTH_POWER)-1:(1*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b010;
						end
					3'b010: begin 
						out <= register[(3*`MEMORY_DEPTH_POWER)-1:(2*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b011;
						end
					3'b011: begin 
						out <= register[(4*`MEMORY_DEPTH_POWER)-1:(3*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b100;
						end
					3'b100: begin 
						out <= register[(5*`MEMORY_DEPTH_POWER)-1:(4*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b101;
						end
					3'b101: begin 
						out <= register[(6*`MEMORY_DEPTH_POWER)-1:(5*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b110;
						end
					3'b110: begin 
						out <= register[(7*`MEMORY_DEPTH_POWER)-1:(6*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b111;
						end
					3'b111: begin 
						out <= register[(8*`MEMORY_DEPTH_POWER)-1:(7*`MEMORY_DEPTH_POWER)];
						writer_head <= 3'b0;
						end
					default: begin 
						out <=  register[0];
						writer_head <= 3'b0;
						end
				endcase
				end
			default: begin
				out <= 1'b0;
				writer_head <= 3'b0;
				end
		endcase
	end

endmodule