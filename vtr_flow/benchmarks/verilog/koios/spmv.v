//////////////////////////////////////////////////////////////////////////////
// Matrix vector multiplication design with sparsity
// This is a Sparse Matrix-Vector Multiplication accelerator design. 
// The mathematical formula is Y = M * X where M is input sparse matrix, 
// X is input dense vector and Y is output dense vector. We support 8-bit 
// precision. Vector X is loaded in on-chip banked memory before start of 
// the operation. The input to the accelerator are 3 values corresponding 
// to each matrix element: data, col, number of non zero values in a row. 
// There are 32 channels which can run in parallel. Each channel has a MAC 
// unit. Computation for each row of the matrix is assigned to a channel 
// dynamically based on the number of non zero values in that row. The final
// accumulated value is written in the output memory. The write address is 
// based on row number. Hence, write is out of order with respect to rows 
// in output vector.
// Multipliers in channels followed by accumulators, Banked Vector Buffer 
// (RAM and crossbar) to store vector elements. Arbiter to fetch data from 
// RAMs store matrix elements. Uses CISR encoding. Multiple FIFOs.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Author: Aatman Borda
//////////////////////////////////////////////////////////////////////////////

//`define SIMULATION
`define COL_ID_BITS 8
`define ROW_ID_BITS 8
`define MAT_VAL_BITS 8
`define VEC_VAL_BITS 8
`define MULT_BITS (`VEC_VAL_BITS + `MAT_VAL_BITS)
`define NUM_CHANNEL 32
`define NUM_CHANNEL_BITS $clog2(`NUM_CHANNEL)
`define LANE_NUM (3 * `NUM_CHANNEL)
// `define LANE_NUM_BITS $clog2(`LANE_NUM)
`define NUM_MAT_VALS 8864
`define NUM_COL_IDS `NUM_MAT_VALS
`define NUM_ROW_IDS `NUM_MAT_VALS
`define NUM_VEC_VALS 128

`define FIFO_DEPTH 8
`define MAX_COLS (1<<`COL_ID_BITS)

// MACROS for BVB
`define BYTES_PER_ADDR_PER_BRAM 1
`define NUM_BRAMS 1
`define VEC_VAL_BYTES (`VEC_VAL_BITS/8)
`define VEC_VAL_OFFSET $clog2(`VEC_VAL_BITS)
`define NUM_VEC_VALS_PER_ADDR_PER_BRAM (`BYTES_PER_ADDR_PER_BRAM/`VEC_VAL_BYTES)
`define NUM_VEC_VALS_PER_ADDR `NUM_VEC_VALS_PER_ADDR_PER_BRAM*`NUM_BRAMS
`define NUM_VEC_VALS_PER_ADDR_BITS $clog2(`NUM_VEC_VALS_PER_ADDR)
`define NUM_ADDR (`NUM_VEC_VALS/`NUM_VEC_VALS_PER_ADDR)
`define BVB_AWIDTH `COL_ID_BITS
`define COUNTER_BITS $clog2(`NUM_ADDR)
`define LOCAL_ID_BITS `NUM_VEC_VALS_PER_ADDR_BITS

module spmv(
	input clk,
	input rst,

	output [`MULT_BITS-1:0] dout_nc,

	output reg done_reg
);

	// Row ID fifo signal
	wire [(`ROW_ID_BITS*`NUM_CHANNEL)-1:0] row_id;
	wire [`NUM_CHANNEL-1:0] row_id_empty;
	wire [`NUM_CHANNEL-1:0] row_id_rd_en;

	// column id fifo signals
	wire [(`COL_ID_BITS*`NUM_CHANNEL-1):0] col_id;
	wire [`NUM_CHANNEL-1:0] col_id_empty;
	wire [`NUM_CHANNEL-1:0] col_id_rd_en; 

	// matrix elements fifo signals
	wire [(`MAT_VAL_BITS*`NUM_CHANNEL)-1:0] mat_val;
	wire [`NUM_CHANNEL-1:0] mat_val_empty;
	wire [`NUM_CHANNEL-1:0] mat_val_rd_en;

	// The above the FIFOs are called fetcher FIFOs as they are a part of fetcher module and are filled by fetcher.

	// Fetcher module signals. There are just the above three put together.
	wire [((`ROW_ID_BITS+`COL_ID_BITS+`MAT_VAL_BITS)*`NUM_CHANNEL)-1:0] fetcher_out;
	wire [3*`NUM_CHANNEL-1:0] fetcher_empty;
	wire [3*`NUM_CHANNEL-1:0] fetcher_rd_en;

	// Vector values fifo signals
	wire [`VEC_VAL_BITS*`NUM_CHANNEL-1:0] vec_val;
	wire [`NUM_CHANNEL-1:0] vec_val_empty;
	wire [`NUM_CHANNEL-1:0] vec_val_rd_en;

	// 1 if at least 1 of the fetcher FIFOs is empty
	reg all_empty;

	// Signal to start the engine
	reg start;

	// Output data
	wire [(`MULT_BITS*`NUM_CHANNEL)-1:0] data_out;
	// output fifo empty signals
	wire [`NUM_CHANNEL-1:0] data_out_empty;
	// Address to store output data
	wire [(`ROW_ID_BITS*`NUM_CHANNEL)-1:0] addr_out;
	// output fifo read enable signal
	wire [`NUM_CHANNEL-1:0] out_rd_en;
	// reg [`NUM_CHANNEL-1:0] out_rd_en_reg;
	wire [`NUM_CHANNEL-1:0] out_rd_en_shifted;
	// Memory to write the output data

	wire [`MULT_BITS-1:0] data_out_shifted;
	wire [`ROW_ID_BITS-1:0] addr_out_shifted;
	wire [`MULT_BITS-1:0] wr_data;
	wire [`ROW_ID_BITS-1:0] wr_addr;
	reg wr_en;

	// Signals to indicate that the computation is complete
	wire [`NUM_CHANNEL-1:0] done;
	reg done_all, last;

	wire fetcher_done;

	always@(posedge clk or posedge rst) begin
		if (rst) begin
			all_empty <= 1;
			start <= 0;
		end
		else begin
			all_empty <= (|fetcher_empty) & (~start);
			if (!all_empty) begin
				start <= 1;
			end
		end
	end

	reg [`NUM_CHANNEL_BITS-1:0] counter;
	reg [`NUM_CHANNEL_BITS-1:0] counter_delay;
	reg [`NUM_CHANNEL_BITS-1:0] counter_store;

	assign out_rd_en = (({`NUM_CHANNEL{1'b0}})|(1<<counter)) & ~data_out_empty;
	assign out_rd_en_shifted = out_rd_en >> counter;

	assign data_out_shifted = data_out >> (counter_delay*`MULT_BITS);
	assign addr_out_shifted = addr_out >> (counter_delay*`ROW_ID_BITS);

	assign wr_data = data_out_shifted;
	assign wr_addr = addr_out_shifted;

	always @ (posedge clk or posedge rst) begin
		if(rst) begin
			counter <= 0;
			counter_delay <= 0;
			last <= 0;
		end
		else if(start) begin
			done_all <= &done;
			counter <= counter + 1;
			counter_delay <= counter;
			if(out_rd_en_shifted[0]) begin
				wr_en <= 1;
			end
			else begin
				wr_en <= 0;
			end
			if(done_all & !last) begin
				counter_store <= counter_delay;
				last <= 1;
			end
			else if (last) begin
				done_reg <= (counter_store==counter_delay);
			end
		end
	end

	spram #(
		.AWIDTH(`ROW_ID_BITS),
		.NUM_WORDS(`NUM_VEC_VALS),
		.DWIDTH(`MULT_BITS)
	) write_mem (
		.clk(clk),
		.address(wr_addr),
		.wren(wr_en),
		.din(wr_data),
		.dout(dout_nc)
	);

	// Assign FIFOs' read enables to the respective wires of fetcher read enable.
	assign fetcher_rd_en[3*`NUM_CHANNEL-1: 2*`NUM_CHANNEL] = row_id_rd_en;
	assign fetcher_rd_en[2*`NUM_CHANNEL-1: 1*`NUM_CHANNEL] = col_id_rd_en;
	assign fetcher_rd_en[1*`NUM_CHANNEL-1: 0] = mat_val_rd_en;

	fetcher fetcher (
		.clk(clk),
		.rst(rst),

		.mat_val_rd_en(fetcher_rd_en[1*`NUM_CHANNEL-1: 0]),
		.col_id_rd_en(fetcher_rd_en[2*`NUM_CHANNEL-1: 1*`NUM_CHANNEL]),
		.row_id_rd_en(fetcher_rd_en[3*`NUM_CHANNEL-1: 2*`NUM_CHANNEL]),

		.mat_val_out(fetcher_out[(1*`MAT_VAL_BITS*`NUM_CHANNEL)-1: 0]),
		.col_id_out(fetcher_out[(2*`COL_ID_BITS*`NUM_CHANNEL)-1: 1*`COL_ID_BITS*`NUM_CHANNEL]),
		.row_id_out(fetcher_out[(3*`ROW_ID_BITS*`NUM_CHANNEL)-1: 2*`ROW_ID_BITS*`NUM_CHANNEL]),

		.mat_val_empty(fetcher_empty[1*`NUM_CHANNEL-1: 0]),
		.col_id_empty(fetcher_empty[2*`NUM_CHANNEL-1: 1*`NUM_CHANNEL]),
		.row_id_empty(fetcher_empty[3*`NUM_CHANNEL-1: 2*`NUM_CHANNEL]),

		.done(fetcher_done)
	);

	assign row_id = fetcher_out[3*`ROW_ID_BITS*`NUM_CHANNEL-1: 2*`ROW_ID_BITS*`NUM_CHANNEL];
	assign row_id_empty = fetcher_empty[3*`NUM_CHANNEL-1: 2*`NUM_CHANNEL];

	assign col_id = fetcher_out[2*`COL_ID_BITS*`NUM_CHANNEL-1: 1*`COL_ID_BITS*`NUM_CHANNEL];
	assign col_id_empty = fetcher_empty[2*`NUM_CHANNEL-1: 1*`NUM_CHANNEL];

	assign mat_val = fetcher_out[1*`MAT_VAL_BITS*`NUM_CHANNEL-1: 0];
	assign mat_val_empty = fetcher_empty[1*`NUM_CHANNEL-1: 0];


	bvb bvb (
		.clk(clk),
		.rst(rst),
		.start(start),
		.id(col_id),
		.id_empty(col_id_empty),
		.id_rd_en(col_id_rd_en),
		.val(vec_val),
		.val_empty(vec_val_empty),
		.val_rd_en(vec_val_rd_en)
	);

	Big_Channel Big_Channel_ (
		.clk(clk), 
		.rst(rst),
		.start(start), 
		.fetcher_done(fetcher_done),
		.mat_val(mat_val), 
		.mat_val_empty(mat_val_empty), 
		.mat_val_rd_en(mat_val_rd_en), 
		.vec_val(vec_val), 
		.vec_val_empty(vec_val_empty), 
		.vec_val_rd_en(vec_val_rd_en), 
		.row_id(row_id), 
		.row_id_empty(row_id_empty), 
		.row_id_rd_en(row_id_rd_en), 
		.data_out(data_out),
		.data_out_empty(data_out_empty),
		.addr_out(addr_out),
		.out_rd_en(out_rd_en),
		.done(done)
	);
endmodule



/* Fetcher Module */
module fetcher(
	input clk,
	input rst,

	input  [`NUM_CHANNEL-1:0] mat_val_rd_en,
	input  [`NUM_CHANNEL-1:0] col_id_rd_en,
	input  [`NUM_CHANNEL-1:0] row_id_rd_en,

	output [(`MAT_VAL_BITS*`NUM_CHANNEL)-1:0] mat_val_out,
	output [(`COL_ID_BITS*`NUM_CHANNEL)-1:0] col_id_out,
	output [(`ROW_ID_BITS*`NUM_CHANNEL)-1:0] row_id_out,

	output [`NUM_CHANNEL-1:0] mat_val_empty,
	output [`NUM_CHANNEL-1:0] col_id_empty,
	output [`NUM_CHANNEL-1:0] row_id_empty,

	output done
	);

`ifdef SIMULATION
	parameter MAT_VAL_FILE = "/home/aatman/Desktop/SpMV/src/coe/mat_val.txt";
	parameter COL_ID_FILE = "/home/aatman/Desktop/SpMV/src/coe/col_id.txt";
	parameter ROW_ID_FILE = "/home/aatman/Desktop/SpMV/src/coe/row_id.txt";
`endif

	// parameter FIFO_DEPTH = 8;
	parameter FIFO_DEPTH_BITS = $clog2(`FIFO_DEPTH);

	// parameter ROW_LEN_ROM_DWIDTH = `ROW_LEN_BITS * `NUM_CHANNEL;
	// parameter ROW_LEN_ROM_NUM_ADDR = `NUM_ROW_LENS/`NUM_CHANNEL;
	parameter ROW_ID_ROM_DWIDTH = `ROW_ID_BITS;
	parameter ROW_ID_ROM_NUM_ADDR = `NUM_ROW_IDS;
	parameter ROW_ID_AWIDTH = $clog2(`NUM_ROW_IDS);

	// parameter MAT_VAL_ROM_DWIDTH = `MAT_VAL_BITS * `NUM_CHANNEL;
	// parameter MAT_VAL_ROM_NUM_ADDR = `NUM_MAT_VALS/`NUM_CHANNEL;
	parameter MAT_VAL_ROM_DWIDTH = `MAT_VAL_BITS;
	parameter MAT_VAL_ROM_NUM_ADDR = `NUM_MAT_VALS;
	parameter MAT_VAL_AWIDTH = $clog2(`NUM_MAT_VALS);

	// parameter COL_ID_ROM_DWIDTH = `COL_ID_BITS * `NUM_CHANNEL;
	// parameter COL_ID_ROM_NUM_ADDR = `NUM_COL_IDS/`NUM_CHANNEL;
	parameter COL_ID_ROM_DWIDTH = `COL_ID_BITS;
	parameter COL_ID_ROM_NUM_ADDR = `NUM_COL_IDS;
	parameter COL_ID_AWIDTH = $clog2(`NUM_COL_IDS);

	parameter NUM_CHANNEL_BITS = $clog2(`NUM_CHANNEL);

	reg [MAT_VAL_AWIDTH-1:0] mat_val_addr;
	reg [COL_ID_AWIDTH-1:0] col_id_addr;
	reg [ROW_ID_AWIDTH-1:0] row_id_addr;

	wire [MAT_VAL_ROM_DWIDTH-1:0] mat_val_dout;
	wire [COL_ID_ROM_DWIDTH-1:0] col_id_dout;
	wire [ROW_ID_ROM_DWIDTH-1:0] row_id_dout;

	reg [NUM_CHANNEL_BITS-1:0] mat_val_lane;
	reg [NUM_CHANNEL_BITS-1:0] col_id_lane;
	reg [NUM_CHANNEL_BITS-1:0] row_id_lane;
	 
	reg [`NUM_CHANNEL-1:0] mat_val_wr_en;
	reg [`NUM_CHANNEL-1:0] col_id_wr_en;
	reg [`NUM_CHANNEL-1:0] row_id_wr_en;

	wire [`NUM_CHANNEL-1:0] mat_val_full;
	wire [`NUM_CHANNEL-1:0] col_id_full;
	wire [`NUM_CHANNEL-1:0] row_id_full;

	wire [`NUM_CHANNEL-1:0] mat_val_full_shifted;
	wire [`NUM_CHANNEL-1:0] col_id_full_shifted;
	wire [`NUM_CHANNEL-1:0] row_id_full_shifted;

	// wire done;
	reg [MAT_VAL_AWIDTH:0] counter;

	spram #(
	`ifdef SIMULATION
		.INIT(MAT_VAL_FILE),
	`endif
		.AWIDTH(MAT_VAL_AWIDTH),
		.NUM_WORDS(MAT_VAL_ROM_NUM_ADDR),
		.DWIDTH(MAT_VAL_ROM_DWIDTH)
	) mat_val_rom (
		.clk(clk),
		.address(mat_val_addr),
		.wren(0),
		.din(0),
		.dout(mat_val_dout)
	);

	spram #(
	`ifdef SIMULATION
		.INIT(COL_ID_FILE),
	`endif
		.AWIDTH(COL_ID_AWIDTH),
		.NUM_WORDS(COL_ID_ROM_NUM_ADDR),
		.DWIDTH(COL_ID_ROM_DWIDTH)
	) col_id_rom (
		.clk(clk),
		.address(col_id_addr),
		.wren(0),
		.din(0),
		.dout(col_id_dout)
	);

	spram #(
	`ifdef SIMULATION
		.INIT(ROW_ID_FILE),
	`endif
		.AWIDTH(ROW_ID_AWIDTH),
		.NUM_WORDS(ROW_ID_ROM_NUM_ADDR),
		.DWIDTH(ROW_ID_ROM_DWIDTH)
	) row_id_rom (
		.clk(clk),
		.address(row_id_addr),
		.wren(0),
		.din(0),
		.dout(row_id_dout)
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_0 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[0]), // input wr_en 
		.re(mat_val_rd_en[0]), // input rd_en 
		.dout(mat_val_out[((0+1)*`MAT_VAL_BITS)-1:0*`MAT_VAL_BITS]), 
		.full(mat_val_full[0]), // output full 
		.empty(mat_val_empty[0]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_0 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[0]), // input wr_en 
		.re(col_id_rd_en[0]), // input rd_en 
		.dout(col_id_out[((0+1)*`COL_ID_BITS)-1:0*`COL_ID_BITS]), 
		.full(col_id_full[0]), // output full 
		.empty(col_id_empty[0]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_0 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[0]), // input wr_en 
		.re(row_id_rd_en[0]), // input rd_en 
		.dout(row_id_out[((0+1)*`ROW_ID_BITS)-1:0*`ROW_ID_BITS]), 
		.full(row_id_full[0]), // output full 
		.empty(row_id_empty[0]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_1 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[1]), // input wr_en 
		.re(mat_val_rd_en[1]), // input rd_en 
		.dout(mat_val_out[((1+1)*`MAT_VAL_BITS)-1:1*`MAT_VAL_BITS]), 
		.full(mat_val_full[1]), // output full 
		.empty(mat_val_empty[1]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_1 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[1]), // input wr_en 
		.re(col_id_rd_en[1]), // input rd_en 
		.dout(col_id_out[((1+1)*`COL_ID_BITS)-1:1*`COL_ID_BITS]), 
		.full(col_id_full[1]), // output full 
		.empty(col_id_empty[1]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_1 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[1]), // input wr_en 
		.re(row_id_rd_en[1]), // input rd_en 
		.dout(row_id_out[((1+1)*`ROW_ID_BITS)-1:1*`ROW_ID_BITS]), 
		.full(row_id_full[1]), // output full 
		.empty(row_id_empty[1]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_2 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[2]), // input wr_en 
		.re(mat_val_rd_en[2]), // input rd_en 
		.dout(mat_val_out[((2+1)*`MAT_VAL_BITS)-1:2*`MAT_VAL_BITS]), 
		.full(mat_val_full[2]), // output full 
		.empty(mat_val_empty[2]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_2 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[2]), // input wr_en 
		.re(col_id_rd_en[2]), // input rd_en 
		.dout(col_id_out[((2+1)*`COL_ID_BITS)-1:2*`COL_ID_BITS]), 
		.full(col_id_full[2]), // output full 
		.empty(col_id_empty[2]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_2 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[2]), // input wr_en 
		.re(row_id_rd_en[2]), // input rd_en 
		.dout(row_id_out[((2+1)*`ROW_ID_BITS)-1:2*`ROW_ID_BITS]), 
		.full(row_id_full[2]), // output full 
		.empty(row_id_empty[2]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_3 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[3]), // input wr_en 
		.re(mat_val_rd_en[3]), // input rd_en 
		.dout(mat_val_out[((3+1)*`MAT_VAL_BITS)-1:3*`MAT_VAL_BITS]), 
		.full(mat_val_full[3]), // output full 
		.empty(mat_val_empty[3]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_3 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[3]), // input wr_en 
		.re(col_id_rd_en[3]), // input rd_en 
		.dout(col_id_out[((3+1)*`COL_ID_BITS)-1:3*`COL_ID_BITS]), 
		.full(col_id_full[3]), // output full 
		.empty(col_id_empty[3]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_3 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[3]), // input wr_en 
		.re(row_id_rd_en[3]), // input rd_en 
		.dout(row_id_out[((3+1)*`ROW_ID_BITS)-1:3*`ROW_ID_BITS]), 
		.full(row_id_full[3]), // output full 
		.empty(row_id_empty[3]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_4 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[4]), // input wr_en 
		.re(mat_val_rd_en[4]), // input rd_en 
		.dout(mat_val_out[((4+1)*`MAT_VAL_BITS)-1:4*`MAT_VAL_BITS]), 
		.full(mat_val_full[4]), // output full 
		.empty(mat_val_empty[4]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_4 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[4]), // input wr_en 
		.re(col_id_rd_en[4]), // input rd_en 
		.dout(col_id_out[((4+1)*`COL_ID_BITS)-1:4*`COL_ID_BITS]), 
		.full(col_id_full[4]), // output full 
		.empty(col_id_empty[4]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_4 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[4]), // input wr_en 
		.re(row_id_rd_en[4]), // input rd_en 
		.dout(row_id_out[((4+1)*`ROW_ID_BITS)-1:4*`ROW_ID_BITS]), 
		.full(row_id_full[4]), // output full 
		.empty(row_id_empty[4]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_5 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[5]), // input wr_en 
		.re(mat_val_rd_en[5]), // input rd_en 
		.dout(mat_val_out[((5+1)*`MAT_VAL_BITS)-1:5*`MAT_VAL_BITS]), 
		.full(mat_val_full[5]), // output full 
		.empty(mat_val_empty[5]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_5 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[5]), // input wr_en 
		.re(col_id_rd_en[5]), // input rd_en 
		.dout(col_id_out[((5+1)*`COL_ID_BITS)-1:5*`COL_ID_BITS]), 
		.full(col_id_full[5]), // output full 
		.empty(col_id_empty[5]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_5 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[5]), // input wr_en 
		.re(row_id_rd_en[5]), // input rd_en 
		.dout(row_id_out[((5+1)*`ROW_ID_BITS)-1:5*`ROW_ID_BITS]), 
		.full(row_id_full[5]), // output full 
		.empty(row_id_empty[5]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_6 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[6]), // input wr_en 
		.re(mat_val_rd_en[6]), // input rd_en 
		.dout(mat_val_out[((6+1)*`MAT_VAL_BITS)-1:6*`MAT_VAL_BITS]), 
		.full(mat_val_full[6]), // output full 
		.empty(mat_val_empty[6]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_6 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[6]), // input wr_en 
		.re(col_id_rd_en[6]), // input rd_en 
		.dout(col_id_out[((6+1)*`COL_ID_BITS)-1:6*`COL_ID_BITS]), 
		.full(col_id_full[6]), // output full 
		.empty(col_id_empty[6]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_6 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[6]), // input wr_en 
		.re(row_id_rd_en[6]), // input rd_en 
		.dout(row_id_out[((6+1)*`ROW_ID_BITS)-1:6*`ROW_ID_BITS]), 
		.full(row_id_full[6]), // output full 
		.empty(row_id_empty[6]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_7 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[7]), // input wr_en 
		.re(mat_val_rd_en[7]), // input rd_en 
		.dout(mat_val_out[((7+1)*`MAT_VAL_BITS)-1:7*`MAT_VAL_BITS]), 
		.full(mat_val_full[7]), // output full 
		.empty(mat_val_empty[7]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_7 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[7]), // input wr_en 
		.re(col_id_rd_en[7]), // input rd_en 
		.dout(col_id_out[((7+1)*`COL_ID_BITS)-1:7*`COL_ID_BITS]), 
		.full(col_id_full[7]), // output full 
		.empty(col_id_empty[7]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_7 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[7]), // input wr_en 
		.re(row_id_rd_en[7]), // input rd_en 
		.dout(row_id_out[((7+1)*`ROW_ID_BITS)-1:7*`ROW_ID_BITS]), 
		.full(row_id_full[7]), // output full 
		.empty(row_id_empty[7]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_8 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[8]), // input wr_en 
		.re(mat_val_rd_en[8]), // input rd_en 
		.dout(mat_val_out[((8+1)*`MAT_VAL_BITS)-1:8*`MAT_VAL_BITS]), 
		.full(mat_val_full[8]), // output full 
		.empty(mat_val_empty[8]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_8 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[8]), // input wr_en 
		.re(col_id_rd_en[8]), // input rd_en 
		.dout(col_id_out[((8+1)*`COL_ID_BITS)-1:8*`COL_ID_BITS]), 
		.full(col_id_full[8]), // output full 
		.empty(col_id_empty[8]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_8 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[8]), // input wr_en 
		.re(row_id_rd_en[8]), // input rd_en 
		.dout(row_id_out[((8+1)*`ROW_ID_BITS)-1:8*`ROW_ID_BITS]), 
		.full(row_id_full[8]), // output full 
		.empty(row_id_empty[8]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_9 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[9]), // input wr_en 
		.re(mat_val_rd_en[9]), // input rd_en 
		.dout(mat_val_out[((9+1)*`MAT_VAL_BITS)-1:9*`MAT_VAL_BITS]), 
		.full(mat_val_full[9]), // output full 
		.empty(mat_val_empty[9]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_9 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[9]), // input wr_en 
		.re(col_id_rd_en[9]), // input rd_en 
		.dout(col_id_out[((9+1)*`COL_ID_BITS)-1:9*`COL_ID_BITS]), 
		.full(col_id_full[9]), // output full 
		.empty(col_id_empty[9]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_9 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[9]), // input wr_en 
		.re(row_id_rd_en[9]), // input rd_en 
		.dout(row_id_out[((9+1)*`ROW_ID_BITS)-1:9*`ROW_ID_BITS]), 
		.full(row_id_full[9]), // output full 
		.empty(row_id_empty[9]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_10 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[10]), // input wr_en 
		.re(mat_val_rd_en[10]), // input rd_en 
		.dout(mat_val_out[((10+1)*`MAT_VAL_BITS)-1:10*`MAT_VAL_BITS]), 
		.full(mat_val_full[10]), // output full 
		.empty(mat_val_empty[10]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_10 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[10]), // input wr_en 
		.re(col_id_rd_en[10]), // input rd_en 
		.dout(col_id_out[((10+1)*`COL_ID_BITS)-1:10*`COL_ID_BITS]), 
		.full(col_id_full[10]), // output full 
		.empty(col_id_empty[10]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_10 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[10]), // input wr_en 
		.re(row_id_rd_en[10]), // input rd_en 
		.dout(row_id_out[((10+1)*`ROW_ID_BITS)-1:10*`ROW_ID_BITS]), 
		.full(row_id_full[10]), // output full 
		.empty(row_id_empty[10]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_11 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[11]), // input wr_en 
		.re(mat_val_rd_en[11]), // input rd_en 
		.dout(mat_val_out[((11+1)*`MAT_VAL_BITS)-1:11*`MAT_VAL_BITS]), 
		.full(mat_val_full[11]), // output full 
		.empty(mat_val_empty[11]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_11 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[11]), // input wr_en 
		.re(col_id_rd_en[11]), // input rd_en 
		.dout(col_id_out[((11+1)*`COL_ID_BITS)-1:11*`COL_ID_BITS]), 
		.full(col_id_full[11]), // output full 
		.empty(col_id_empty[11]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_11 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[11]), // input wr_en 
		.re(row_id_rd_en[11]), // input rd_en 
		.dout(row_id_out[((11+1)*`ROW_ID_BITS)-1:11*`ROW_ID_BITS]), 
		.full(row_id_full[11]), // output full 
		.empty(row_id_empty[11]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_12 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[12]), // input wr_en 
		.re(mat_val_rd_en[12]), // input rd_en 
		.dout(mat_val_out[((12+1)*`MAT_VAL_BITS)-1:12*`MAT_VAL_BITS]), 
		.full(mat_val_full[12]), // output full 
		.empty(mat_val_empty[12]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_12 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[12]), // input wr_en 
		.re(col_id_rd_en[12]), // input rd_en 
		.dout(col_id_out[((12+1)*`COL_ID_BITS)-1:12*`COL_ID_BITS]), 
		.full(col_id_full[12]), // output full 
		.empty(col_id_empty[12]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_12 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[12]), // input wr_en 
		.re(row_id_rd_en[12]), // input rd_en 
		.dout(row_id_out[((12+1)*`ROW_ID_BITS)-1:12*`ROW_ID_BITS]), 
		.full(row_id_full[12]), // output full 
		.empty(row_id_empty[12]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_13 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[13]), // input wr_en 
		.re(mat_val_rd_en[13]), // input rd_en 
		.dout(mat_val_out[((13+1)*`MAT_VAL_BITS)-1:13*`MAT_VAL_BITS]), 
		.full(mat_val_full[13]), // output full 
		.empty(mat_val_empty[13]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_13 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[13]), // input wr_en 
		.re(col_id_rd_en[13]), // input rd_en 
		.dout(col_id_out[((13+1)*`COL_ID_BITS)-1:13*`COL_ID_BITS]), 
		.full(col_id_full[13]), // output full 
		.empty(col_id_empty[13]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_13 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[13]), // input wr_en 
		.re(row_id_rd_en[13]), // input rd_en 
		.dout(row_id_out[((13+1)*`ROW_ID_BITS)-1:13*`ROW_ID_BITS]), 
		.full(row_id_full[13]), // output full 
		.empty(row_id_empty[13]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_14 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[14]), // input wr_en 
		.re(mat_val_rd_en[14]), // input rd_en 
		.dout(mat_val_out[((14+1)*`MAT_VAL_BITS)-1:14*`MAT_VAL_BITS]), 
		.full(mat_val_full[14]), // output full 
		.empty(mat_val_empty[14]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_14 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[14]), // input wr_en 
		.re(col_id_rd_en[14]), // input rd_en 
		.dout(col_id_out[((14+1)*`COL_ID_BITS)-1:14*`COL_ID_BITS]), 
		.full(col_id_full[14]), // output full 
		.empty(col_id_empty[14]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_14 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[14]), // input wr_en 
		.re(row_id_rd_en[14]), // input rd_en 
		.dout(row_id_out[((14+1)*`ROW_ID_BITS)-1:14*`ROW_ID_BITS]), 
		.full(row_id_full[14]), // output full 
		.empty(row_id_empty[14]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_15 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[15]), // input wr_en 
		.re(mat_val_rd_en[15]), // input rd_en 
		.dout(mat_val_out[((15+1)*`MAT_VAL_BITS)-1:15*`MAT_VAL_BITS]), 
		.full(mat_val_full[15]), // output full 
		.empty(mat_val_empty[15]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_15 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[15]), // input wr_en 
		.re(col_id_rd_en[15]), // input rd_en 
		.dout(col_id_out[((15+1)*`COL_ID_BITS)-1:15*`COL_ID_BITS]), 
		.full(col_id_full[15]), // output full 
		.empty(col_id_empty[15]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_15 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[15]), // input wr_en 
		.re(row_id_rd_en[15]), // input rd_en 
		.dout(row_id_out[((15+1)*`ROW_ID_BITS)-1:15*`ROW_ID_BITS]), 
		.full(row_id_full[15]), // output full 
		.empty(row_id_empty[15]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_16 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[16]), // input wr_en 
		.re(mat_val_rd_en[16]), // input rd_en 
		.dout(mat_val_out[((16+1)*`MAT_VAL_BITS)-1:16*`MAT_VAL_BITS]), 
		.full(mat_val_full[16]), // output full 
		.empty(mat_val_empty[16]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_16 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[16]), // input wr_en 
		.re(col_id_rd_en[16]), // input rd_en 
		.dout(col_id_out[((16+1)*`COL_ID_BITS)-1:16*`COL_ID_BITS]), 
		.full(col_id_full[16]), // output full 
		.empty(col_id_empty[16]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_16 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[16]), // input wr_en 
		.re(row_id_rd_en[16]), // input rd_en 
		.dout(row_id_out[((16+1)*`ROW_ID_BITS)-1:16*`ROW_ID_BITS]), 
		.full(row_id_full[16]), // output full 
		.empty(row_id_empty[16]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_17 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[17]), // input wr_en 
		.re(mat_val_rd_en[17]), // input rd_en 
		.dout(mat_val_out[((17+1)*`MAT_VAL_BITS)-1:17*`MAT_VAL_BITS]), 
		.full(mat_val_full[17]), // output full 
		.empty(mat_val_empty[17]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_17 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[17]), // input wr_en 
		.re(col_id_rd_en[17]), // input rd_en 
		.dout(col_id_out[((17+1)*`COL_ID_BITS)-1:17*`COL_ID_BITS]), 
		.full(col_id_full[17]), // output full 
		.empty(col_id_empty[17]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_17 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[17]), // input wr_en 
		.re(row_id_rd_en[17]), // input rd_en 
		.dout(row_id_out[((17+1)*`ROW_ID_BITS)-1:17*`ROW_ID_BITS]), 
		.full(row_id_full[17]), // output full 
		.empty(row_id_empty[17]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_18 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[18]), // input wr_en 
		.re(mat_val_rd_en[18]), // input rd_en 
		.dout(mat_val_out[((18+1)*`MAT_VAL_BITS)-1:18*`MAT_VAL_BITS]), 
		.full(mat_val_full[18]), // output full 
		.empty(mat_val_empty[18]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_18 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[18]), // input wr_en 
		.re(col_id_rd_en[18]), // input rd_en 
		.dout(col_id_out[((18+1)*`COL_ID_BITS)-1:18*`COL_ID_BITS]), 
		.full(col_id_full[18]), // output full 
		.empty(col_id_empty[18]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_18 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[18]), // input wr_en 
		.re(row_id_rd_en[18]), // input rd_en 
		.dout(row_id_out[((18+1)*`ROW_ID_BITS)-1:18*`ROW_ID_BITS]), 
		.full(row_id_full[18]), // output full 
		.empty(row_id_empty[18]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_19 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[19]), // input wr_en 
		.re(mat_val_rd_en[19]), // input rd_en 
		.dout(mat_val_out[((19+1)*`MAT_VAL_BITS)-1:19*`MAT_VAL_BITS]), 
		.full(mat_val_full[19]), // output full 
		.empty(mat_val_empty[19]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_19 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[19]), // input wr_en 
		.re(col_id_rd_en[19]), // input rd_en 
		.dout(col_id_out[((19+1)*`COL_ID_BITS)-1:19*`COL_ID_BITS]), 
		.full(col_id_full[19]), // output full 
		.empty(col_id_empty[19]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_19 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[19]), // input wr_en 
		.re(row_id_rd_en[19]), // input rd_en 
		.dout(row_id_out[((19+1)*`ROW_ID_BITS)-1:19*`ROW_ID_BITS]), 
		.full(row_id_full[19]), // output full 
		.empty(row_id_empty[19]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_20 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[20]), // input wr_en 
		.re(mat_val_rd_en[20]), // input rd_en 
		.dout(mat_val_out[((20+1)*`MAT_VAL_BITS)-1:20*`MAT_VAL_BITS]), 
		.full(mat_val_full[20]), // output full 
		.empty(mat_val_empty[20]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_20 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[20]), // input wr_en 
		.re(col_id_rd_en[20]), // input rd_en 
		.dout(col_id_out[((20+1)*`COL_ID_BITS)-1:20*`COL_ID_BITS]), 
		.full(col_id_full[20]), // output full 
		.empty(col_id_empty[20]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_20 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[20]), // input wr_en 
		.re(row_id_rd_en[20]), // input rd_en 
		.dout(row_id_out[((20+1)*`ROW_ID_BITS)-1:20*`ROW_ID_BITS]), 
		.full(row_id_full[20]), // output full 
		.empty(row_id_empty[20]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_21 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[21]), // input wr_en 
		.re(mat_val_rd_en[21]), // input rd_en 
		.dout(mat_val_out[((21+1)*`MAT_VAL_BITS)-1:21*`MAT_VAL_BITS]), 
		.full(mat_val_full[21]), // output full 
		.empty(mat_val_empty[21]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_21 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[21]), // input wr_en 
		.re(col_id_rd_en[21]), // input rd_en 
		.dout(col_id_out[((21+1)*`COL_ID_BITS)-1:21*`COL_ID_BITS]), 
		.full(col_id_full[21]), // output full 
		.empty(col_id_empty[21]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_21 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[21]), // input wr_en 
		.re(row_id_rd_en[21]), // input rd_en 
		.dout(row_id_out[((21+1)*`ROW_ID_BITS)-1:21*`ROW_ID_BITS]), 
		.full(row_id_full[21]), // output full 
		.empty(row_id_empty[21]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_22 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[22]), // input wr_en 
		.re(mat_val_rd_en[22]), // input rd_en 
		.dout(mat_val_out[((22+1)*`MAT_VAL_BITS)-1:22*`MAT_VAL_BITS]), 
		.full(mat_val_full[22]), // output full 
		.empty(mat_val_empty[22]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_22 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[22]), // input wr_en 
		.re(col_id_rd_en[22]), // input rd_en 
		.dout(col_id_out[((22+1)*`COL_ID_BITS)-1:22*`COL_ID_BITS]), 
		.full(col_id_full[22]), // output full 
		.empty(col_id_empty[22]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_22 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[22]), // input wr_en 
		.re(row_id_rd_en[22]), // input rd_en 
		.dout(row_id_out[((22+1)*`ROW_ID_BITS)-1:22*`ROW_ID_BITS]), 
		.full(row_id_full[22]), // output full 
		.empty(row_id_empty[22]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_23 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[23]), // input wr_en 
		.re(mat_val_rd_en[23]), // input rd_en 
		.dout(mat_val_out[((23+1)*`MAT_VAL_BITS)-1:23*`MAT_VAL_BITS]), 
		.full(mat_val_full[23]), // output full 
		.empty(mat_val_empty[23]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_23 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[23]), // input wr_en 
		.re(col_id_rd_en[23]), // input rd_en 
		.dout(col_id_out[((23+1)*`COL_ID_BITS)-1:23*`COL_ID_BITS]), 
		.full(col_id_full[23]), // output full 
		.empty(col_id_empty[23]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_23 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[23]), // input wr_en 
		.re(row_id_rd_en[23]), // input rd_en 
		.dout(row_id_out[((23+1)*`ROW_ID_BITS)-1:23*`ROW_ID_BITS]), 
		.full(row_id_full[23]), // output full 
		.empty(row_id_empty[23]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_24 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[24]), // input wr_en 
		.re(mat_val_rd_en[24]), // input rd_en 
		.dout(mat_val_out[((24+1)*`MAT_VAL_BITS)-1:24*`MAT_VAL_BITS]), 
		.full(mat_val_full[24]), // output full 
		.empty(mat_val_empty[24]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_24 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[24]), // input wr_en 
		.re(col_id_rd_en[24]), // input rd_en 
		.dout(col_id_out[((24+1)*`COL_ID_BITS)-1:24*`COL_ID_BITS]), 
		.full(col_id_full[24]), // output full 
		.empty(col_id_empty[24]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_24 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[24]), // input wr_en 
		.re(row_id_rd_en[24]), // input rd_en 
		.dout(row_id_out[((24+1)*`ROW_ID_BITS)-1:24*`ROW_ID_BITS]), 
		.full(row_id_full[24]), // output full 
		.empty(row_id_empty[24]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_25 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[25]), // input wr_en 
		.re(mat_val_rd_en[25]), // input rd_en 
		.dout(mat_val_out[((25+1)*`MAT_VAL_BITS)-1:25*`MAT_VAL_BITS]), 
		.full(mat_val_full[25]), // output full 
		.empty(mat_val_empty[25]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_25 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[25]), // input wr_en 
		.re(col_id_rd_en[25]), // input rd_en 
		.dout(col_id_out[((25+1)*`COL_ID_BITS)-1:25*`COL_ID_BITS]), 
		.full(col_id_full[25]), // output full 
		.empty(col_id_empty[25]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_25 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[25]), // input wr_en 
		.re(row_id_rd_en[25]), // input rd_en 
		.dout(row_id_out[((25+1)*`ROW_ID_BITS)-1:25*`ROW_ID_BITS]), 
		.full(row_id_full[25]), // output full 
		.empty(row_id_empty[25]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_26 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[26]), // input wr_en 
		.re(mat_val_rd_en[26]), // input rd_en 
		.dout(mat_val_out[((26+1)*`MAT_VAL_BITS)-1:26*`MAT_VAL_BITS]), 
		.full(mat_val_full[26]), // output full 
		.empty(mat_val_empty[26]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_26 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[26]), // input wr_en 
		.re(col_id_rd_en[26]), // input rd_en 
		.dout(col_id_out[((26+1)*`COL_ID_BITS)-1:26*`COL_ID_BITS]), 
		.full(col_id_full[26]), // output full 
		.empty(col_id_empty[26]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_26 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[26]), // input wr_en 
		.re(row_id_rd_en[26]), // input rd_en 
		.dout(row_id_out[((26+1)*`ROW_ID_BITS)-1:26*`ROW_ID_BITS]), 
		.full(row_id_full[26]), // output full 
		.empty(row_id_empty[26]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_27 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[27]), // input wr_en 
		.re(mat_val_rd_en[27]), // input rd_en 
		.dout(mat_val_out[((27+1)*`MAT_VAL_BITS)-1:27*`MAT_VAL_BITS]), 
		.full(mat_val_full[27]), // output full 
		.empty(mat_val_empty[27]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_27 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[27]), // input wr_en 
		.re(col_id_rd_en[27]), // input rd_en 
		.dout(col_id_out[((27+1)*`COL_ID_BITS)-1:27*`COL_ID_BITS]), 
		.full(col_id_full[27]), // output full 
		.empty(col_id_empty[27]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_27 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[27]), // input wr_en 
		.re(row_id_rd_en[27]), // input rd_en 
		.dout(row_id_out[((27+1)*`ROW_ID_BITS)-1:27*`ROW_ID_BITS]), 
		.full(row_id_full[27]), // output full 
		.empty(row_id_empty[27]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_28 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[28]), // input wr_en 
		.re(mat_val_rd_en[28]), // input rd_en 
		.dout(mat_val_out[((28+1)*`MAT_VAL_BITS)-1:28*`MAT_VAL_BITS]), 
		.full(mat_val_full[28]), // output full 
		.empty(mat_val_empty[28]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_28 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[28]), // input wr_en 
		.re(col_id_rd_en[28]), // input rd_en 
		.dout(col_id_out[((28+1)*`COL_ID_BITS)-1:28*`COL_ID_BITS]), 
		.full(col_id_full[28]), // output full 
		.empty(col_id_empty[28]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_28 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[28]), // input wr_en 
		.re(row_id_rd_en[28]), // input rd_en 
		.dout(row_id_out[((28+1)*`ROW_ID_BITS)-1:28*`ROW_ID_BITS]), 
		.full(row_id_full[28]), // output full 
		.empty(row_id_empty[28]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_29 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[29]), // input wr_en 
		.re(mat_val_rd_en[29]), // input rd_en 
		.dout(mat_val_out[((29+1)*`MAT_VAL_BITS)-1:29*`MAT_VAL_BITS]), 
		.full(mat_val_full[29]), // output full 
		.empty(mat_val_empty[29]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_29 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[29]), // input wr_en 
		.re(col_id_rd_en[29]), // input rd_en 
		.dout(col_id_out[((29+1)*`COL_ID_BITS)-1:29*`COL_ID_BITS]), 
		.full(col_id_full[29]), // output full 
		.empty(col_id_empty[29]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_29 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[29]), // input wr_en 
		.re(row_id_rd_en[29]), // input rd_en 
		.dout(row_id_out[((29+1)*`ROW_ID_BITS)-1:29*`ROW_ID_BITS]), 
		.full(row_id_full[29]), // output full 
		.empty(row_id_empty[29]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_30 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[30]), // input wr_en 
		.re(mat_val_rd_en[30]), // input rd_en 
		.dout(mat_val_out[((30+1)*`MAT_VAL_BITS)-1:30*`MAT_VAL_BITS]), 
		.full(mat_val_full[30]), // output full 
		.empty(mat_val_empty[30]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_30 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[30]), // input wr_en 
		.re(col_id_rd_en[30]), // input rd_en 
		.dout(col_id_out[((30+1)*`COL_ID_BITS)-1:30*`COL_ID_BITS]), 
		.full(col_id_full[30]), // output full 
		.empty(col_id_empty[30]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_30 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[30]), // input wr_en 
		.re(row_id_rd_en[30]), // input rd_en 
		.dout(row_id_out[((30+1)*`ROW_ID_BITS)-1:30*`ROW_ID_BITS]), 
		.full(row_id_full[30]), // output full 
		.empty(row_id_empty[30]) // output empty 
	);

	generic_fifo_sc_a #(
		.dw(MAT_VAL_ROM_DWIDTH),
		.aw(FIFO_DEPTH_BITS)
	) fifo_mat_val_31 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(mat_val_dout), 
		.we(mat_val_wr_en[31]), // input wr_en 
		.re(mat_val_rd_en[31]), // input rd_en 
		.dout(mat_val_out[((31+1)*`MAT_VAL_BITS)-1:31*`MAT_VAL_BITS]), 
		.full(mat_val_full[31]), // output full 
		.empty(mat_val_empty[31]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(COL_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_col_id_31 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(col_id_dout), 
		.we(col_id_wr_en[31]), // input wr_en 
		.re(col_id_rd_en[31]), // input rd_en 
		.dout(col_id_out[((31+1)*`COL_ID_BITS)-1:31*`COL_ID_BITS]), 
		.full(col_id_full[31]), // output full 
		.empty(col_id_empty[31]) // output empty 
	); 

	generic_fifo_sc_a #( 
		.dw(ROW_ID_ROM_DWIDTH), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_row_id_31 ( 
		.clk(clk), // input clk 
		.rst(rst), // input clk 
		.clr(0), 
		.din(row_id_dout), 
		.we(row_id_wr_en[31]), // input wr_en 
		.re(row_id_rd_en[31]), // input rd_en 
		.dout(row_id_out[((31+1)*`ROW_ID_BITS)-1:31*`ROW_ID_BITS]), 
		.full(row_id_full[31]), // output full 
		.empty(row_id_empty[31]) // output empty 
	);

	//TODO: done signal check for matrix values only. Should check for all three. Use 3 done signals and the final done is the and of all three.
	assign done = (counter >=`NUM_MAT_VALS);

	assign mat_val_full_shifted = mat_val_full>>mat_val_lane;
	assign col_id_full_shifted = col_id_full>>col_id_lane;
	assign row_id_full_shifted = row_id_full>>row_id_lane;

	always @ (posedge clk or posedge rst) begin
		if (rst) begin
			mat_val_addr <= 0;
			col_id_addr <= 0;
			row_id_addr <= 0;

			mat_val_wr_en <= 0;
			col_id_wr_en <= 0;
			row_id_wr_en <= 0;

			mat_val_lane <= 0;
			col_id_lane <= 0;
			row_id_lane <= 0;

			counter <= 0;
		end
		else begin
			if(!done) begin
				// Write matrix value into its FIFO
				if (!mat_val_full_shifted[0]) begin
					mat_val_wr_en <= (({`NUM_CHANNEL{1'b0}}) | (1<<mat_val_lane));
					mat_val_lane <= mat_val_lane + 1;
					mat_val_addr <= mat_val_addr + 1;
					counter <= counter + 1;
				end

				// Write column ID into its FIFO
				if (!col_id_full_shifted[0]) begin
					col_id_wr_en <= (({`NUM_CHANNEL{1'b0}}) | (1<<col_id_lane));
					col_id_lane <= col_id_lane + 1;
					col_id_addr <= col_id_addr + 1;
				end

				// Write row id into its FIFO
				if (!row_id_full_shifted[0]) begin
					row_id_wr_en <= (({`NUM_CHANNEL{1'b0}}) | (1<<row_id_lane));
					row_id_lane <= row_id_lane + 1;
					row_id_addr <= row_id_addr + 1;
				end
			end
			else begin
				mat_val_wr_en <= 0;
				col_id_wr_en <= 0;
				row_id_wr_en <= 0;
			end
		end
	end
endmodule

module bvb(
	input clk,
	input rst,
	input start,
	// input from column ID FIFO
	input  [(`NUM_CHANNEL*`COL_ID_BITS)-1:0] id,
	input  [`NUM_CHANNEL-1:0] id_empty,
	// input to the column ID FIFO
	output [`NUM_CHANNEL-1:0] id_rd_en,

	// vector value FIFO outputs
	output [(`NUM_CHANNEL*`VEC_VAL_BITS)-1:0] val,
	output [`NUM_CHANNEL-1:0]   val_empty,
	// input to vector value FIFO
	input  [`NUM_CHANNEL-1:0]   val_rd_en
);

	parameter FIFO_DEPTH_BITS = $clog2(`FIFO_DEPTH);

	reg [`NUM_CHANNEL-1:0] val_wr_en;
	wire [`NUM_CHANNEL-1:0] val_full;

	wire [`NUM_VEC_VALS_PER_ADDR*`VEC_VAL_BITS-1:0] ram_out;

	wire fifo_in_val;

	wire [`NUM_VEC_VALS_PER_ADDR*`VEC_VAL_BITS-1:0] din_nc;

	reg [`COL_ID_BITS-1:0] rd_addr;

	wire id_empty_shifted;
	wire val_full_shifted;

	wire id_rd_en_local;
	reg id_rd_en_local_reg;

	reg [`NUM_CHANNEL_BITS-1:0] counter;
	reg [`NUM_CHANNEL_BITS-1:0] counter_delay;

	spram #(
	`ifdef SIMULATION
		.INIT("/home/aatman/Desktop/SpMV/src/coe/vec_val.txt"),
	`endif
		.AWIDTH(`BVB_AWIDTH),
		.NUM_WORDS(`NUM_ADDR),
		.DWIDTH(`NUM_VEC_VALS_PER_ADDR*`VEC_VAL_BITS)
		) vector_ram (
			.clk(clk), 
			.wren(0),
			.address(rd_addr), 
			.din(din_nc), 
			.dout(ram_out)
		);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_0 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[0]), // input wr_en 
		.re(val_rd_en[0]), // input rd_en 
		.dout(val[(0+1)*`VEC_VAL_BITS-1:0*`VEC_VAL_BITS]), 
		.full(val_full[0]), // output full 
		.empty(val_empty[0]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_1 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[1]), // input wr_en 
		.re(val_rd_en[1]), // input rd_en 
		.dout(val[(1+1)*`VEC_VAL_BITS-1:1*`VEC_VAL_BITS]), 
		.full(val_full[1]), // output full 
		.empty(val_empty[1]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_2 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[2]), // input wr_en 
		.re(val_rd_en[2]), // input rd_en 
		.dout(val[(2+1)*`VEC_VAL_BITS-1:2*`VEC_VAL_BITS]), 
		.full(val_full[2]), // output full 
		.empty(val_empty[2]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_3 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[3]), // input wr_en 
		.re(val_rd_en[3]), // input rd_en 
		.dout(val[(3+1)*`VEC_VAL_BITS-1:3*`VEC_VAL_BITS]), 
		.full(val_full[3]), // output full 
		.empty(val_empty[3]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_4 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[4]), // input wr_en 
		.re(val_rd_en[4]), // input rd_en 
		.dout(val[(4+1)*`VEC_VAL_BITS-1:4*`VEC_VAL_BITS]), 
		.full(val_full[4]), // output full 
		.empty(val_empty[4]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_5 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[5]), // input wr_en 
		.re(val_rd_en[5]), // input rd_en 
		.dout(val[(5+1)*`VEC_VAL_BITS-1:5*`VEC_VAL_BITS]), 
		.full(val_full[5]), // output full 
		.empty(val_empty[5]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_6 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[6]), // input wr_en 
		.re(val_rd_en[6]), // input rd_en 
		.dout(val[(6+1)*`VEC_VAL_BITS-1:6*`VEC_VAL_BITS]), 
		.full(val_full[6]), // output full 
		.empty(val_empty[6]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_7 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[7]), // input wr_en 
		.re(val_rd_en[7]), // input rd_en 
		.dout(val[(7+1)*`VEC_VAL_BITS-1:7*`VEC_VAL_BITS]), 
		.full(val_full[7]), // output full 
		.empty(val_empty[7]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_8 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[8]), // input wr_en 
		.re(val_rd_en[8]), // input rd_en 
		.dout(val[(8+1)*`VEC_VAL_BITS-1:8*`VEC_VAL_BITS]), 
		.full(val_full[8]), // output full 
		.empty(val_empty[8]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_9 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[9]), // input wr_en 
		.re(val_rd_en[9]), // input rd_en 
		.dout(val[(9+1)*`VEC_VAL_BITS-1:9*`VEC_VAL_BITS]), 
		.full(val_full[9]), // output full 
		.empty(val_empty[9]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_10 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[10]), // input wr_en 
		.re(val_rd_en[10]), // input rd_en 
		.dout(val[(10+1)*`VEC_VAL_BITS-1:10*`VEC_VAL_BITS]), 
		.full(val_full[10]), // output full 
		.empty(val_empty[10]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_11 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[11]), // input wr_en 
		.re(val_rd_en[11]), // input rd_en 
		.dout(val[(11+1)*`VEC_VAL_BITS-1:11*`VEC_VAL_BITS]), 
		.full(val_full[11]), // output full 
		.empty(val_empty[11]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_12 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[12]), // input wr_en 
		.re(val_rd_en[12]), // input rd_en 
		.dout(val[(12+1)*`VEC_VAL_BITS-1:12*`VEC_VAL_BITS]), 
		.full(val_full[12]), // output full 
		.empty(val_empty[12]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_13 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[13]), // input wr_en 
		.re(val_rd_en[13]), // input rd_en 
		.dout(val[(13+1)*`VEC_VAL_BITS-1:13*`VEC_VAL_BITS]), 
		.full(val_full[13]), // output full 
		.empty(val_empty[13]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_14 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[14]), // input wr_en 
		.re(val_rd_en[14]), // input rd_en 
		.dout(val[(14+1)*`VEC_VAL_BITS-1:14*`VEC_VAL_BITS]), 
		.full(val_full[14]), // output full 
		.empty(val_empty[14]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_15 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[15]), // input wr_en 
		.re(val_rd_en[15]), // input rd_en 
		.dout(val[(15+1)*`VEC_VAL_BITS-1:15*`VEC_VAL_BITS]), 
		.full(val_full[15]), // output full 
		.empty(val_empty[15]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_16 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[16]), // input wr_en 
		.re(val_rd_en[16]), // input rd_en 
		.dout(val[(16+1)*`VEC_VAL_BITS-1:16*`VEC_VAL_BITS]), 
		.full(val_full[16]), // output full 
		.empty(val_empty[16]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_17 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[17]), // input wr_en 
		.re(val_rd_en[17]), // input rd_en 
		.dout(val[(17+1)*`VEC_VAL_BITS-1:17*`VEC_VAL_BITS]), 
		.full(val_full[17]), // output full 
		.empty(val_empty[17]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_18 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[18]), // input wr_en 
		.re(val_rd_en[18]), // input rd_en 
		.dout(val[(18+1)*`VEC_VAL_BITS-1:18*`VEC_VAL_BITS]), 
		.full(val_full[18]), // output full 
		.empty(val_empty[18]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_19 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[19]), // input wr_en 
		.re(val_rd_en[19]), // input rd_en 
		.dout(val[(19+1)*`VEC_VAL_BITS-1:19*`VEC_VAL_BITS]), 
		.full(val_full[19]), // output full 
		.empty(val_empty[19]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_20 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[20]), // input wr_en 
		.re(val_rd_en[20]), // input rd_en 
		.dout(val[(20+1)*`VEC_VAL_BITS-1:20*`VEC_VAL_BITS]), 
		.full(val_full[20]), // output full 
		.empty(val_empty[20]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_21 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[21]), // input wr_en 
		.re(val_rd_en[21]), // input rd_en 
		.dout(val[(21+1)*`VEC_VAL_BITS-1:21*`VEC_VAL_BITS]), 
		.full(val_full[21]), // output full 
		.empty(val_empty[21]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_22 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[22]), // input wr_en 
		.re(val_rd_en[22]), // input rd_en 
		.dout(val[(22+1)*`VEC_VAL_BITS-1:22*`VEC_VAL_BITS]), 
		.full(val_full[22]), // output full 
		.empty(val_empty[22]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_23 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[23]), // input wr_en 
		.re(val_rd_en[23]), // input rd_en 
		.dout(val[(23+1)*`VEC_VAL_BITS-1:23*`VEC_VAL_BITS]), 
		.full(val_full[23]), // output full 
		.empty(val_empty[23]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_24 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[24]), // input wr_en 
		.re(val_rd_en[24]), // input rd_en 
		.dout(val[(24+1)*`VEC_VAL_BITS-1:24*`VEC_VAL_BITS]), 
		.full(val_full[24]), // output full 
		.empty(val_empty[24]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_25 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[25]), // input wr_en 
		.re(val_rd_en[25]), // input rd_en 
		.dout(val[(25+1)*`VEC_VAL_BITS-1:25*`VEC_VAL_BITS]), 
		.full(val_full[25]), // output full 
		.empty(val_empty[25]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_26 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[26]), // input wr_en 
		.re(val_rd_en[26]), // input rd_en 
		.dout(val[(26+1)*`VEC_VAL_BITS-1:26*`VEC_VAL_BITS]), 
		.full(val_full[26]), // output full 
		.empty(val_empty[26]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_27 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[27]), // input wr_en 
		.re(val_rd_en[27]), // input rd_en 
		.dout(val[(27+1)*`VEC_VAL_BITS-1:27*`VEC_VAL_BITS]), 
		.full(val_full[27]), // output full 
		.empty(val_empty[27]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_28 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[28]), // input wr_en 
		.re(val_rd_en[28]), // input rd_en 
		.dout(val[(28+1)*`VEC_VAL_BITS-1:28*`VEC_VAL_BITS]), 
		.full(val_full[28]), // output full 
		.empty(val_empty[28]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_29 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[29]), // input wr_en 
		.re(val_rd_en[29]), // input rd_en 
		.dout(val[(29+1)*`VEC_VAL_BITS-1:29*`VEC_VAL_BITS]), 
		.full(val_full[29]), // output full 
		.empty(val_empty[29]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_30 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[30]), // input wr_en 
		.re(val_rd_en[30]), // input rd_en 
		.dout(val[(30+1)*`VEC_VAL_BITS-1:30*`VEC_VAL_BITS]), 
		.full(val_full[30]), // output full 
		.empty(val_empty[30]) // output empty 
	);

	generic_fifo_sc_a #( 
		.dw(`VEC_VAL_BITS), 
		.aw(FIFO_DEPTH_BITS) 
	) fifo_bvb_31 ( 
		.clk(clk), // input clk 
		.rst(rst), 
		.clr(0), 
		.din(ram_out), 
		.we(val_wr_en[31]), // input wr_en 
		.re(val_rd_en[31]), // input rd_en 
		.dout(val[(31+1)*`VEC_VAL_BITS-1:31*`VEC_VAL_BITS]), 
		.full(val_full[31]), // output full 
		.empty(val_empty[31]) // output empty 
	);

	assign id_empty_shifted = id_empty >> counter;
	assign val_full_shifted = val_full >> counter;
	assign id_rd_en_local = (start & (~id_empty_shifted) & (~val_full_shifted));
	assign id_rd_en = id_rd_en_local << counter;

	always @ (posedge clk or posedge rst) begin
		if (rst) begin
			counter <= 0;
			counter_delay <= 0;
		end
		else if (start) begin
			counter <= counter + 1;
			counter_delay <= counter;
			id_rd_en_local_reg <= id_rd_en_local;
			val_wr_en <= id_rd_en_local_reg << counter_delay;
			rd_addr <= id >> (counter*`COL_ID_BITS);
		end
	end
endmodule

module Big_Channel(
	input clk, 
	input rst, 
	input start,
	input fetcher_done,
	
	input [`MAT_VAL_BITS * `NUM_CHANNEL -1 :0] mat_val,
	input [`NUM_CHANNEL - 1 : 0] mat_val_empty,
	output [`NUM_CHANNEL - 1 : 0] mat_val_rd_en,

	input [`VEC_VAL_BITS * `NUM_CHANNEL - 1 : 0] vec_val,
	input [`NUM_CHANNEL-1:0] vec_val_empty,
	output [`NUM_CHANNEL-1:0] vec_val_rd_en,

	input [`ROW_ID_BITS * `NUM_CHANNEL -1:0] row_id,
	input [`NUM_CHANNEL-1:0] row_id_empty,
	output [`NUM_CHANNEL-1:0] row_id_rd_en,

	output [`MULT_BITS * `NUM_CHANNEL -1:0] data_out,
	output [`NUM_CHANNEL-1:0] data_out_empty,
	output [`ROW_ID_BITS * `NUM_CHANNEL -1:0] addr_out,
	input [`NUM_CHANNEL-1:0] out_rd_en,

	output [`NUM_CHANNEL-1:0] done
);
	
	Channel_Accumulator CH_0 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(0+1)*`MAT_VAL_BITS-1:0*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[0]), 
		.mat_val_rd_en(mat_val_rd_en[0]), 
		.vec_val(vec_val[(0+1)*`VEC_VAL_BITS-1:0*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[0]), 
		.vec_val_rd_en(vec_val_rd_en[0]), 
		.row_id(row_id[(0+1)*`ROW_ID_BITS-1:0*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[0]), 
		.row_id_rd_en(row_id_rd_en[0]), 
		.data_out(data_out[(0+1)*`MULT_BITS-1:0*`MULT_BITS]), 
		.data_out_empty(data_out_empty[0]), 
		.addr_out(addr_out[(0+1)*`ROW_ID_BITS-1:0*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[0]), 
		.done(done[0]) 
	);

	Channel_Accumulator CH_1 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(1+1)*`MAT_VAL_BITS-1:1*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[1]), 
		.mat_val_rd_en(mat_val_rd_en[1]), 
		.vec_val(vec_val[(1+1)*`VEC_VAL_BITS-1:1*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[1]), 
		.vec_val_rd_en(vec_val_rd_en[1]), 
		.row_id(row_id[(1+1)*`ROW_ID_BITS-1:1*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[1]), 
		.row_id_rd_en(row_id_rd_en[1]), 
		.data_out(data_out[(1+1)*`MULT_BITS-1:1*`MULT_BITS]), 
		.data_out_empty(data_out_empty[1]), 
		.addr_out(addr_out[(1+1)*`ROW_ID_BITS-1:1*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[1]), 
		.done(done[1]) 
	);

	Channel_Accumulator CH_2 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(2+1)*`MAT_VAL_BITS-1:2*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[2]), 
		.mat_val_rd_en(mat_val_rd_en[2]), 
		.vec_val(vec_val[(2+1)*`VEC_VAL_BITS-1:2*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[2]), 
		.vec_val_rd_en(vec_val_rd_en[2]), 
		.row_id(row_id[(2+1)*`ROW_ID_BITS-1:2*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[2]), 
		.row_id_rd_en(row_id_rd_en[2]), 
		.data_out(data_out[(2+1)*`MULT_BITS-1:2*`MULT_BITS]), 
		.data_out_empty(data_out_empty[2]), 
		.addr_out(addr_out[(2+1)*`ROW_ID_BITS-1:2*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[2]), 
		.done(done[2]) 
	);

	Channel_Accumulator CH_3 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(3+1)*`MAT_VAL_BITS-1:3*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[3]), 
		.mat_val_rd_en(mat_val_rd_en[3]), 
		.vec_val(vec_val[(3+1)*`VEC_VAL_BITS-1:3*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[3]), 
		.vec_val_rd_en(vec_val_rd_en[3]), 
		.row_id(row_id[(3+1)*`ROW_ID_BITS-1:3*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[3]), 
		.row_id_rd_en(row_id_rd_en[3]), 
		.data_out(data_out[(3+1)*`MULT_BITS-1:3*`MULT_BITS]), 
		.data_out_empty(data_out_empty[3]), 
		.addr_out(addr_out[(3+1)*`ROW_ID_BITS-1:3*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[3]), 
		.done(done[3]) 
	);

	Channel_Accumulator CH_4 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(4+1)*`MAT_VAL_BITS-1:4*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[4]), 
		.mat_val_rd_en(mat_val_rd_en[4]), 
		.vec_val(vec_val[(4+1)*`VEC_VAL_BITS-1:4*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[4]), 
		.vec_val_rd_en(vec_val_rd_en[4]), 
		.row_id(row_id[(4+1)*`ROW_ID_BITS-1:4*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[4]), 
		.row_id_rd_en(row_id_rd_en[4]), 
		.data_out(data_out[(4+1)*`MULT_BITS-1:4*`MULT_BITS]), 
		.data_out_empty(data_out_empty[4]), 
		.addr_out(addr_out[(4+1)*`ROW_ID_BITS-1:4*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[4]), 
		.done(done[4]) 
	);

	Channel_Accumulator CH_5 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(5+1)*`MAT_VAL_BITS-1:5*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[5]), 
		.mat_val_rd_en(mat_val_rd_en[5]), 
		.vec_val(vec_val[(5+1)*`VEC_VAL_BITS-1:5*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[5]), 
		.vec_val_rd_en(vec_val_rd_en[5]), 
		.row_id(row_id[(5+1)*`ROW_ID_BITS-1:5*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[5]), 
		.row_id_rd_en(row_id_rd_en[5]), 
		.data_out(data_out[(5+1)*`MULT_BITS-1:5*`MULT_BITS]), 
		.data_out_empty(data_out_empty[5]), 
		.addr_out(addr_out[(5+1)*`ROW_ID_BITS-1:5*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[5]), 
		.done(done[5]) 
	);

	Channel_Accumulator CH_6 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(6+1)*`MAT_VAL_BITS-1:6*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[6]), 
		.mat_val_rd_en(mat_val_rd_en[6]), 
		.vec_val(vec_val[(6+1)*`VEC_VAL_BITS-1:6*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[6]), 
		.vec_val_rd_en(vec_val_rd_en[6]), 
		.row_id(row_id[(6+1)*`ROW_ID_BITS-1:6*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[6]), 
		.row_id_rd_en(row_id_rd_en[6]), 
		.data_out(data_out[(6+1)*`MULT_BITS-1:6*`MULT_BITS]), 
		.data_out_empty(data_out_empty[6]), 
		.addr_out(addr_out[(6+1)*`ROW_ID_BITS-1:6*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[6]), 
		.done(done[6]) 
	);

	Channel_Accumulator CH_7 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(7+1)*`MAT_VAL_BITS-1:7*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[7]), 
		.mat_val_rd_en(mat_val_rd_en[7]), 
		.vec_val(vec_val[(7+1)*`VEC_VAL_BITS-1:7*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[7]), 
		.vec_val_rd_en(vec_val_rd_en[7]), 
		.row_id(row_id[(7+1)*`ROW_ID_BITS-1:7*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[7]), 
		.row_id_rd_en(row_id_rd_en[7]), 
		.data_out(data_out[(7+1)*`MULT_BITS-1:7*`MULT_BITS]), 
		.data_out_empty(data_out_empty[7]), 
		.addr_out(addr_out[(7+1)*`ROW_ID_BITS-1:7*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[7]), 
		.done(done[7]) 
	);

	Channel_Accumulator CH_8 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(8+1)*`MAT_VAL_BITS-1:8*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[8]), 
		.mat_val_rd_en(mat_val_rd_en[8]), 
		.vec_val(vec_val[(8+1)*`VEC_VAL_BITS-1:8*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[8]), 
		.vec_val_rd_en(vec_val_rd_en[8]), 
		.row_id(row_id[(8+1)*`ROW_ID_BITS-1:8*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[8]), 
		.row_id_rd_en(row_id_rd_en[8]), 
		.data_out(data_out[(8+1)*`MULT_BITS-1:8*`MULT_BITS]), 
		.data_out_empty(data_out_empty[8]), 
		.addr_out(addr_out[(8+1)*`ROW_ID_BITS-1:8*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[8]), 
		.done(done[8]) 
	);

	Channel_Accumulator CH_9 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(9+1)*`MAT_VAL_BITS-1:9*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[9]), 
		.mat_val_rd_en(mat_val_rd_en[9]), 
		.vec_val(vec_val[(9+1)*`VEC_VAL_BITS-1:9*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[9]), 
		.vec_val_rd_en(vec_val_rd_en[9]), 
		.row_id(row_id[(9+1)*`ROW_ID_BITS-1:9*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[9]), 
		.row_id_rd_en(row_id_rd_en[9]), 
		.data_out(data_out[(9+1)*`MULT_BITS-1:9*`MULT_BITS]), 
		.data_out_empty(data_out_empty[9]), 
		.addr_out(addr_out[(9+1)*`ROW_ID_BITS-1:9*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[9]), 
		.done(done[9]) 
	);

	Channel_Accumulator CH_10 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(10+1)*`MAT_VAL_BITS-1:10*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[10]), 
		.mat_val_rd_en(mat_val_rd_en[10]), 
		.vec_val(vec_val[(10+1)*`VEC_VAL_BITS-1:10*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[10]), 
		.vec_val_rd_en(vec_val_rd_en[10]), 
		.row_id(row_id[(10+1)*`ROW_ID_BITS-1:10*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[10]), 
		.row_id_rd_en(row_id_rd_en[10]), 
		.data_out(data_out[(10+1)*`MULT_BITS-1:10*`MULT_BITS]), 
		.data_out_empty(data_out_empty[10]), 
		.addr_out(addr_out[(10+1)*`ROW_ID_BITS-1:10*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[10]), 
		.done(done[10]) 
	);

	Channel_Accumulator CH_11 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(11+1)*`MAT_VAL_BITS-1:11*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[11]), 
		.mat_val_rd_en(mat_val_rd_en[11]), 
		.vec_val(vec_val[(11+1)*`VEC_VAL_BITS-1:11*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[11]), 
		.vec_val_rd_en(vec_val_rd_en[11]), 
		.row_id(row_id[(11+1)*`ROW_ID_BITS-1:11*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[11]), 
		.row_id_rd_en(row_id_rd_en[11]), 
		.data_out(data_out[(11+1)*`MULT_BITS-1:11*`MULT_BITS]), 
		.data_out_empty(data_out_empty[11]), 
		.addr_out(addr_out[(11+1)*`ROW_ID_BITS-1:11*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[11]), 
		.done(done[11]) 
	);

	Channel_Accumulator CH_12 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(12+1)*`MAT_VAL_BITS-1:12*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[12]), 
		.mat_val_rd_en(mat_val_rd_en[12]), 
		.vec_val(vec_val[(12+1)*`VEC_VAL_BITS-1:12*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[12]), 
		.vec_val_rd_en(vec_val_rd_en[12]), 
		.row_id(row_id[(12+1)*`ROW_ID_BITS-1:12*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[12]), 
		.row_id_rd_en(row_id_rd_en[12]), 
		.data_out(data_out[(12+1)*`MULT_BITS-1:12*`MULT_BITS]), 
		.data_out_empty(data_out_empty[12]), 
		.addr_out(addr_out[(12+1)*`ROW_ID_BITS-1:12*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[12]), 
		.done(done[12]) 
	);

	Channel_Accumulator CH_13 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(13+1)*`MAT_VAL_BITS-1:13*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[13]), 
		.mat_val_rd_en(mat_val_rd_en[13]), 
		.vec_val(vec_val[(13+1)*`VEC_VAL_BITS-1:13*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[13]), 
		.vec_val_rd_en(vec_val_rd_en[13]), 
		.row_id(row_id[(13+1)*`ROW_ID_BITS-1:13*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[13]), 
		.row_id_rd_en(row_id_rd_en[13]), 
		.data_out(data_out[(13+1)*`MULT_BITS-1:13*`MULT_BITS]), 
		.data_out_empty(data_out_empty[13]), 
		.addr_out(addr_out[(13+1)*`ROW_ID_BITS-1:13*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[13]), 
		.done(done[13]) 
	);

	Channel_Accumulator CH_14 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(14+1)*`MAT_VAL_BITS-1:14*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[14]), 
		.mat_val_rd_en(mat_val_rd_en[14]), 
		.vec_val(vec_val[(14+1)*`VEC_VAL_BITS-1:14*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[14]), 
		.vec_val_rd_en(vec_val_rd_en[14]), 
		.row_id(row_id[(14+1)*`ROW_ID_BITS-1:14*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[14]), 
		.row_id_rd_en(row_id_rd_en[14]), 
		.data_out(data_out[(14+1)*`MULT_BITS-1:14*`MULT_BITS]), 
		.data_out_empty(data_out_empty[14]), 
		.addr_out(addr_out[(14+1)*`ROW_ID_BITS-1:14*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[14]), 
		.done(done[14]) 
	);

	Channel_Accumulator CH_15 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(15+1)*`MAT_VAL_BITS-1:15*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[15]), 
		.mat_val_rd_en(mat_val_rd_en[15]), 
		.vec_val(vec_val[(15+1)*`VEC_VAL_BITS-1:15*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[15]), 
		.vec_val_rd_en(vec_val_rd_en[15]), 
		.row_id(row_id[(15+1)*`ROW_ID_BITS-1:15*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[15]), 
		.row_id_rd_en(row_id_rd_en[15]), 
		.data_out(data_out[(15+1)*`MULT_BITS-1:15*`MULT_BITS]), 
		.data_out_empty(data_out_empty[15]), 
		.addr_out(addr_out[(15+1)*`ROW_ID_BITS-1:15*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[15]), 
		.done(done[15]) 
	);

	Channel_Accumulator CH_16 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(16+1)*`MAT_VAL_BITS-1:16*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[16]), 
		.mat_val_rd_en(mat_val_rd_en[16]), 
		.vec_val(vec_val[(16+1)*`VEC_VAL_BITS-1:16*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[16]), 
		.vec_val_rd_en(vec_val_rd_en[16]), 
		.row_id(row_id[(16+1)*`ROW_ID_BITS-1:16*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[16]), 
		.row_id_rd_en(row_id_rd_en[16]), 
		.data_out(data_out[(16+1)*`MULT_BITS-1:16*`MULT_BITS]), 
		.data_out_empty(data_out_empty[16]), 
		.addr_out(addr_out[(16+1)*`ROW_ID_BITS-1:16*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[16]), 
		.done(done[16]) 
	);

	Channel_Accumulator CH_17 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(17+1)*`MAT_VAL_BITS-1:17*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[17]), 
		.mat_val_rd_en(mat_val_rd_en[17]), 
		.vec_val(vec_val[(17+1)*`VEC_VAL_BITS-1:17*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[17]), 
		.vec_val_rd_en(vec_val_rd_en[17]), 
		.row_id(row_id[(17+1)*`ROW_ID_BITS-1:17*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[17]), 
		.row_id_rd_en(row_id_rd_en[17]), 
		.data_out(data_out[(17+1)*`MULT_BITS-1:17*`MULT_BITS]), 
		.data_out_empty(data_out_empty[17]), 
		.addr_out(addr_out[(17+1)*`ROW_ID_BITS-1:17*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[17]), 
		.done(done[17]) 
	);

	Channel_Accumulator CH_18 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(18+1)*`MAT_VAL_BITS-1:18*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[18]), 
		.mat_val_rd_en(mat_val_rd_en[18]), 
		.vec_val(vec_val[(18+1)*`VEC_VAL_BITS-1:18*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[18]), 
		.vec_val_rd_en(vec_val_rd_en[18]), 
		.row_id(row_id[(18+1)*`ROW_ID_BITS-1:18*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[18]), 
		.row_id_rd_en(row_id_rd_en[18]), 
		.data_out(data_out[(18+1)*`MULT_BITS-1:18*`MULT_BITS]), 
		.data_out_empty(data_out_empty[18]), 
		.addr_out(addr_out[(18+1)*`ROW_ID_BITS-1:18*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[18]), 
		.done(done[18]) 
	);

	Channel_Accumulator CH_19 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(19+1)*`MAT_VAL_BITS-1:19*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[19]), 
		.mat_val_rd_en(mat_val_rd_en[19]), 
		.vec_val(vec_val[(19+1)*`VEC_VAL_BITS-1:19*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[19]), 
		.vec_val_rd_en(vec_val_rd_en[19]), 
		.row_id(row_id[(19+1)*`ROW_ID_BITS-1:19*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[19]), 
		.row_id_rd_en(row_id_rd_en[19]), 
		.data_out(data_out[(19+1)*`MULT_BITS-1:19*`MULT_BITS]), 
		.data_out_empty(data_out_empty[19]), 
		.addr_out(addr_out[(19+1)*`ROW_ID_BITS-1:19*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[19]), 
		.done(done[19]) 
	);

	Channel_Accumulator CH_20 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(20+1)*`MAT_VAL_BITS-1:20*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[20]), 
		.mat_val_rd_en(mat_val_rd_en[20]), 
		.vec_val(vec_val[(20+1)*`VEC_VAL_BITS-1:20*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[20]), 
		.vec_val_rd_en(vec_val_rd_en[20]), 
		.row_id(row_id[(20+1)*`ROW_ID_BITS-1:20*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[20]), 
		.row_id_rd_en(row_id_rd_en[20]), 
		.data_out(data_out[(20+1)*`MULT_BITS-1:20*`MULT_BITS]), 
		.data_out_empty(data_out_empty[20]), 
		.addr_out(addr_out[(20+1)*`ROW_ID_BITS-1:20*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[20]), 
		.done(done[20]) 
	);

	Channel_Accumulator CH_21 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(21+1)*`MAT_VAL_BITS-1:21*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[21]), 
		.mat_val_rd_en(mat_val_rd_en[21]), 
		.vec_val(vec_val[(21+1)*`VEC_VAL_BITS-1:21*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[21]), 
		.vec_val_rd_en(vec_val_rd_en[21]), 
		.row_id(row_id[(21+1)*`ROW_ID_BITS-1:21*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[21]), 
		.row_id_rd_en(row_id_rd_en[21]), 
		.data_out(data_out[(21+1)*`MULT_BITS-1:21*`MULT_BITS]), 
		.data_out_empty(data_out_empty[21]), 
		.addr_out(addr_out[(21+1)*`ROW_ID_BITS-1:21*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[21]), 
		.done(done[21]) 
	);

	Channel_Accumulator CH_22 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(22+1)*`MAT_VAL_BITS-1:22*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[22]), 
		.mat_val_rd_en(mat_val_rd_en[22]), 
		.vec_val(vec_val[(22+1)*`VEC_VAL_BITS-1:22*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[22]), 
		.vec_val_rd_en(vec_val_rd_en[22]), 
		.row_id(row_id[(22+1)*`ROW_ID_BITS-1:22*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[22]), 
		.row_id_rd_en(row_id_rd_en[22]), 
		.data_out(data_out[(22+1)*`MULT_BITS-1:22*`MULT_BITS]), 
		.data_out_empty(data_out_empty[22]), 
		.addr_out(addr_out[(22+1)*`ROW_ID_BITS-1:22*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[22]), 
		.done(done[22]) 
	);

	Channel_Accumulator CH_23 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(23+1)*`MAT_VAL_BITS-1:23*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[23]), 
		.mat_val_rd_en(mat_val_rd_en[23]), 
		.vec_val(vec_val[(23+1)*`VEC_VAL_BITS-1:23*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[23]), 
		.vec_val_rd_en(vec_val_rd_en[23]), 
		.row_id(row_id[(23+1)*`ROW_ID_BITS-1:23*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[23]), 
		.row_id_rd_en(row_id_rd_en[23]), 
		.data_out(data_out[(23+1)*`MULT_BITS-1:23*`MULT_BITS]), 
		.data_out_empty(data_out_empty[23]), 
		.addr_out(addr_out[(23+1)*`ROW_ID_BITS-1:23*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[23]), 
		.done(done[23]) 
	);

	Channel_Accumulator CH_24 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(24+1)*`MAT_VAL_BITS-1:24*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[24]), 
		.mat_val_rd_en(mat_val_rd_en[24]), 
		.vec_val(vec_val[(24+1)*`VEC_VAL_BITS-1:24*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[24]), 
		.vec_val_rd_en(vec_val_rd_en[24]), 
		.row_id(row_id[(24+1)*`ROW_ID_BITS-1:24*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[24]), 
		.row_id_rd_en(row_id_rd_en[24]), 
		.data_out(data_out[(24+1)*`MULT_BITS-1:24*`MULT_BITS]), 
		.data_out_empty(data_out_empty[24]), 
		.addr_out(addr_out[(24+1)*`ROW_ID_BITS-1:24*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[24]), 
		.done(done[24]) 
	);

	Channel_Accumulator CH_25 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(25+1)*`MAT_VAL_BITS-1:25*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[25]), 
		.mat_val_rd_en(mat_val_rd_en[25]), 
		.vec_val(vec_val[(25+1)*`VEC_VAL_BITS-1:25*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[25]), 
		.vec_val_rd_en(vec_val_rd_en[25]), 
		.row_id(row_id[(25+1)*`ROW_ID_BITS-1:25*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[25]), 
		.row_id_rd_en(row_id_rd_en[25]), 
		.data_out(data_out[(25+1)*`MULT_BITS-1:25*`MULT_BITS]), 
		.data_out_empty(data_out_empty[25]), 
		.addr_out(addr_out[(25+1)*`ROW_ID_BITS-1:25*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[25]), 
		.done(done[25]) 
	);

	Channel_Accumulator CH_26 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(26+1)*`MAT_VAL_BITS-1:26*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[26]), 
		.mat_val_rd_en(mat_val_rd_en[26]), 
		.vec_val(vec_val[(26+1)*`VEC_VAL_BITS-1:26*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[26]), 
		.vec_val_rd_en(vec_val_rd_en[26]), 
		.row_id(row_id[(26+1)*`ROW_ID_BITS-1:26*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[26]), 
		.row_id_rd_en(row_id_rd_en[26]), 
		.data_out(data_out[(26+1)*`MULT_BITS-1:26*`MULT_BITS]), 
		.data_out_empty(data_out_empty[26]), 
		.addr_out(addr_out[(26+1)*`ROW_ID_BITS-1:26*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[26]), 
		.done(done[26]) 
	);

	Channel_Accumulator CH_27 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(27+1)*`MAT_VAL_BITS-1:27*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[27]), 
		.mat_val_rd_en(mat_val_rd_en[27]), 
		.vec_val(vec_val[(27+1)*`VEC_VAL_BITS-1:27*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[27]), 
		.vec_val_rd_en(vec_val_rd_en[27]), 
		.row_id(row_id[(27+1)*`ROW_ID_BITS-1:27*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[27]), 
		.row_id_rd_en(row_id_rd_en[27]), 
		.data_out(data_out[(27+1)*`MULT_BITS-1:27*`MULT_BITS]), 
		.data_out_empty(data_out_empty[27]), 
		.addr_out(addr_out[(27+1)*`ROW_ID_BITS-1:27*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[27]), 
		.done(done[27]) 
	);

	Channel_Accumulator CH_28 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(28+1)*`MAT_VAL_BITS-1:28*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[28]), 
		.mat_val_rd_en(mat_val_rd_en[28]), 
		.vec_val(vec_val[(28+1)*`VEC_VAL_BITS-1:28*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[28]), 
		.vec_val_rd_en(vec_val_rd_en[28]), 
		.row_id(row_id[(28+1)*`ROW_ID_BITS-1:28*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[28]), 
		.row_id_rd_en(row_id_rd_en[28]), 
		.data_out(data_out[(28+1)*`MULT_BITS-1:28*`MULT_BITS]), 
		.data_out_empty(data_out_empty[28]), 
		.addr_out(addr_out[(28+1)*`ROW_ID_BITS-1:28*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[28]), 
		.done(done[28]) 
	);

	Channel_Accumulator CH_29 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(29+1)*`MAT_VAL_BITS-1:29*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[29]), 
		.mat_val_rd_en(mat_val_rd_en[29]), 
		.vec_val(vec_val[(29+1)*`VEC_VAL_BITS-1:29*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[29]), 
		.vec_val_rd_en(vec_val_rd_en[29]), 
		.row_id(row_id[(29+1)*`ROW_ID_BITS-1:29*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[29]), 
		.row_id_rd_en(row_id_rd_en[29]), 
		.data_out(data_out[(29+1)*`MULT_BITS-1:29*`MULT_BITS]), 
		.data_out_empty(data_out_empty[29]), 
		.addr_out(addr_out[(29+1)*`ROW_ID_BITS-1:29*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[29]), 
		.done(done[29]) 
	);

	Channel_Accumulator CH_30 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(30+1)*`MAT_VAL_BITS-1:30*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[30]), 
		.mat_val_rd_en(mat_val_rd_en[30]), 
		.vec_val(vec_val[(30+1)*`VEC_VAL_BITS-1:30*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[30]), 
		.vec_val_rd_en(vec_val_rd_en[30]), 
		.row_id(row_id[(30+1)*`ROW_ID_BITS-1:30*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[30]), 
		.row_id_rd_en(row_id_rd_en[30]), 
		.data_out(data_out[(30+1)*`MULT_BITS-1:30*`MULT_BITS]), 
		.data_out_empty(data_out_empty[30]), 
		.addr_out(addr_out[(30+1)*`ROW_ID_BITS-1:30*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[30]), 
		.done(done[30]) 
	);

	Channel_Accumulator CH_31 ( 
		.clk(clk), 
		.rst(rst), 
		.start(start), 
		.fetcher_done(fetcher_done), 
		.mat_val(mat_val[(31+1)*`MAT_VAL_BITS-1:31*`MAT_VAL_BITS]), 
		.mat_val_empty(mat_val_empty[31]), 
		.mat_val_rd_en(mat_val_rd_en[31]), 
		.vec_val(vec_val[(31+1)*`VEC_VAL_BITS-1:31*`VEC_VAL_BITS]), 
		.vec_val_empty(vec_val_empty[31]), 
		.vec_val_rd_en(vec_val_rd_en[31]), 
		.row_id(row_id[(31+1)*`ROW_ID_BITS-1:31*`ROW_ID_BITS]), 
		.row_id_empty(row_id_empty[31]), 
		.row_id_rd_en(row_id_rd_en[31]), 
		.data_out(data_out[(31+1)*`MULT_BITS-1:31*`MULT_BITS]), 
		.data_out_empty(data_out_empty[31]), 
		.addr_out(addr_out[(31+1)*`ROW_ID_BITS-1:31*`ROW_ID_BITS]),
		.out_rd_en(out_rd_en[31]), 
		.done(done[31]) 
	);
endmodule

module Channel_Accumulator(
	input clk, 
	input rst, 
	input start,
	input fetcher_done,
	
	input [`MAT_VAL_BITS-1:0] mat_val,
	input mat_val_empty,
	output mat_val_rd_en,

	input [`VEC_VAL_BITS-1:0] vec_val,
	input vec_val_empty,
	output vec_val_rd_en,

	input [`ROW_ID_BITS-1:0] row_id,
	input row_id_empty,
	output row_id_rd_en,

	output [`MULT_BITS-1:0] data_out,
	output data_out_empty,
	output [`ROW_ID_BITS-1:0] addr_out,
	input out_rd_en,

	output done
	);
 
	wire mult_rd_en;
	wire mult_empty;
	wire [`MULT_BITS-1:0] mult_out;
	wire mult_done;
	
	Channel CH0(.clk(clk), 
				.rst(rst),
				.start(start),
				.fetcher_done(fetcher_done),
				.mat_val(mat_val), 
				.mat_val_empty(mat_val_empty), 
				.mat_val_rd_en(mat_val_rd_en),
				.vec_val(vec_val), 
				.vec_val_empty(vec_val_empty), 
				.vec_val_rd_en(vec_val_rd_en),
				.mult_out(mult_out),
				.mult_empty(mult_empty), 
				.mult_rd_en(mult_rd_en), 
				.done(mult_done)
	);
					
	Accumulator A0(	.clk(clk), 
					.rst(rst), 
					.start(start),
					.mult_done(mult_done),
					.row_id(row_id), 
					.row_id_empty(row_id_empty), 
					.row_id_rd_en(row_id_rd_en), 
					.mult_out(mult_out),
					.mult_empty(mult_empty), 
					.mult_rd_en(mult_rd_en), 
					.data_out(data_out),
					.data_out_empty(data_out_empty),
					.addr_out(addr_out),
					.out_rd_en(out_rd_en),
					.done(done)			
	);

endmodule

module Channel(
	input clk, 
	input rst,
	input start,
	input fetcher_done,

	input [`MAT_VAL_BITS-1:0] mat_val,
	input mat_val_empty,
	output mat_val_rd_en,

	input [`VEC_VAL_BITS-1:0] vec_val,
	input vec_val_empty,
	output vec_val_rd_en,

	output [`MULT_BITS-1:0] mult_out,
	output mult_empty,
	input mult_rd_en,

	output done 
	);

	parameter FIFO_DEPTH = 8;
	// parameter FIFO_DEPTH_BITS = `LOG2(FIFO_DEPTH);
	parameter FIFO_DEPTH_BITS = $clog2(8);

	reg [`MULT_BITS-1:0] mult;
	reg mult_wr_en;
	wire mult_full;
	
	reg vec_val_rd_en_reg;
	reg mat_val_rd_en_reg;

	generic_fifo_sc_a #(
		.dw(`MULT_BITS),
		.aw(FIFO_DEPTH_BITS)
		) fifo_mult (
		.clk(clk),
		.rst(rst),
		.clr(0),
		.din(mult),
		.we(mult_wr_en),
		.re(mult_rd_en),
		.dout(mult_out),
		.full(mult_full),
		.empty(mult_empty)
	);

	assign done = mat_val_empty & fetcher_done & start;

	assign vec_val_rd_en = start & (~vec_val_empty) & (~mat_val_empty) & (~mult_full);
	assign mat_val_rd_en = start & (~vec_val_empty) & (~mat_val_empty) & (~mult_full);

	always@(posedge clk) begin
		vec_val_rd_en_reg <= vec_val_rd_en;
		mat_val_rd_en_reg <= mat_val_rd_en;

		if(vec_val_rd_en_reg && mat_val_rd_en_reg) begin
			mult <= vec_val * mat_val;
			mult_wr_en <= 1;
		end
		else begin
			mult_wr_en <= 0;
		end
	end
endmodule

module Accumulator(
	input clk, 
	input rst, 
	input start, 
	input mult_done,

	input [`ROW_ID_BITS-1:0] row_id,
	input row_id_empty,
	output row_id_rd_en,

	input [`MULT_BITS-1:0] mult_out,
	input mult_empty,
	output mult_rd_en,

	output [`MULT_BITS-1:0] data_out,
	output data_out_empty,
	output [`ROW_ID_BITS-1:0] addr_out,
	input out_rd_en,

	output reg done
	);
	
	reg first_read;

	reg row_id_rd_en_reg;
	reg mult_rd_en_reg;

	wire data_out_full;
	wire addr_out_full_nc;

	wire addr_out_empty_nc;

	reg [`MULT_BITS-1:0] wr_data;
	reg [`ROW_ID_BITS-1:0] wr_addr;
	reg wr_en;

	reg [`MULT_BITS-1:0] wr_data_delay;
	reg [`ROW_ID_BITS-1:0] wr_addr_delay;

	reg last;

	parameter FIFO_DEPTH_BITS = $clog2(`FIFO_DEPTH);

	generic_fifo_sc_a #(
		.aw(FIFO_DEPTH_BITS),
		.dw(`MULT_BITS)
	) fifo_data_out (
		.clk(clk),
		.rst(rst),
		.clr(0),
		.din(wr_data_delay),
		.we(wr_en),
		.re(out_rd_en),
		.dout(data_out),
		.full(data_out_full),
		.empty(data_out_empty)
	);

	generic_fifo_sc_a #(
		.aw(FIFO_DEPTH_BITS),
		.dw(`ROW_ID_BITS)
	) fifo_addr_out (
		.clk(clk),
		.rst(rst),
		.clr(0),
		.din(wr_addr_delay),
		.we(wr_en),
		.re(out_rd_en),
		.dout(addr_out),
		.full(addr_out_full_nc),
		.empty(addr_out_empty_nc)
	);

	assign row_id_rd_en = start & (~row_id_empty) & (~mult_empty) & (~data_out_full);
	assign mult_rd_en = start & (~row_id_empty) & (~mult_empty) & (~data_out_full);

	always@(posedge clk or posedge rst) begin
		if(rst) begin
			wr_data<=0;
			wr_en<=0;
			first_read <= 0;
			done <= 0;
		end 
		else if(start) begin
			last <= mult_done & row_id_empty;
			row_id_rd_en_reg <= row_id_rd_en;
			mult_rd_en_reg <= mult_rd_en;
			wr_addr_delay <= wr_addr;
			wr_data_delay <= wr_data;

			if(~first_read) begin
				wr_addr <= row_id;
				first_read <= 1;
			end 		
			else if(row_id_rd_en_reg & mult_rd_en_reg) begin
				if(row_id!=wr_addr) begin
					wr_en <= 1;
					wr_data <= mult_out;
					wr_addr <= row_id;
				end
				else begin
					wr_en <= 0;
					wr_data <= wr_data + mult_out;
				end
			end
			else if (last) begin
				wr_en <= 1;
				done <= 1;
			end
			else begin
				wr_en <= 0;
			end
		end
	end
endmodule


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Universal FIFO Single Clock                                ////
////                                                             ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
////                                                             ////
////  D/L from: http://www.opencores.org/cores/generic_fifos/    ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2000-2002 Rudolf Usselmann                    ////
////                         www.asics.ws                        ////
////                         rudi@asics.ws                       ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

//  CVS Log
//
//  $Id: generic_fifo_sc_a.v,v 1.1.1.1 2002-09-25 05:42:06 rudi Exp $
//
//  $Date: 2002-09-25 05:42:06 $
//  $Revision: 1.1.1.1 $
//  $Author: rudi $
//  $Locker:  $
//  $State: Exp $
//
// Change History:
//               $Log: not supported by cvs2svn $
//
//
//
//
//
//
//
//
//
//

// `include "timescale.v"

/*

Description
===========

I/Os
----
rst	low active, either sync. or async. master reset (see below how to select)
clr	synchronous clear (just like reset but always synchronous), high active
re	read enable, synchronous, high active
we	read enable, synchronous, high active
din	Data Input
dout	Data Output

full	Indicates the FIFO is full (combinatorial output)
full_r	same as above, but registered output (see note below)
empty	Indicates the FIFO is empty
empty_r	same as above, but registered output (see note below)

full_n		Indicates if the FIFO has space for N entries (combinatorial output)
full_n_r	same as above, but registered output (see note below)
empty_n		Indicates the FIFO has at least N entries (combinatorial output)
empty_n_r	same as above, but registered output (see note below)

level		indicates the FIFO level:
		2'b00	0-25%	 full
		2'b01	25-50%	 full
		2'b10	50-75%	 full
		2'b11	%75-100% full

combinatorial vs. registered status outputs
-------------------------------------------
Both the combinatorial and registered status outputs have exactly the same
synchronous timing. Meaning they are being asserted immediately at the clock
edge after the last read or write. The combinatorial outputs however, pass
through several levels of logic before they are output. The registered status
outputs are direct outputs of a flip-flop. The reason both are provided, is
that the registered outputs require quite a bit of additional logic inside
the FIFO. If you can meet timing of your device with the combinatorial
outputs, use them ! The FIFO will be smaller. If the status signals are
in the critical pass, use the registered outputs, they have a much smaller
output delay (actually only Tcq).

Parameters
----------
The FIFO takes 3 parameters:
dw	Data bus width
aw	Address bus width (Determines the FIFO size by evaluating 2^aw)
n	N is a second status threshold constant for full_n and empty_n
	If you have no need for the second status threshold, do not
	connect the outputs and the logic should be removed by your
	synthesis tool.

Synthesis Results
-----------------
In a Spartan 2e a 8 bit wide, 8 entries deep FIFO, takes 85 LUTs and runs
at about 116 MHz (IO insertion disabled). The registered status outputs
are valid after 2.1NS, the combinatorial once take out to 6.5 NS to be
available.


Misc
----
This design assumes you will do appropriate status checking externally.

IMPORTANT ! writing while the FIFO is full or reading while the FIFO is
empty will place the FIFO in an undefined state.

*/


// Selecting Sync. or Async Reset
// ------------------------------
// Uncomment one of the two lines below. The first line for
// synchronous reset, the second for asynchronous reset

// `define SC_FIFO_ASYNC_RESET				// Uncomment for Syncr. reset
// `define SC_FIFO_ASYNC_RESET	or posedge rst		// Uncomment for Async. reset


module generic_fifo_sc_a
	#(parameter dw=8,
	  parameter aw=8)
	(clk, rst, clr, din, we, dout, re,
	full, empty);

parameter max_size = 1<<aw;

input 			clk, rst, clr;
input 	[dw-1:0]	din;
input 			we;
output	[dw-1:0]	dout;
input 	wire		re;
output			full;
output			empty;

////////////////////////////////////////////////////////////////////
//
// Local Wires
//
wire 	[dw-1:0]	din_nc;
wire 	[dw-1:0]	out_nc;
reg		[aw-1:0]	wp;
wire	[aw-1:0]	wp_pl1;
reg		[aw-1:0]	rp;
wire	[aw-1:0]	rp_pl1;
reg					gb;

////////////////////////////////////////////////////////////////////
//
// Memory Block
//
dpram #(
	.AWIDTH(aw),
	.DWIDTH(dw)
	) u0 (
	.clk(clk),
	.address_a(rp),
	.wren_a(0),
	.data_a(din_nc),
	.out_a(dout),
	.address_b(wp),
	.wren_b(we),
	.data_b(din),
	.out_b(out_nc)
);


////////////////////////////////////////////////////////////////////
//
// Misc Logic
//

always @(posedge clk or posedge rst)
	if(rst)	wp <= {aw{1'b0}};
	else
	if(clr)		wp <= {aw{1'b0}};
	else
	if(we)		wp <= wp_pl1;

assign wp_pl1 = wp + { {aw-1{1'b0}}, 1'b1};

always @(posedge clk or posedge rst)
	if(rst)		rp <= {aw{1'b0}};
	else
	if(clr)		rp <= {aw{1'b0}};
	else
	if(re)		rp <= rp_pl1;

assign rp_pl1 = rp + { {aw-1{1'b0}}, 1'b1};

////////////////////////////////////////////////////////////////////
//
// Combinatorial Full & Empty Flags
//

assign empty = ((wp == rp) & !gb);
assign full  = ((wp == rp) &  gb);

// Guard Bit ...
always @(posedge clk or posedge rst)
	if(rst)						gb <= 1'b0;
	else
	if(clr)						gb <= 1'b0;
	else
	if((wp_pl1 == rp) & we)		gb <= 1'b1;
	else
	if(re)						gb <= 1'b0;

endmodule

module dpram #(
	parameter DWIDTH = 32,
	parameter AWIDTH = 10
  )
  (
	clk,
	address_a,
	address_b,
	wren_a,
	wren_b,
	data_a,
	data_b,
	out_a,
	out_b
);

parameter NUM_WORDS=1<<AWIDTH;

input clk;
input [(AWIDTH-1):0] address_a;
input [(AWIDTH-1):0] address_b;
input  wren_a;
input  wren_b;
input [(DWIDTH-1):0] data_a;
input [(DWIDTH-1):0] data_b;
output reg [(DWIDTH-1):0] out_a;
output reg [(DWIDTH-1):0] out_b;

`ifdef SIMULATION

	reg [DWIDTH-1:0] ram[NUM_WORDS-1:0];

	always @ (posedge clk) begin
		if (wren_a) begin
			ram[address_a] <= data_a;
		end
		else begin
			out_a <= ram[address_a];
		end
	end
	  
	always @ (posedge clk) begin 
		if (wren_b) begin
			ram[address_b] <= data_b;
		end 
		else begin
			out_b <= ram[address_b];
		end
	end

`else

	dual_port_ram u_dual_port_ram(
	.addr1(address_a),
	.we1(wren_a),
	.data1(data_a),
	.out1(out_a),
	.addr2(address_b),
	.we2(wren_b),
	.data2(data_b),
	.out2(out_b),
	.clk(clk)
	);

`endif

endmodule


module spram #(
`ifdef SIMULATION
	parameter INIT="init.txt",
`endif
	parameter AWIDTH=5,
	parameter NUM_WORDS=32,
	parameter DWIDTH=16)
	(
		input clk,
		input [(AWIDTH-1):0] address,
		input wren,
		input [(DWIDTH-1):0] din,
		output reg [(DWIDTH-1):0] dout
	);
	`ifdef SIMULATION
		reg [DWIDTH-1:0] mem [NUM_WORDS-1:0];

		initial begin
			$readmemh(INIT, mem);
		end

		always @ (posedge clk) begin 
			if (wren) begin
				mem[address] <= din;
			end
			else begin
				dout <= mem[address];
			end
		end
	`else 

		single_port_ram u_single_port_ram( 
		.addr(address),
		.we(wren),
		.data(din),
		.out(dout),
		.clk(clk)
		);

	`endif 
endmodule

