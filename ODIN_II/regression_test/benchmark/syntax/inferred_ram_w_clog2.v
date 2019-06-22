`define WORD_SIZE 32
`define RAM_DEPTH 2**5
`define DEPTH_LOG2 $clog2(`RAM_DEPTH)

module inferred_ram_w_clog2(
	clk,
	data_in,
	data_out,
	address
);
	parameter WORD_SIZE_LOCAL = `WORD_SIZE;
	parameter RAM_DEPTH_LOCAL = `RAM_DEPTH;
	parameter DEPTH_LOG2_LOCAL = $clog2(RAM_DEPTH_LOCAL);

	input clk;
	input [`DEPTH_LOG2-1:0] address;
	input [`WORD_SIZE-1:0] data_in;
	output [`WORD_SIZE-1:0] data_out;

	reg  [`WORD_SIZE-1:0] mregs [`RAM_DEPTH-1:0];  

	always @ (posedge clk )
	begin
		mregs[address] <= data_in;
		data_out <= mregs[address];
	end

endmodule

module top(
	clk,
	data_in,
	data_out1,
	data_out2,
	data_out3,
	data_out4,
	data_out5,
	address
);

	input clk;
	input [`DEPTH_LOG2-1:0] address;
	input [`WORD_SIZE-1:0] data_in;
	output [`WORD_SIZE-1:0] data_out1;
	output [`WORD_SIZE-1:0] data_out2;
	output [`WORD_SIZE-1:0] data_out3;
	output [`WORD_SIZE-1:0] data_out4;
	output [`WORD_SIZE-1:0] data_out5;

	parameter NEW_RAM_DEPTH = 2**3;

	// ram_depth = 2**5, depth_log2 = 5
	inferred_ram_w_clog2 inst_1 (clk, data_in, data_out1, address);

	// ram_depth = 2**3, depth_log2 = 3
	inferred_ram_w_clog2 #(.RAM_DEPTH_LOCAL(NEW_RAM_DEPTH)) inst_2 (clk, data_in, data_out2, address);

	// ram_depth = 2**4, depth_log2 = 4
	inferred_ram_w_clog2 #(.RAM_DEPTH_LOCAL(2**($clog2(16)))) inst_3 (clk, data_in, data_out3, address);

	assign data_out4 = $clog2(`RAM_DEPTH);

	always @ (posedge clk)
	begin
		data_out5 = $clog2(NEW_RAM_DEPTH);
	end

endmodule