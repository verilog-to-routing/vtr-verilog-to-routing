/* Tests unused logic removal to ensure no useful logic is removed from the circuit */





module bm_arithmetic_unused_bits(clk,
	add_31_16_in0, add_31_16_in1, add_31_16_out,
	add_15_0_in0, add_15_0_in1, add_15_0_out,
	add_31_24_in0, add_31_24_in1, add_31_24_out,
	add_7_0_in0, add_7_0_in1, add_7_0_out,
	sub_31_16_in0, sub_31_16_in1, sub_31_16_out,
	sub_15_0_in0, sub_15_0_in1, sub_15_0_out,
	sub_31_24_in0, sub_31_24_in1, sub_31_24_out,
	sub_7_0_in0, sub_7_0_in1, sub_7_0_out,
	mult_31_16_in0, mult_31_16_in1, mult_31_16_out,
	mult_15_0_in0, mult_15_0_in1, mult_15_0_out,
	mult_31_24_in0, mult_31_24_in1, mult_31_24_out,
	mult_7_0_in0, mult_7_0_in1, mult_7_0_out
);

input clk;
input [31:0] add_31_16_in0, add_31_16_in1;
output [15:0] add_31_16_out;
input [31:0] add_15_0_in0, add_15_0_in1;
output [15:0] add_15_0_out;
input [31:0] add_31_24_in0, add_31_24_in1;
output [7:0] add_31_24_out;
input [31:0] add_7_0_in0, add_7_0_in1;
output [7:0] add_7_0_out;
input [31:0] sub_31_16_in0, sub_31_16_in1;
output [15:0] sub_31_16_out;
input [31:0] sub_15_0_in0, sub_15_0_in1;
output [15:0] sub_15_0_out;
input [31:0] sub_31_24_in0, sub_31_24_in1;
output [7:0] sub_31_24_out;
input [31:0] sub_7_0_in0, sub_7_0_in1;
output [7:0] sub_7_0_out;
input [31:0] mult_31_16_in0, mult_31_16_in1;
output [15:0] mult_31_16_out;
input [31:0] mult_15_0_in0, mult_15_0_in1;
output [15:0] mult_15_0_out;
input [31:0] mult_31_24_in0, mult_31_24_in1;
output [7:0] mult_31_24_out;
input [31:0] mult_7_0_in0, mult_7_0_in1;
output [7:0] mult_7_0_out;

wire [31:0] add_31_16_temp;
wire [31:0] add_15_0_temp;
wire [31:0] add_31_24_temp;
wire [31:0] add_7_0_temp;
wire [31:0] sub_31_16_temp;
wire [31:0] sub_15_0_temp;
wire [31:0] sub_31_24_temp;
wire [31:0] sub_7_0_temp;
wire [31:0] mult_31_16_temp;
wire [31:0] mult_15_0_temp;
wire [31:0] mult_31_24_temp;
wire [31:0] mult_7_0_temp;

assign add_31_16_temp = add_31_16_in0 + add_31_16_in1;
always @(posedge clk) add_31_16_out <= add_31_16_temp[31:16];

assign add_15_0_temp = add_15_0_in0 + add_15_0_in1;
always @(posedge clk) add_15_0_out <= add_15_0_temp[15:0];

assign add_31_24_temp = add_31_24_in0 + add_31_24_in1;
always @(posedge clk) add_31_24_out <= add_31_24_temp[31:24];

assign add_7_0_temp = add_7_0_in0 + add_7_0_in1;
always @(posedge clk) add_7_0_out <= add_7_0_temp[7:0];

assign sub_31_16_temp = sub_31_16_in0 - sub_31_16_in1;
always @(posedge clk) sub_31_16_out <= sub_31_16_temp[31:16];

assign sub_15_0_temp = sub_15_0_in0 - sub_15_0_in1;
always @(posedge clk) sub_15_0_out <= sub_15_0_temp[15:0];

assign sub_31_24_temp = sub_31_24_in0 - sub_31_24_in1;
always @(posedge clk) sub_31_24_out <= sub_31_24_temp[31:24];

assign sub_7_0_temp = sub_7_0_in0 - sub_7_0_in1;
always @(posedge clk) sub_7_0_out <= sub_7_0_temp[7:0];

assign mult_31_16_temp = mult_31_16_in0 * mult_31_16_in1;
always @(posedge clk) mult_31_16_out <= mult_31_16_temp[31:16];

assign mult_15_0_temp = mult_15_0_in0 * mult_15_0_in1;
always @(posedge clk) mult_15_0_out <= mult_15_0_temp[15:0];

assign mult_31_24_temp = mult_31_24_in0 * mult_31_24_in1;
always @(posedge clk) mult_31_24_out <= mult_31_24_temp[31:24];

assign mult_7_0_temp = mult_7_0_in0 * mult_7_0_in1;
always @(posedge clk) mult_7_0_out <= mult_7_0_temp[7:0];

endmodule
