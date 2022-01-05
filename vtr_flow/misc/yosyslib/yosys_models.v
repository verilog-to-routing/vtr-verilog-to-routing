/********************************************************************************************
# Description: this file includes the required definitions for Yosys to how it should		#
#			   infer implicit memories and instantiate arithmetic operations, such as 		#
#			   addition, subtraction, and multiplication. In this file, implicit memory 	#
#			   boundary and required splitting process, arithmetic operation depth/width    # 
#			   splitting, and error handling for possible invalid situations are specified.	#
# 																							#
# NOTE: PPP, AAA, and MMM will be replaced with the configuration specifications via the 	#
#		run_vtr_flow.py script file while the VTR flow is executing.						#
# 																							#
# Author: Eddie Hung																		#
# VTR-to-Bitstream: "http://eddiehung.github.io/vtb.html"								#
********************************************************************************************/


`define MEM_MINWIDTH 1
`define MEM_MAXADDR PPP
`define MEM_MAXDATA 36

`define DSP_A_MAXWIDTH 36
`define DSP_B_MAXWIDTH 36

`define ADDER_MINWIDTH AAA
`define MULTIPLY_MINWIDTH MMM

`define MAX(a,b) (a > b ? a : b)
`define MIN(a,b) (a < b ? a : b)

module \$mem (RD_CLK, RD_EN, RD_ADDR, RD_DATA, WR_CLK, WR_EN, WR_ADDR, WR_DATA);
	parameter MEMID = "";
	parameter SIZE = 256;
	parameter OFFSET = 0;
	parameter ABITS = 8;
	parameter WIDTH = 8;
	parameter signed INIT = 1'bx;

	parameter RD_PORTS = 1;
	parameter RD_CLK_ENABLE = 1'b1;
	parameter RD_CLK_POLARITY = 1'b1;
	parameter RD_TRANSPARENT = 1'b1;

	parameter WR_PORTS = 1;
	parameter WR_CLK_ENABLE = 1'b1;
	parameter WR_CLK_POLARITY = 1'b1;

	input [RD_PORTS-1:0] RD_CLK;
	input [RD_PORTS-1:0] RD_EN;
	input [RD_PORTS*ABITS-1:0] RD_ADDR;
	output reg [RD_PORTS*WIDTH-1:0] RD_DATA;

	input [WR_PORTS-1:0] WR_CLK;
	input [WR_PORTS*WIDTH-1:0] WR_EN;
	input [WR_PORTS*ABITS-1:0] WR_ADDR;
	input [WR_PORTS*WIDTH-1:0] WR_DATA;

	wire [1023:0] _TECHMAP_DO_ = "proc; clean";
	
	parameter _TECHMAP_CONNMAP_RD_CLK_ = 0;
	parameter _TECHMAP_CONNMAP_WR_CLK_ = 0;
	
	parameter _TECHMAP_CONNMAP_RD_ADDR_ = 0;
	parameter _TECHMAP_CONNMAP_WR_ADDR_ = 0;

	parameter _TECHMAP_CONNMAP_WR_EN_ = 0;
	parameter _TECHMAP_CONSTVAL_RD_EN_ = 0;

	parameter _TECHMAP_BITS_CONNMAP_ = 0;
	//parameter _TECHMAP_CONNMAP_RD_PORTS_ = 0;
	//parameter _TECHMAP_CONNMAP_WR_PORTS_ = 0;
	
	reg _TECHMAP_FAIL_;
	initial begin
		_TECHMAP_FAIL_ <= 0;
	
		// only map cells with only one read and one write port
		if (RD_PORTS > 2 || WR_PORTS > 2)
			_TECHMAP_FAIL_ <= 1;
	
		// read enable must not be constant low
		if (_TECHMAP_CONSTVAL_RD_EN_[0] == 1'b0)
			_TECHMAP_FAIL_ <= 1;

		// we expect positive read clock and non-transparent reads
		if (RD_TRANSPARENT || !RD_CLK_ENABLE || !RD_CLK_POLARITY)
			_TECHMAP_FAIL_ <= 1;
	
		// we expect positive write clock
		if (!WR_CLK_ENABLE || !WR_CLK_POLARITY)
			_TECHMAP_FAIL_ <= 1;
	
		// read and write must be in same clock domain
		if (_TECHMAP_CONNMAP_RD_CLK_ != _TECHMAP_CONNMAP_WR_CLK_)
			_TECHMAP_FAIL_ <= 1;
	
		// we don't do small memories or memories with offsets
		if (OFFSET != 0 || ABITS < `MEM_MINWIDTH || SIZE < 2**`MEM_MINWIDTH)
			_TECHMAP_FAIL_ <= 1;
	
	end

	genvar i;
	for (i = 0; i < `MAX(RD_PORTS, WR_PORTS); i = i+1) begin
		initial begin
			// check each pair of read and write port are the same
			if (RD_PORTS >= i && WR_PORTS >= i) begin
				if (_TECHMAP_CONNMAP_RD_ADDR_[i*ABITS*_TECHMAP_BITS_CONNMAP_ +: ABITS*_TECHMAP_BITS_CONNMAP_] != 
					_TECHMAP_CONNMAP_WR_ADDR_[i*ABITS*_TECHMAP_BITS_CONNMAP_ +: ABITS*_TECHMAP_BITS_CONNMAP_])
						_TECHMAP_FAIL_ <= 1;
			end
		end
		// check all bits of write enable are the same
		if (i < WR_PORTS) begin
			genvar j;
			for (j = 1; j < WIDTH; j = j+1) begin
				initial begin
					if (_TECHMAP_CONNMAP_WR_EN_[0 +: _TECHMAP_BITS_CONNMAP_] !=
							_TECHMAP_CONNMAP_WR_EN_[j*_TECHMAP_BITS_CONNMAP_ +: _TECHMAP_BITS_CONNMAP_])
						_TECHMAP_FAIL_ <= 1;
				end
			end
		end
	end

	
	\$__mem_gen #(
		.MEMID(MEMID), .SIZE(SIZE), .OFFSET(OFFSET), .ABITS(ABITS), .WIDTH(WIDTH),
		.RD_PORTS(RD_PORTS), .RD_CLK_ENABLE(RD_CLK_ENABLE), .RD_CLK_POLARITY(RD_CLK_POLARITY), .RD_TRANSPARENT(RD_TRANSPARENT),
		.WR_PORTS(WR_PORTS), .WR_CLK_ENABLE(WR_CLK_ENABLE), .WR_CLK_POLARITY(WR_CLK_POLARITY)
	) _TECHMAP_REPLACE_ (
		.RD_CLK(RD_CLK),
		.RD_EN(RD_EN),
		.RD_ADDR(RD_ADDR),
		.RD_DATA(RD_DATA),
		.WR_CLK(WR_CLK),
		.WR_EN(WR_EN),
		.WR_ADDR(WR_ADDR),
		.WR_DATA(WR_DATA)
	);
endmodule

module \$__mem_gen (RD_CLK, RD_EN, RD_ADDR, RD_DATA, WR_CLK, WR_EN, WR_ADDR, WR_DATA);
	parameter MEMID = "";
	parameter SIZE = 256;
	parameter OFFSET = 0;
	parameter ABITS = 8;
	parameter WIDTH = 8;
	parameter signed INIT = 1'bx;

	parameter RD_PORTS = 1;
	parameter RD_CLK_ENABLE = 1'b1;
	parameter RD_CLK_POLARITY = 1'b1;
	parameter RD_TRANSPARENT = 1'b1;

	parameter WR_PORTS = 1;
	parameter WR_CLK_ENABLE = 1'b1;
	parameter WR_CLK_POLARITY = 1'b1;

	input [RD_PORTS-1:0] RD_CLK;
	input [RD_PORTS-1:0] RD_EN;
	input [RD_PORTS*ABITS-1:0] RD_ADDR;
	output reg [RD_PORTS*WIDTH-1:0] RD_DATA;

	input [WR_PORTS-1:0] WR_CLK;
	input [WR_PORTS*WIDTH-1:0] WR_EN;
	input [WR_PORTS*ABITS-1:0] WR_ADDR;
	input [WR_PORTS*WIDTH-1:0] WR_DATA;

	wire [1023:0] _TECHMAP_DO_ = "proc; clean";

	genvar i;
	generate
		if (ABITS > `MEM_MAXADDR) begin
			wire [WIDTH-1:0] rd_data_hi, rd_data_lo;
			wire [(ABITS-1)*RD_PORTS-1:0] rd_addr_new;
			for (i = 0; i < RD_PORTS; i = i+1) begin
				assign rd_addr_new[(ABITS-1)*(i+1)-1:(ABITS-1)*i] = RD_ADDR[ABITS*(i+1)-2:ABITS*i];
			end
			wire [(ABITS-1)*WR_PORTS-1:0] wr_addr_new;
			wire [WR_PORTS-1:0] wr_en_new;
			for (i = 0; i < WR_PORTS; i = i+1) begin
				assign wr_addr_new[(ABITS-1)*(i+1)-1:(ABITS-1)*i] = WR_ADDR[ABITS*(i+1)-2:ABITS*i];
				assign wr_en_new[i] = WR_EN[i] & WR_ADDR[ABITS*(i+1)-1];
			end

			if (SIZE > 2**(ABITS-1)) begin
				\$__mem_gen #(
					.MEMID(MEMID), .SIZE(SIZE - 2**(ABITS-1)), .OFFSET(OFFSET), .ABITS(ABITS-1), .WIDTH(WIDTH),
					.RD_PORTS(RD_PORTS), .RD_CLK_ENABLE(RD_CLK_ENABLE), .RD_CLK_POLARITY(RD_CLK_POLARITY), .RD_TRANSPARENT(RD_TRANSPARENT),
					.WR_PORTS(WR_PORTS), .WR_CLK_ENABLE(WR_CLK_ENABLE), .WR_CLK_POLARITY(WR_CLK_POLARITY)
				) mem_hi (
					.RD_CLK(RD_CLK),
					.RD_EN(RD_EN),
					.RD_ADDR(rd_addr_new),
					.RD_DATA(rd_data_hi),
					.WR_CLK(WR_CLK),
					.WR_EN(wr_en_new),
					.WR_ADDR(wr_addr_new),
					.WR_DATA(WR_DATA)
				);
			end 
			else begin
				assign rd_data_hi = {{WIDTH}{1'bx}};
			end

			\$__mem_gen #(
				.MEMID(MEMID), .SIZE(SIZE > 2**(ABITS-1) ? 2**(ABITS-1) : SIZE), .OFFSET(OFFSET), .ABITS(ABITS-1), .WIDTH(WIDTH),
				.RD_PORTS(RD_PORTS), .RD_CLK_ENABLE(RD_CLK_ENABLE), .RD_CLK_POLARITY(RD_CLK_POLARITY), .RD_TRANSPARENT(RD_TRANSPARENT),
				.WR_PORTS(WR_PORTS), .WR_CLK_ENABLE(WR_CLK_ENABLE), .WR_CLK_POLARITY(WR_CLK_POLARITY)
			) mem_lo (
				.RD_CLK(RD_CLK),
				.RD_EN(RD_EN),
				.RD_ADDR(rd_addr_new),
				.RD_DATA(rd_data_lo),
				.WR_CLK(WR_CLK),
				.WR_EN(wr_en_new),
				.WR_ADDR(wr_addr_new),
				.WR_DATA(WR_DATA)
			);

			reg [RD_PORTS-1:0] delayed_abit;
			for (i = 0; i < RD_PORTS; i = i+1) begin
				always @(posedge RD_CLK[i])
					delayed_abit[i] <= RD_ADDR[ABITS*(i+1)-1];
				assign RD_DATA[WIDTH*(i+1)-1:WIDTH*i] = delayed_abit[i] ? rd_data_hi : rd_data_lo;
			end
		end 
		else begin
			for (i = 0; i < WIDTH; i=i+1) begin:slice
				if (RD_PORTS <= 1 && WR_PORTS <= 1) 
					single_port_ram mem (
						.clk(RD_CLK[0]),
						.addr({ {{`MEM_MAXADDR-ABITS}{1'bx}}, RD_ADDR[ABITS-1:0] }),
						.data(WR_DATA[i]),
						.out(RD_DATA[i]),
						.we(WR_EN[0])
					);
				else if (RD_PORTS <= 2 && WR_PORTS <= 2) 
					dual_port_ram mem (
						.clk(RD_CLK[0]),
						.addr1({ {{`MEM_MAXADDR-ABITS}{1'bx}}, RD_ADDR[ABITS-1:0] }),
						.data1(WR_DATA[i]),
						.out1(RD_DATA[i]),
						.we1(WR_EN[0]),
						.addr2({ {{`MEM_MAXADDR-ABITS}{1'bx}}, RD_ADDR[2*ABITS-1:ABITS] }),
						.data2(WR_DATA[WIDTH+i]),
						.out2(RD_DATA[WIDTH+i]),
						.we2(WR_EN[WIDTH])
					);
			end
		end
	endgenerate
endmodule

(* techmap_celltype = "$mul" *)
module mul_swap_ports (A, B, Y);
parameter A_SIGNED = 0;
parameter B_SIGNED = 0;
parameter A_WIDTH = 1;
parameter B_WIDTH = 1;
parameter Y_WIDTH = 1;

input [A_WIDTH-1:0] A;
input [B_WIDTH-1:0] B;
output [Y_WIDTH-1:0] Y;

wire _TECHMAP_FAIL_ = A_WIDTH <= B_WIDTH;

\$mul #(
	.A_SIGNED(B_SIGNED),
	.B_SIGNED(A_SIGNED),
	.A_WIDTH(B_WIDTH),
	.B_WIDTH(A_WIDTH),
	.Y_WIDTH(Y_WIDTH)
) _TECHMAP_REPLACE_ (
	.A(B),
	.B(A),
	.Y(Y)
);

endmodule

module \$mul (A, B, Y);
	parameter A_SIGNED = 0;
	parameter B_SIGNED = 0;
	parameter A_WIDTH = 1;
	parameter B_WIDTH = 1;
	parameter Y_WIDTH = 1;

	input [A_WIDTH-1:0] A;
	input [B_WIDTH-1:0] B;
	output [Y_WIDTH-1:0] Y;

	wire [1023:0] _TECHMAP_DO_ = "proc; clean";

	reg _TECHMAP_FAIL_;
	initial begin
		_TECHMAP_FAIL_ <= 0;

		//if (A_SIGNED || B_SIGNED)
		//	_TECHMAP_FAIL_ <= 1;

		if (A_WIDTH < B_WIDTH)
			_TECHMAP_FAIL_ <= 1;
		if (A_WIDTH < `MULTIPLY_MINWIDTH || B_WIDTH < `MULTIPLY_MINWIDTH)
			_TECHMAP_FAIL_ <= 1;
	end

	generate
		\$__mul_gen #(
			.A_SIGNED(A_SIGNED),
			.B_SIGNED(B_SIGNED),
			.A_WIDTH(A_WIDTH),
			.B_WIDTH(B_WIDTH),
			.Y_WIDTH(Y_WIDTH)
		) mul_slice (
			.A(A),
			.B(B),
			.Y(Y[Y_WIDTH-1:0])
		);
	endgenerate
endmodule

module \$__mul_gen (A, B, Y);
	parameter A_SIGNED = 0;
	parameter B_SIGNED = 0;
	parameter A_WIDTH = 1;
	parameter B_WIDTH = 1;
	parameter Y_WIDTH = 1;

	input [A_WIDTH-1:0] A;
	input [B_WIDTH-1:0] B;
	output [Y_WIDTH-1:0] Y;

	wire [1023:0] _TECHMAP_DO_ = "proc; clean";

	generate
		if (A_WIDTH > `DSP_A_MAXWIDTH) begin
			localparam n_floored = A_WIDTH/`DSP_A_MAXWIDTH;
			localparam n = n_floored + (n_floored*`DSP_A_MAXWIDTH < A_WIDTH ? 1 : 0);
			wire [`DSP_A_MAXWIDTH+B_WIDTH-1:0] partial [n-1:1];
			wire [Y_WIDTH-1:0] partial_sum [n-2:0];

			\$__mul_gen #(
				.A_SIGNED(A_SIGNED),
				.B_SIGNED(B_SIGNED),
				.A_WIDTH(`DSP_A_MAXWIDTH),
				.B_WIDTH(B_WIDTH),
				.Y_WIDTH(Y_WIDTH)
			) mul_slice_first (
				.A(A[`DSP_A_MAXWIDTH-1:0]),
				.B(B),
				.Y(partial_sum[0])
			);

			genvar i;
			generate
			for (i = 1; i < n-1; i=i+1) begin:slice
				\$__mul_gen #(
					.A_SIGNED(A_SIGNED),
					.B_SIGNED(B_SIGNED),
					.A_WIDTH(`DSP_A_MAXWIDTH),
					.B_WIDTH(B_WIDTH),
					.Y_WIDTH(Y_WIDTH)
				) mul_slice (
					.A(A[(i+1)*`DSP_A_MAXWIDTH-1:i*`DSP_A_MAXWIDTH]),
					.B(B),
					.Y(partial[i])
				);
				//assign partial_sum[i] = (partial[i] << i*`DSP_A_MAXWIDTH) + partial_sum[i-1];
				assign partial_sum[i] = {
					partial[i][B_WIDTH+`DSP_A_MAXWIDTH-1:`DSP_A_MAXWIDTH],
					partial[i][`DSP_A_MAXWIDTH-1:0] + partial_sum[i-1][B_WIDTH+(i*`DSP_A_MAXWIDTH)-1:B_WIDTH+((i-1)*`DSP_A_MAXWIDTH)],
					partial_sum[i-1][B_WIDTH+((i-1)*`DSP_A_MAXWIDTH):0]
				};
			end
			endgenerate

			\$__mul_gen #(
				.A_SIGNED(A_SIGNED),
				.B_SIGNED(B_SIGNED),
				.A_WIDTH(A_WIDTH-(n-1)*`DSP_A_MAXWIDTH),
				.B_WIDTH(B_WIDTH),
				.Y_WIDTH(Y_WIDTH)
			) mul_slice_last (
				.A(A[A_WIDTH-1:(n-1)*`DSP_A_MAXWIDTH]),
				.B(B),
				.Y(partial[n-1])
			);
			//assign Y = (partial[n-1] << (n-1)*`DSP_A_MAXWIDTH) + partial_sum[n-2];
			assign Y = {
				partial[n-1][B_WIDTH+`DSP_A_MAXWIDTH-1:`DSP_A_MAXWIDTH],
				partial[n-1][`DSP_A_MAXWIDTH-1:0] + partial_sum[n-2][B_WIDTH+((n-1)*`DSP_A_MAXWIDTH)-1:B_WIDTH+((n-2)*`DSP_A_MAXWIDTH)],
				partial_sum[n-2][B_WIDTH+((n-2)*`DSP_A_MAXWIDTH):0]
			};
		end
		else if (B_WIDTH > `DSP_B_MAXWIDTH) begin
			localparam n_floored = B_WIDTH/`DSP_B_MAXWIDTH;
			localparam n = n_floored + (n_floored*`DSP_B_MAXWIDTH < B_WIDTH ? 1 : 0);
			wire [A_WIDTH+`DSP_B_MAXWIDTH-1:0] partial [n-1:1];
			wire [Y_WIDTH-1:0] partial_sum [n-2:0];

			\$__mul_gen #(
				.A_SIGNED(A_SIGNED),
				.B_SIGNED(B_SIGNED),
				.A_WIDTH(A_WIDTH),
				.B_WIDTH(`DSP_B_MAXWIDTH),
				.Y_WIDTH(Y_WIDTH)
			) mul_first (
				.A(A),
				.B(B[`DSP_B_MAXWIDTH-1:0]),
				.Y(partial_sum[0])
			);

			genvar i;
			generate
			for (i = 1; i < n-1; i=i+1) begin:slice
				\$__mul_gen #(
					.A_SIGNED(A_SIGNED),
					.B_SIGNED(B_SIGNED),
					.A_WIDTH(A_WIDTH),
					.B_WIDTH(`DSP_B_MAXWIDTH),
					.Y_WIDTH(Y_WIDTH)
				) mul (
					.A(A),
					.B(B[(i+1)*`DSP_B_MAXWIDTH-1:i*`DSP_B_MAXWIDTH]),
					.Y(partial[i])
				);
				//assign partial_sum[i] = (partial[i] << i*`DSP_B_MAXWIDTH) + partial_sum[i-1];
				assign partial_sum[i] = {
					partial[i][A_WIDTH+`DSP_B_MAXWIDTH-1:`DSP_B_MAXWIDTH],
					partial[i][`DSP_B_MAXWIDTH-1:0] + partial_sum[i-1][A_WIDTH+(i*`DSP_B_MAXWIDTH)-1:A_WIDTH+((i-1)*`DSP_B_MAXWIDTH)],
					partial_sum[i-1][A_WIDTH+((i-1)*`DSP_B_MAXWIDTH):0]
				};
			end
			endgenerate

			\$__mul_gen #(
				.A_SIGNED(A_SIGNED),
				.B_SIGNED(B_SIGNED),
				.A_WIDTH(A_WIDTH),
				.B_WIDTH(B_WIDTH-(n-1)*`DSP_B_MAXWIDTH),
				.Y_WIDTH(Y_WIDTH)
			) mul_last (
				.A(A),
				.B(B[B_WIDTH-1:(n-1)*`DSP_B_MAXWIDTH]),
				.Y(partial[n-1])
			);
			//assign Y = (partial[n-1] << (n-1)*`DSP_B_MAXWIDTH) + partial_sum[n-2];
			assign Y = {
				partial[n-1][A_WIDTH+`DSP_B_MAXWIDTH-1:`DSP_B_MAXWIDTH],
				partial[n-1][`DSP_B_MAXWIDTH-1:0] + partial_sum[n-2][A_WIDTH+((n-1)*`DSP_B_MAXWIDTH)-1:A_WIDTH+((n-2)*`DSP_B_MAXWIDTH)],
				partial_sum[n-2][A_WIDTH+((n-2)*`DSP_B_MAXWIDTH):0]
			};
		end
		else begin 
			wire [A_WIDTH+B_WIDTH-1:0] out;
			wire [(`DSP_A_MAXWIDTH+`DSP_B_MAXWIDTH)-(A_WIDTH+B_WIDTH)-1:0] dummy;
			wire Asign, Bsign;
			assign Asign = (A_SIGNED ? A[A_WIDTH-1] : 1'b0);
			assign Bsign = (B_SIGNED ? B[B_WIDTH-1] : 1'b0);
			multiply _TECHMAP_REPLACE_ (
				.a({ {{`DSP_A_MAXWIDTH-A_WIDTH}{Asign}}, A }),
				.b({ {{`DSP_B_MAXWIDTH-B_WIDTH}{Bsign}}, B }),
				.out({dummy, out})
			);
			if (Y_WIDTH < A_WIDTH + B_WIDTH)
				assign Y = out[Y_WIDTH-1:0];
			else begin
				wire Ysign = (A_SIGNED || B_SIGNED ? out[A_WIDTH+BWIDTH-1] : 1'b0);
				assign Y = { {{Y_WIDTH-(A_WIDTH+B_WIDTH)}{Ysign}}, out[A_WIDTH+B_WIDTH-1:0] };
			end
		end
	endgenerate
endmodule

module \$add (A, B, Y);
	parameter A_SIGNED = 0;
	parameter A_WIDTH = 1;
	parameter B_SIGNED = 0;
	parameter B_WIDTH = 1;
	parameter Y_WIDTH = 1;
	
	input [A_WIDTH-1:0] A;
	input [B_WIDTH-1:0] B;
	output [Y_WIDTH-1:0] Y;
	
	wire [1023:0] _TECHMAP_DO_ = "proc; clean";

	reg _TECHMAP_FAIL_;
	initial begin
		_TECHMAP_FAIL_ <= 0;

		//if (A_SIGNED || B_SIGNED)
		//	_TECHMAP_FAIL_ <= 1;
		//if (A_WIDTH < 1 || B_WIDTH < 1)
		//	_TECHMAP_FAIL_ <= 1;
		if (Y_WIDTH < `ADDER_MINWIDTH)
			_TECHMAP_FAIL_ <= 1;
		if (A_WIDTH < `ADDER_MINWIDTH && B_WIDTH < `ADDER_MINWIDTH)
			_TECHMAP_FAIL_ <= 1;
	end

	localparam maxab = `MAX(A_WIDTH, B_WIDTH);
	localparam width = `MIN(Y_WIDTH, maxab+1);
	
	generate
		wire [maxab-1:0] _a;
		wire [maxab-1:0] _b;
		wire [width-1:0] _y;
		wire [width:0] _c;
		wire [width:0] dummy;
		wire Asign, Bsign;

		assign Asign = (A_SIGNED ? A[A_WIDTH-1] : 1'b0);
		assign Bsign = (B_SIGNED ? B[B_WIDTH-1] : 1'b0);
		assign _a = { {{maxab}{Asign}}, A };
		assign _b = { {{maxab}{Bsign}}, B };
	
		adder add_first (.a(_a[0]), .b(_b[0]), .cin(1'bx), .cout(_c[1]), .sumout(_y[0]));
		genvar i;
		for (i = 1; i < width; i=i+1) begin : slice
			if (i === maxab)
				adder add_last (.a(1'b0), .b(1'b0), .cin(_c[i]), .cout(dummy[i+1]), .sumout(_y[i]));
			else 
				adder add (.a(_a[i]), .b(_b[i]), .cin(_c[i]), .cout(_c[i+1]), .sumout(_y[i]));
		end

		if (Y_WIDTH < maxab + 1)
			assign Y = _y[Y_WIDTH-1:0];
		else begin
			wire Ysign = (A_SIGNED || B_SIGNED ? _y[Y_WIDTH-1] : 1'b0);
			assign Y = { {{Y_WIDTH-(maxab+1)}{_y[maxab+1-1]}}, _y[maxab+1-1:0] };
		end
	endgenerate
endmodule

module \$sub (A, B, Y);
	parameter A_SIGNED = 0;
	parameter A_WIDTH = 1;
	parameter B_SIGNED = 0;
	parameter B_WIDTH = 1;
	parameter Y_WIDTH = 1;
	input [A_WIDTH-1:0] A;
	input [B_WIDTH-1:0] B;
	output [Y_WIDTH-1:0] Y;
	
	wire [1023:0] _TECHMAP_DO_ = "proc; clean";

	reg _TECHMAP_FAIL_;
	initial begin
		_TECHMAP_FAIL_ <= 0;

		//if (A_SIGNED || B_SIGNED)
		//	_TECHMAP_FAIL_ <= 1;
		if (A_WIDTH < 1 || B_WIDTH < 1)
			_TECHMAP_FAIL_ <= 1;
		if (Y_WIDTH < `ADDER_MINWIDTH)
			_TECHMAP_FAIL_ <= 1;
		if (A_WIDTH < `ADDER_MINWIDTH && B_WIDTH < `ADDER_MINWIDTH)
			_TECHMAP_FAIL_ <= 1;
	end

	localparam maxab = `MAX(A_WIDTH, B_WIDTH);
	localparam width = `MIN(Y_WIDTH, maxab+1);
	
	generate
		wire [maxab-1:0] _a;
		wire [maxab-1:0] _b;
		wire [width-1:0] _y;
		wire [width:0] _c;
		wire [width:0] dummy;
		wire Asign, Bsign;

		assign Asign = (A_SIGNED ? A[A_WIDTH-1] : 1'b0);
		assign Bsign = (B_SIGNED ? B[B_WIDTH-1] : 1'b0);
		assign _a = { {{maxab}{Asign}}, A };
		assign _b = { {{maxab}{Bsign}}, B };
	
		// VPR requires that the first element of a carry chain have cin = 1'bx
		// Therefore use sub_init to generate cout = 1'b1 for the
		// actual first stage
		adder sub_init (.a(1'b1), .b(~1'b0), .cin(1'bx), .cout(_c[0]), .sumout(dummy[0]));
		adder sub_first (.a(_a[0]), .b(~_b[0]), .cin(_c[0]), .cout(_c[1]), .sumout(_y[0]));
		genvar i;
		for (i = 1; i < width; i=i+1) begin : slice
			if (i === maxab)
				adder sub_last (.a(1'b0), .b(~1'b0), .cin(_c[i]), .cout(dummy[i+1]), .sumout(_y[i]));
			else 
				adder sub (.a(_a[i]), .b(~_b[i]), .cin(_c[i]), .cout(_c[i+1]), .sumout(_y[i]));
		end

		if (Y_WIDTH < maxab+1)
			assign Y = _y[Y_WIDTH-1:0];
		else begin
			wire Ysign = (A_SIGNED || B_SIGNED ? _y[Y_WIDTH-1] : 1'b0);
			assign Y = { {{Y_WIDTH-(maxab+1)}{_y[maxab+1-1]}}, _y[maxab+1-1:0] };
		end	endgenerate
endmodule
