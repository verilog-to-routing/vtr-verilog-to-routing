`define WORD_SIZE 32
`define DEPTH_LOG2 4
`define RAM_DEPTH 2**4

module inferred_dpram(
	clk,
	address_a,
	address_b,
	data_a,
	data_b,
	data_out_a,
	data_out_b
);

	input clk;

	input [`DEPTH_LOG2-1:0] address_a;
	input [`DEPTH_LOG2-1:0] address_b;

	input [`WORD_SIZE-1:0] data_a;
	input [`WORD_SIZE-1:0] data_b;

	output [`WORD_SIZE-1:0] data_out_a;
	output [`WORD_SIZE-1:0] data_out_b;

	reg  [`WORD_SIZE-1:0] mregs [`RAM_DEPTH-1:0];  

	always @ (posedge clk )
	begin
		// First port
		mregs[address_a] <= data_a;
		data_out_a <= mregs[address_a];

		// Second port
		mregs[address_b] <= data_b;
		data_out_b <= mregs[address_b];
	end

endmodule
