// CONFIG:
// NUM_COEFF = 17
// PIPLINED = 0

// WARNING: more than enough COEFFICIENTS in array (there are 26, and we only need 9)
module fir (
	clk,
	reset,
	clk_ena,
	i_valid,
	i_in,
	o_valid,
	o_out
);
	// Data Width
	parameter dw = 18; //Data input/output bits

	// Number of filter coefficients
	parameter N = 17;
	parameter N_UNIQ = 9; // ciel(N/2) assuming symmetric filter coefficients

	//Number of extra valid cycles needed to align output (i.e. computation pipeline depth + input/output registers
	localparam N_VALID_REGS = 18;

	input clk;
	input reset;
	input clk_ena;
	input i_valid;
	input [dw-1:0] i_in; // signed
	output o_valid;
	output [dw-1:0] o_out; // signed

	// Data Width dervied parameters
	localparam dw_add_int = 18; //Internal adder precision bits
	localparam dw_mult_int = 36; //Internal multiplier precision bits
	localparam scale_factor = 17; //Multiplier normalization shift amount

	// Number of extra registers in INPUT_PIPELINE_REG to prevent contention for CHAIN_END's chain adders
	localparam N_INPUT_REGS = 17;

	// Debug
	// initial begin
	// 	$display ("Data Width: %d", dw);
	// 	$display ("Data Width Add Internal: %d", dw_add_int);
	// 	$display ("Data Width Mult Internal: %d", dw_mult_int);
	// 	$display ("Scale Factor: %d", scale_factor);
	// end

	reg [dw-1:0] COEFFICIENT_0;
	reg [dw-1:0] COEFFICIENT_1;
	reg [dw-1:0] COEFFICIENT_2;
	reg [dw-1:0] COEFFICIENT_3;
	reg [dw-1:0] COEFFICIENT_4;
	reg [dw-1:0] COEFFICIENT_5;
	reg [dw-1:0] COEFFICIENT_6;
	reg [dw-1:0] COEFFICIENT_7;
	reg [dw-1:0] COEFFICIENT_8;

	always@(posedge clk) begin
		COEFFICIENT_0 <= 18'd88;
		COEFFICIENT_1 <= 18'd0;
		COEFFICIENT_2 <= -18'd97;
		COEFFICIENT_3 <= -18'd197;
		COEFFICIENT_4 <= -18'd294;
		COEFFICIENT_5 <= -18'd380;
		COEFFICIENT_6 <= -18'd447;
		COEFFICIENT_7 <= -18'd490;
		COEFFICIENT_8 <= -18'd504;
	end

	////******************************************************
	// *
	// * Valid Delay Pipeline
	// *
	// *****************************************************
	//Input valid signal is pipelined to become output valid signal

	//Valid registers
	reg [N_VALID_REGS-1:0] VALID_PIPELINE_REGS;

	always@(posedge clk or posedge reset) begin
		if(reset) begin
			VALID_PIPELINE_REGS <= 0;
		end else begin
			if(clk_ena) begin
				VALID_PIPELINE_REGS <= {VALID_PIPELINE_REGS[N_VALID_REGS-2:0], i_valid};
			end else begin
				VALID_PIPELINE_REGS <= VALID_PIPELINE_REGS;
			end
		end
	end

	////******************************************************
	// *
	// * Input Register Pipeline
	// *
	// *****************************************************
	//Pipelined input values

	//Input value registers

	wire [dw-1:0] INPUT_PIPELINE_REG_0;
	wire [dw-1:0] INPUT_PIPELINE_REG_1;
	wire [dw-1:0] INPUT_PIPELINE_REG_2;
	wire [dw-1:0] INPUT_PIPELINE_REG_3;
	wire [dw-1:0] INPUT_PIPELINE_REG_4;
	wire [dw-1:0] INPUT_PIPELINE_REG_5;
	wire [dw-1:0] INPUT_PIPELINE_REG_6;
	wire [dw-1:0] INPUT_PIPELINE_REG_7;
	wire [dw-1:0] INPUT_PIPELINE_REG_8;
	wire [dw-1:0] INPUT_PIPELINE_REG_9;
	wire [dw-1:0] INPUT_PIPELINE_REG_10;
	wire [dw-1:0] INPUT_PIPELINE_REG_11;
	wire [dw-1:0] INPUT_PIPELINE_REG_12;
	wire [dw-1:0] INPUT_PIPELINE_REG_13;
	wire [dw-1:0] INPUT_PIPELINE_REG_14;
	wire [dw-1:0] INPUT_PIPELINE_REG_15;
	wire [dw-1:0] INPUT_PIPELINE_REG_16;

	input_pipeline in_pipe(
		.clk(clk), .clk_ena(clk_ena),
		.in_stream(i_in),
		.pipeline_reg_0(INPUT_PIPELINE_REG_0),
		.pipeline_reg_1(INPUT_PIPELINE_REG_1),
		.pipeline_reg_2(INPUT_PIPELINE_REG_2),
		.pipeline_reg_3(INPUT_PIPELINE_REG_3),
		.pipeline_reg_4(INPUT_PIPELINE_REG_4),
		.pipeline_reg_5(INPUT_PIPELINE_REG_5),
		.pipeline_reg_6(INPUT_PIPELINE_REG_6),
		.pipeline_reg_7(INPUT_PIPELINE_REG_7),
		.pipeline_reg_8(INPUT_PIPELINE_REG_8),
		.pipeline_reg_9(INPUT_PIPELINE_REG_9),
		.pipeline_reg_10(INPUT_PIPELINE_REG_10),
		.pipeline_reg_11(INPUT_PIPELINE_REG_11),
		.pipeline_reg_12(INPUT_PIPELINE_REG_12),
		.pipeline_reg_13(INPUT_PIPELINE_REG_13),
		.pipeline_reg_14(INPUT_PIPELINE_REG_14),
		.pipeline_reg_15(INPUT_PIPELINE_REG_15),
		.pipeline_reg_16(INPUT_PIPELINE_REG_16),
		.reset(reset)	);
	defparam in_pipe.WIDTH = 18; // = dw
	////******************************************************
	// *
	// * Computation Pipeline
	// *
	// *****************************************************

	// ************************* LEVEL 0 ************************* \\
	wire [dw-1:0] L0_output_wires_0;
	wire [dw-1:0] L0_output_wires_1;
	wire [dw-1:0] L0_output_wires_2;
	wire [dw-1:0] L0_output_wires_3;
	wire [dw-1:0] L0_output_wires_4;
	wire [dw-1:0] L0_output_wires_5;
	wire [dw-1:0] L0_output_wires_6;
	wire [dw-1:0] L0_output_wires_7;
	wire [dw-1:0] L0_output_wires_8;

	adder_with_1_reg L0_adder_0and16(
		.dataa (INPUT_PIPELINE_REG_0),
		.datab (INPUT_PIPELINE_REG_16),
		.result(L0_output_wires_0)
	);

	adder_with_1_reg L0_adder_1and15(
		.dataa (INPUT_PIPELINE_REG_1),
		.datab (INPUT_PIPELINE_REG_15),
		.result(L0_output_wires_1)
	);

	adder_with_1_reg L0_adder_2and14(
		.dataa (INPUT_PIPELINE_REG_2),
		.datab (INPUT_PIPELINE_REG_14),
		.result(L0_output_wires_2)
	);

	adder_with_1_reg L0_adder_3and13(
		.dataa (INPUT_PIPELINE_REG_3),
		.datab (INPUT_PIPELINE_REG_13),
		.result(L0_output_wires_3)
	);

	adder_with_1_reg L0_adder_4and12(
		.dataa (INPUT_PIPELINE_REG_4),
		.datab (INPUT_PIPELINE_REG_12),
		.result(L0_output_wires_4)
	);

	adder_with_1_reg L0_adder_5and11(
		.dataa (INPUT_PIPELINE_REG_5),
		.datab (INPUT_PIPELINE_REG_11),
		.result(L0_output_wires_5)
	);

	adder_with_1_reg L0_adder_6and10(
		.dataa (INPUT_PIPELINE_REG_6),
		.datab (INPUT_PIPELINE_REG_10),
		.result(L0_output_wires_6)
	);

	adder_with_1_reg L0_adder_7and9(
		.dataa (INPUT_PIPELINE_REG_7),
		.datab (INPUT_PIPELINE_REG_9),
		.result(L0_output_wires_7)
	);

	// (8 main tree Adders)

	// ********* Byes ******** \\
	one_register L0_byereg_for_8(
		.dataa (INPUT_PIPELINE_REG_8),
		.result(L0_output_wires_8)
	);

	// (1 byes)

	// ************************* LEVEL 1 ************************* \\
	// **************** Multipliers **************** \\
	wire [dw-1:0] L1_mult_wires_0;
	wire [dw-1:0] L1_mult_wires_1;
	wire [dw-1:0] L1_mult_wires_2;
	wire [dw-1:0] L1_mult_wires_3;
	wire [dw-1:0] L1_mult_wires_4;
	wire [dw-1:0] L1_mult_wires_5;
	wire [dw-1:0] L1_mult_wires_6;
	wire [dw-1:0] L1_mult_wires_7;
	wire [dw-1:0] L1_mult_wires_8;

	multiplier_with_reg L1_mul_0(
		.dataa (L0_output_wires_0),
		.datab (COEFFICIENT_0),
		.result(L1_mult_wires_0)
	);

	multiplier_with_reg L1_mul_1(
		.dataa (L0_output_wires_1),
		.datab (COEFFICIENT_1),
		.result(L1_mult_wires_1)
	);

	multiplier_with_reg L1_mul_2(
		.dataa (L0_output_wires_2),
		.datab (COEFFICIENT_2),
		.result(L1_mult_wires_2)
	);

	multiplier_with_reg L1_mul_3(
		.dataa (L0_output_wires_3),
		.datab (COEFFICIENT_3),
		.result(L1_mult_wires_3)
	);

	multiplier_with_reg L1_mul_4(
		.dataa (L0_output_wires_4),
		.datab (COEFFICIENT_4),
		.result(L1_mult_wires_4)
	);

	multiplier_with_reg L1_mul_5(
		.dataa (L0_output_wires_5),
		.datab (COEFFICIENT_5),
		.result(L1_mult_wires_5)
	);

	multiplier_with_reg L1_mul_6(
		.dataa (L0_output_wires_6),
		.datab (COEFFICIENT_6),
		.result(L1_mult_wires_6)
	);

	multiplier_with_reg L1_mul_7(
		.dataa (L0_output_wires_7),
		.datab (COEFFICIENT_7),
		.result(L1_mult_wires_7)
	);

	multiplier_with_reg L1_mul_8(
		.dataa (L0_output_wires_8),
		.datab (COEFFICIENT_8),
		.result(L1_mult_wires_8)
	);

	// (9 Multipliers)

	// **************** Adders **************** \\
	wire [dw-1:0] L1_output_wires_0;
	wire [dw-1:0] L1_output_wires_1;
	wire [dw-1:0] L1_output_wires_2;
	wire [dw-1:0] L1_output_wires_3;
	wire [dw-1:0] L1_output_wires_4;

	adder_with_1_reg L1_adder_0and1(
		.dataa (L1_mult_wires_0),
		.datab (L1_mult_wires_1),
		.result(L1_output_wires_0)
	);

	adder_with_1_reg L1_adder_2and3(
		.dataa (L1_mult_wires_2),
		.datab (L1_mult_wires_3),
		.result(L1_output_wires_1)
	);

	adder_with_1_reg L1_adder_4and5(
		.dataa (L1_mult_wires_4),
		.datab (L1_mult_wires_5),
		.result(L1_output_wires_2)
	);

	adder_with_1_reg L1_adder_6and7(
		.dataa (L1_mult_wires_6),
		.datab (L1_mult_wires_7),
		.result(L1_output_wires_3)
	);

	// (4 main tree Adders)

	// ********* Byes ******** \\
	one_register L1_byereg_for_8(
		.dataa (L1_mult_wires_8),
		.result(L1_output_wires_4)
	);

	// (1 byes)

	// ************************* LEVEL 2 ************************* \\
	wire [dw-1:0] L2_output_wires_0;
	wire [dw-1:0] L2_output_wires_1;
	wire [dw-1:0] L2_output_wires_2;

	adder_with_1_reg L2_adder_0and1(
		.dataa (L1_output_wires_0),
		.datab (L1_output_wires_1),
		.result(L2_output_wires_0)
	);

	adder_with_1_reg L2_adder_2and3(
		.dataa (L1_output_wires_2),
		.datab (L1_output_wires_3),
		.result(L2_output_wires_1)
	);

	// (2 main tree Adders)

	// ********* Byes ******** \\
	one_register L2_byereg_for_4(
		.dataa (L1_output_wires_4),
		.result(L2_output_wires_2)
	);

	// (1 byes)

	// ************************* LEVEL 3 ************************* \\
	wire [dw-1:0] L3_output_wires_0;
	wire [dw-1:0] L3_output_wires_1;

	adder_with_1_reg L3_adder_0and1(
		.dataa (L2_output_wires_0),
		.datab (L2_output_wires_1),
		.result(L3_output_wires_0)
	);

	// (1 main tree Adders)

	// ********* Byes ******** \\
	one_register L3_byereg_for_2(
		.dataa (L2_output_wires_2),
		.result(L3_output_wires_1)
	);

	// (1 byes)

	// ************************* LEVEL 4 ************************* \\
	wire [dw-1:0] L4_output_wires_0;

	adder_with_1_reg L4_adder_0and1(
		.dataa (L3_output_wires_0),
		.datab (L3_output_wires_1),
		.result(L4_output_wires_0)
	);

	// (1 main tree Adders)

	////******************************************************
	// *
	// * Output Logic
	// *
	// *****************************************************
	//Actual outputs
	reg     [17:0]  o_out;

	always @(posedge clk) begin
		if(clk_ena) begin
			o_out <= L4_output_wires_0;
		end
	end

	assign o_valid = VALID_PIPELINE_REGS[N_VALID_REGS-1];

endmodule


module input_pipeline (
	clk,
	clk_ena,
	in_stream,
	pipeline_reg_0,
	pipeline_reg_1,
	pipeline_reg_2,
	pipeline_reg_3,
	pipeline_reg_4,
	pipeline_reg_5,
	pipeline_reg_6,
	pipeline_reg_7,
	pipeline_reg_8,
	pipeline_reg_9,
	pipeline_reg_10,
	pipeline_reg_11,
	pipeline_reg_12,
	pipeline_reg_13,
	pipeline_reg_14,
	pipeline_reg_15,
	pipeline_reg_16,
	reset);
	parameter WIDTH = 1;
	//Input value registers
	input clk;
	input clk_ena;
	input [WIDTH-1:0] in_stream;
	output [WIDTH-1:0] pipeline_reg_0;
	output [WIDTH-1:0] pipeline_reg_1;
	output [WIDTH-1:0] pipeline_reg_2;
	output [WIDTH-1:0] pipeline_reg_3;
	output [WIDTH-1:0] pipeline_reg_4;
	output [WIDTH-1:0] pipeline_reg_5;
	output [WIDTH-1:0] pipeline_reg_6;
	output [WIDTH-1:0] pipeline_reg_7;
	output [WIDTH-1:0] pipeline_reg_8;
	output [WIDTH-1:0] pipeline_reg_9;
	output [WIDTH-1:0] pipeline_reg_10;
	output [WIDTH-1:0] pipeline_reg_11;
	output [WIDTH-1:0] pipeline_reg_12;
	output [WIDTH-1:0] pipeline_reg_13;
	output [WIDTH-1:0] pipeline_reg_14;
	output [WIDTH-1:0] pipeline_reg_15;
	output [WIDTH-1:0] pipeline_reg_16;
	reg [WIDTH-1:0] pipeline_reg_0;
	reg [WIDTH-1:0] pipeline_reg_1;
	reg [WIDTH-1:0] pipeline_reg_2;
	reg [WIDTH-1:0] pipeline_reg_3;
	reg [WIDTH-1:0] pipeline_reg_4;
	reg [WIDTH-1:0] pipeline_reg_5;
	reg [WIDTH-1:0] pipeline_reg_6;
	reg [WIDTH-1:0] pipeline_reg_7;
	reg [WIDTH-1:0] pipeline_reg_8;
	reg [WIDTH-1:0] pipeline_reg_9;
	reg [WIDTH-1:0] pipeline_reg_10;
	reg [WIDTH-1:0] pipeline_reg_11;
	reg [WIDTH-1:0] pipeline_reg_12;
	reg [WIDTH-1:0] pipeline_reg_13;
	reg [WIDTH-1:0] pipeline_reg_14;
	reg [WIDTH-1:0] pipeline_reg_15;
	reg [WIDTH-1:0] pipeline_reg_16;
	input reset;

	always@(posedge clk or posedge reset) begin
		if(reset) begin
			pipeline_reg_0 <= 0;
			pipeline_reg_1 <= 0;
			pipeline_reg_2 <= 0;
			pipeline_reg_3 <= 0;
			pipeline_reg_4 <= 0;
			pipeline_reg_5 <= 0;
			pipeline_reg_6 <= 0;
			pipeline_reg_7 <= 0;
			pipeline_reg_8 <= 0;
			pipeline_reg_9 <= 0;
			pipeline_reg_10 <= 0;
			pipeline_reg_11 <= 0;
			pipeline_reg_12 <= 0;
			pipeline_reg_13 <= 0;
			pipeline_reg_14 <= 0;
			pipeline_reg_15 <= 0;
			pipeline_reg_16 <= 0;
		end else begin
			if(clk_ena) begin
				pipeline_reg_0 <= in_stream;
				pipeline_reg_1 <= pipeline_reg_0;
				pipeline_reg_2 <= pipeline_reg_1;
				pipeline_reg_3 <= pipeline_reg_2;
				pipeline_reg_4 <= pipeline_reg_3;
				pipeline_reg_5 <= pipeline_reg_4;
				pipeline_reg_6 <= pipeline_reg_5;
				pipeline_reg_7 <= pipeline_reg_6;
				pipeline_reg_8 <= pipeline_reg_7;
				pipeline_reg_9 <= pipeline_reg_8;
				pipeline_reg_10 <= pipeline_reg_9;
				pipeline_reg_11 <= pipeline_reg_10;
				pipeline_reg_12 <= pipeline_reg_11;
				pipeline_reg_13 <= pipeline_reg_12;
				pipeline_reg_14 <= pipeline_reg_13;
				pipeline_reg_15 <= pipeline_reg_14;
				pipeline_reg_16 <= pipeline_reg_15;
			end //else begin
				//pipeline_reg_0 <= pipeline_reg_0;
				//pipeline_reg_1 <= pipeline_reg_1;
				//pipeline_reg_2 <= pipeline_reg_2;
				//pipeline_reg_3 <= pipeline_reg_3;
				//pipeline_reg_4 <= pipeline_reg_4;
				//pipeline_reg_5 <= pipeline_reg_5;
				//pipeline_reg_6 <= pipeline_reg_6;
				//pipeline_reg_7 <= pipeline_reg_7;
				//pipeline_reg_8 <= pipeline_reg_8;
				//pipeline_reg_9 <= pipeline_reg_9;
				//pipeline_reg_10 <= pipeline_reg_10;
				//pipeline_reg_11 <= pipeline_reg_11;
				//pipeline_reg_12 <= pipeline_reg_12;
				//pipeline_reg_13 <= pipeline_reg_13;
				//pipeline_reg_14 <= pipeline_reg_14;
				//pipeline_reg_15 <= pipeline_reg_15;
				//pipeline_reg_16 <= pipeline_reg_16;
			//end
		end
	end
endmodule


module adder_with_1_reg (
	dataa,
	datab,
	result);

	input	  clk;
	input	  clk_ena;
	input	[17:0]  dataa;
	input	[17:0]  datab;
	output	[17:0]  result;

	assign result = dataa + datab;

endmodule


module multiplier_with_reg (
	dataa,
	datab,
	result);

	input	  clk;
	input	  clk_ena;
	input	[17:0]  dataa;
	input	[17:0]  datab;
	output	[17:0]  result;

	assign result = dataa * datab;

endmodule


module one_register (
	dataa,
	result);

	input	  clk;
	input	  clk_ena;
	input	[17:0]  dataa;
	output	[17:0]  result;

	assign result = dataa;

endmodule


