// CONFIG:
// NUM_COEFF = 39
// PIPLINED = 1

// WARNING: more than enough COEFFICIENTS in array (there are 26, and we only need 20)
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
	parameter N = 39;
	parameter N_UNIQ = 20; // ciel(N/2) assuming symmetric filter coefficients

	//Number of extra valid cycles needed to align output (i.e. computation pipeline depth + input/output registers
	localparam N_VALID_REGS = 46;

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
	localparam N_INPUT_REGS = 39;

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
	reg [dw-1:0] COEFFICIENT_9;
	reg [dw-1:0] COEFFICIENT_10;
	reg [dw-1:0] COEFFICIENT_11;
	reg [dw-1:0] COEFFICIENT_12;
	reg [dw-1:0] COEFFICIENT_13;
	reg [dw-1:0] COEFFICIENT_14;
	reg [dw-1:0] COEFFICIENT_15;
	reg [dw-1:0] COEFFICIENT_16;
	reg [dw-1:0] COEFFICIENT_17;
	reg [dw-1:0] COEFFICIENT_18;
	reg [dw-1:0] COEFFICIENT_19;

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
		COEFFICIENT_9 <= -18'd481;
		COEFFICIENT_10 <= -18'd420;
		COEFFICIENT_11 <= -18'd319;
		COEFFICIENT_12 <= -18'd178;
		COEFFICIENT_13 <= 18'd0;
		COEFFICIENT_14 <= 18'd212;
		COEFFICIENT_15 <= 18'd451;
		COEFFICIENT_16 <= 18'd710;
		COEFFICIENT_17 <= 18'd980;
		COEFFICIENT_18 <= 18'd1252;
		COEFFICIENT_19 <= 18'd1514;
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
	wire [dw-1:0] INPUT_PIPELINE_REG_17;
	wire [dw-1:0] INPUT_PIPELINE_REG_18;
	wire [dw-1:0] INPUT_PIPELINE_REG_19;
	wire [dw-1:0] INPUT_PIPELINE_REG_20;
	wire [dw-1:0] INPUT_PIPELINE_REG_21;
	wire [dw-1:0] INPUT_PIPELINE_REG_22;
	wire [dw-1:0] INPUT_PIPELINE_REG_23;
	wire [dw-1:0] INPUT_PIPELINE_REG_24;
	wire [dw-1:0] INPUT_PIPELINE_REG_25;
	wire [dw-1:0] INPUT_PIPELINE_REG_26;
	wire [dw-1:0] INPUT_PIPELINE_REG_27;
	wire [dw-1:0] INPUT_PIPELINE_REG_28;
	wire [dw-1:0] INPUT_PIPELINE_REG_29;
	wire [dw-1:0] INPUT_PIPELINE_REG_30;
	wire [dw-1:0] INPUT_PIPELINE_REG_31;
	wire [dw-1:0] INPUT_PIPELINE_REG_32;
	wire [dw-1:0] INPUT_PIPELINE_REG_33;
	wire [dw-1:0] INPUT_PIPELINE_REG_34;
	wire [dw-1:0] INPUT_PIPELINE_REG_35;
	wire [dw-1:0] INPUT_PIPELINE_REG_36;
	wire [dw-1:0] INPUT_PIPELINE_REG_37;
	wire [dw-1:0] INPUT_PIPELINE_REG_38;

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
		.pipeline_reg_17(INPUT_PIPELINE_REG_17),
		.pipeline_reg_18(INPUT_PIPELINE_REG_18),
		.pipeline_reg_19(INPUT_PIPELINE_REG_19),
		.pipeline_reg_20(INPUT_PIPELINE_REG_20),
		.pipeline_reg_21(INPUT_PIPELINE_REG_21),
		.pipeline_reg_22(INPUT_PIPELINE_REG_22),
		.pipeline_reg_23(INPUT_PIPELINE_REG_23),
		.pipeline_reg_24(INPUT_PIPELINE_REG_24),
		.pipeline_reg_25(INPUT_PIPELINE_REG_25),
		.pipeline_reg_26(INPUT_PIPELINE_REG_26),
		.pipeline_reg_27(INPUT_PIPELINE_REG_27),
		.pipeline_reg_28(INPUT_PIPELINE_REG_28),
		.pipeline_reg_29(INPUT_PIPELINE_REG_29),
		.pipeline_reg_30(INPUT_PIPELINE_REG_30),
		.pipeline_reg_31(INPUT_PIPELINE_REG_31),
		.pipeline_reg_32(INPUT_PIPELINE_REG_32),
		.pipeline_reg_33(INPUT_PIPELINE_REG_33),
		.pipeline_reg_34(INPUT_PIPELINE_REG_34),
		.pipeline_reg_35(INPUT_PIPELINE_REG_35),
		.pipeline_reg_36(INPUT_PIPELINE_REG_36),
		.pipeline_reg_37(INPUT_PIPELINE_REG_37),
		.pipeline_reg_38(INPUT_PIPELINE_REG_38),
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
	wire [dw-1:0] L0_output_wires_9;
	wire [dw-1:0] L0_output_wires_10;
	wire [dw-1:0] L0_output_wires_11;
	wire [dw-1:0] L0_output_wires_12;
	wire [dw-1:0] L0_output_wires_13;
	wire [dw-1:0] L0_output_wires_14;
	wire [dw-1:0] L0_output_wires_15;
	wire [dw-1:0] L0_output_wires_16;
	wire [dw-1:0] L0_output_wires_17;
	wire [dw-1:0] L0_output_wires_18;
	wire [dw-1:0] L0_output_wires_19;

	adder_with_1_reg L0_adder_0and38(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_0),
		.datab (INPUT_PIPELINE_REG_38),
		.result(L0_output_wires_0)
	);

	adder_with_1_reg L0_adder_1and37(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_1),
		.datab (INPUT_PIPELINE_REG_37),
		.result(L0_output_wires_1)
	);

	adder_with_1_reg L0_adder_2and36(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_2),
		.datab (INPUT_PIPELINE_REG_36),
		.result(L0_output_wires_2)
	);

	adder_with_1_reg L0_adder_3and35(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_3),
		.datab (INPUT_PIPELINE_REG_35),
		.result(L0_output_wires_3)
	);

	adder_with_1_reg L0_adder_4and34(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_4),
		.datab (INPUT_PIPELINE_REG_34),
		.result(L0_output_wires_4)
	);

	adder_with_1_reg L0_adder_5and33(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_5),
		.datab (INPUT_PIPELINE_REG_33),
		.result(L0_output_wires_5)
	);

	adder_with_1_reg L0_adder_6and32(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_6),
		.datab (INPUT_PIPELINE_REG_32),
		.result(L0_output_wires_6)
	);

	adder_with_1_reg L0_adder_7and31(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_7),
		.datab (INPUT_PIPELINE_REG_31),
		.result(L0_output_wires_7)
	);

	adder_with_1_reg L0_adder_8and30(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_8),
		.datab (INPUT_PIPELINE_REG_30),
		.result(L0_output_wires_8)
	);

	adder_with_1_reg L0_adder_9and29(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_9),
		.datab (INPUT_PIPELINE_REG_29),
		.result(L0_output_wires_9)
	);

	adder_with_1_reg L0_adder_10and28(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_10),
		.datab (INPUT_PIPELINE_REG_28),
		.result(L0_output_wires_10)
	);

	adder_with_1_reg L0_adder_11and27(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_11),
		.datab (INPUT_PIPELINE_REG_27),
		.result(L0_output_wires_11)
	);

	adder_with_1_reg L0_adder_12and26(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_12),
		.datab (INPUT_PIPELINE_REG_26),
		.result(L0_output_wires_12)
	);

	adder_with_1_reg L0_adder_13and25(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_13),
		.datab (INPUT_PIPELINE_REG_25),
		.result(L0_output_wires_13)
	);

	adder_with_1_reg L0_adder_14and24(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_14),
		.datab (INPUT_PIPELINE_REG_24),
		.result(L0_output_wires_14)
	);

	adder_with_1_reg L0_adder_15and23(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_15),
		.datab (INPUT_PIPELINE_REG_23),
		.result(L0_output_wires_15)
	);

	adder_with_1_reg L0_adder_16and22(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_16),
		.datab (INPUT_PIPELINE_REG_22),
		.result(L0_output_wires_16)
	);

	adder_with_1_reg L0_adder_17and21(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_17),
		.datab (INPUT_PIPELINE_REG_21),
		.result(L0_output_wires_17)
	);

	adder_with_1_reg L0_adder_18and20(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_18),
		.datab (INPUT_PIPELINE_REG_20),
		.result(L0_output_wires_18)
	);

	// (19 main tree Adders)

	// ********* Byes ******** \\
	one_register L0_byereg_for_19(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (INPUT_PIPELINE_REG_19),
		.result(L0_output_wires_19)
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
	wire [dw-1:0] L1_mult_wires_9;
	wire [dw-1:0] L1_mult_wires_10;
	wire [dw-1:0] L1_mult_wires_11;
	wire [dw-1:0] L1_mult_wires_12;
	wire [dw-1:0] L1_mult_wires_13;
	wire [dw-1:0] L1_mult_wires_14;
	wire [dw-1:0] L1_mult_wires_15;
	wire [dw-1:0] L1_mult_wires_16;
	wire [dw-1:0] L1_mult_wires_17;
	wire [dw-1:0] L1_mult_wires_18;
	wire [dw-1:0] L1_mult_wires_19;

	multiplier_with_reg L1_mul_0(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_0),
		.datab (COEFFICIENT_0),
		.result(L1_mult_wires_0)
	);

	multiplier_with_reg L1_mul_1(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_1),
		.datab (COEFFICIENT_1),
		.result(L1_mult_wires_1)
	);

	multiplier_with_reg L1_mul_2(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_2),
		.datab (COEFFICIENT_2),
		.result(L1_mult_wires_2)
	);

	multiplier_with_reg L1_mul_3(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_3),
		.datab (COEFFICIENT_3),
		.result(L1_mult_wires_3)
	);

	multiplier_with_reg L1_mul_4(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_4),
		.datab (COEFFICIENT_4),
		.result(L1_mult_wires_4)
	);

	multiplier_with_reg L1_mul_5(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_5),
		.datab (COEFFICIENT_5),
		.result(L1_mult_wires_5)
	);

	multiplier_with_reg L1_mul_6(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_6),
		.datab (COEFFICIENT_6),
		.result(L1_mult_wires_6)
	);

	multiplier_with_reg L1_mul_7(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_7),
		.datab (COEFFICIENT_7),
		.result(L1_mult_wires_7)
	);

	multiplier_with_reg L1_mul_8(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_8),
		.datab (COEFFICIENT_8),
		.result(L1_mult_wires_8)
	);

	multiplier_with_reg L1_mul_9(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_9),
		.datab (COEFFICIENT_9),
		.result(L1_mult_wires_9)
	);

	multiplier_with_reg L1_mul_10(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_10),
		.datab (COEFFICIENT_10),
		.result(L1_mult_wires_10)
	);

	multiplier_with_reg L1_mul_11(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_11),
		.datab (COEFFICIENT_11),
		.result(L1_mult_wires_11)
	);

	multiplier_with_reg L1_mul_12(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_12),
		.datab (COEFFICIENT_12),
		.result(L1_mult_wires_12)
	);

	multiplier_with_reg L1_mul_13(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_13),
		.datab (COEFFICIENT_13),
		.result(L1_mult_wires_13)
	);

	multiplier_with_reg L1_mul_14(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_14),
		.datab (COEFFICIENT_14),
		.result(L1_mult_wires_14)
	);

	multiplier_with_reg L1_mul_15(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_15),
		.datab (COEFFICIENT_15),
		.result(L1_mult_wires_15)
	);

	multiplier_with_reg L1_mul_16(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_16),
		.datab (COEFFICIENT_16),
		.result(L1_mult_wires_16)
	);

	multiplier_with_reg L1_mul_17(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_17),
		.datab (COEFFICIENT_17),
		.result(L1_mult_wires_17)
	);

	multiplier_with_reg L1_mul_18(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_18),
		.datab (COEFFICIENT_18),
		.result(L1_mult_wires_18)
	);

	multiplier_with_reg L1_mul_19(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L0_output_wires_19),
		.datab (COEFFICIENT_19),
		.result(L1_mult_wires_19)
	);

	// (20 Multipliers)

	// **************** Adders **************** \\
	wire [dw-1:0] L1_output_wires_0;
	wire [dw-1:0] L1_output_wires_1;
	wire [dw-1:0] L1_output_wires_2;
	wire [dw-1:0] L1_output_wires_3;
	wire [dw-1:0] L1_output_wires_4;
	wire [dw-1:0] L1_output_wires_5;
	wire [dw-1:0] L1_output_wires_6;
	wire [dw-1:0] L1_output_wires_7;
	wire [dw-1:0] L1_output_wires_8;
	wire [dw-1:0] L1_output_wires_9;

	adder_with_1_reg L1_adder_0and1(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_0),
		.datab (L1_mult_wires_1),
		.result(L1_output_wires_0)
	);

	adder_with_1_reg L1_adder_2and3(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_2),
		.datab (L1_mult_wires_3),
		.result(L1_output_wires_1)
	);

	adder_with_1_reg L1_adder_4and5(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_4),
		.datab (L1_mult_wires_5),
		.result(L1_output_wires_2)
	);

	adder_with_1_reg L1_adder_6and7(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_6),
		.datab (L1_mult_wires_7),
		.result(L1_output_wires_3)
	);

	adder_with_1_reg L1_adder_8and9(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_8),
		.datab (L1_mult_wires_9),
		.result(L1_output_wires_4)
	);

	adder_with_1_reg L1_adder_10and11(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_10),
		.datab (L1_mult_wires_11),
		.result(L1_output_wires_5)
	);

	adder_with_1_reg L1_adder_12and13(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_12),
		.datab (L1_mult_wires_13),
		.result(L1_output_wires_6)
	);

	adder_with_1_reg L1_adder_14and15(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_14),
		.datab (L1_mult_wires_15),
		.result(L1_output_wires_7)
	);

	adder_with_1_reg L1_adder_16and17(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_16),
		.datab (L1_mult_wires_17),
		.result(L1_output_wires_8)
	);

	adder_with_1_reg L1_adder_18and19(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_mult_wires_18),
		.datab (L1_mult_wires_19),
		.result(L1_output_wires_9)
	);

	// (10 main tree Adders)

	// ************************* LEVEL 2 ************************* \\
	wire [dw-1:0] L2_output_wires_0;
	wire [dw-1:0] L2_output_wires_1;
	wire [dw-1:0] L2_output_wires_2;
	wire [dw-1:0] L2_output_wires_3;
	wire [dw-1:0] L2_output_wires_4;

	adder_with_1_reg L2_adder_0and1(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_output_wires_0),
		.datab (L1_output_wires_1),
		.result(L2_output_wires_0)
	);

	adder_with_1_reg L2_adder_2and3(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_output_wires_2),
		.datab (L1_output_wires_3),
		.result(L2_output_wires_1)
	);

	adder_with_1_reg L2_adder_4and5(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_output_wires_4),
		.datab (L1_output_wires_5),
		.result(L2_output_wires_2)
	);

	adder_with_1_reg L2_adder_6and7(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_output_wires_6),
		.datab (L1_output_wires_7),
		.result(L2_output_wires_3)
	);

	adder_with_1_reg L2_adder_8and9(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L1_output_wires_8),
		.datab (L1_output_wires_9),
		.result(L2_output_wires_4)
	);

	// (5 main tree Adders)

	// ************************* LEVEL 3 ************************* \\
	wire [dw-1:0] L3_output_wires_0;
	wire [dw-1:0] L3_output_wires_1;
	wire [dw-1:0] L3_output_wires_2;

	adder_with_1_reg L3_adder_0and1(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L2_output_wires_0),
		.datab (L2_output_wires_1),
		.result(L3_output_wires_0)
	);

	adder_with_1_reg L3_adder_2and3(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L2_output_wires_2),
		.datab (L2_output_wires_3),
		.result(L3_output_wires_1)
	);

	// (2 main tree Adders)

	// ********* Byes ******** \\
	one_register L3_byereg_for_4(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L2_output_wires_4),
		.result(L3_output_wires_2)
	);

	// (1 byes)

	// ************************* LEVEL 4 ************************* \\
	wire [dw-1:0] L4_output_wires_0;
	wire [dw-1:0] L4_output_wires_1;

	adder_with_1_reg L4_adder_0and1(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L3_output_wires_0),
		.datab (L3_output_wires_1),
		.result(L4_output_wires_0)
	);

	// (1 main tree Adders)

	// ********* Byes ******** \\
	one_register L4_byereg_for_2(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L3_output_wires_2),
		.result(L4_output_wires_1)
	);

	// (1 byes)

	// ************************* LEVEL 5 ************************* \\
	wire [dw-1:0] L5_output_wires_0;

	adder_with_1_reg L5_adder_0and1(
		.clk(clk), .clk_ena(clk_ena),
		.dataa (L4_output_wires_0),
		.datab (L4_output_wires_1),
		.result(L5_output_wires_0)
	);

	// (1 main tree Adders)

	////******************************************************
	// *
	// * Output Logic
	// *
	// *****************************************************
	//Actual outputs
	assign o_out = L5_output_wires_0;

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
	pipeline_reg_17,
	pipeline_reg_18,
	pipeline_reg_19,
	pipeline_reg_20,
	pipeline_reg_21,
	pipeline_reg_22,
	pipeline_reg_23,
	pipeline_reg_24,
	pipeline_reg_25,
	pipeline_reg_26,
	pipeline_reg_27,
	pipeline_reg_28,
	pipeline_reg_29,
	pipeline_reg_30,
	pipeline_reg_31,
	pipeline_reg_32,
	pipeline_reg_33,
	pipeline_reg_34,
	pipeline_reg_35,
	pipeline_reg_36,
	pipeline_reg_37,
	pipeline_reg_38,
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
	output [WIDTH-1:0] pipeline_reg_17;
	output [WIDTH-1:0] pipeline_reg_18;
	output [WIDTH-1:0] pipeline_reg_19;
	output [WIDTH-1:0] pipeline_reg_20;
	output [WIDTH-1:0] pipeline_reg_21;
	output [WIDTH-1:0] pipeline_reg_22;
	output [WIDTH-1:0] pipeline_reg_23;
	output [WIDTH-1:0] pipeline_reg_24;
	output [WIDTH-1:0] pipeline_reg_25;
	output [WIDTH-1:0] pipeline_reg_26;
	output [WIDTH-1:0] pipeline_reg_27;
	output [WIDTH-1:0] pipeline_reg_28;
	output [WIDTH-1:0] pipeline_reg_29;
	output [WIDTH-1:0] pipeline_reg_30;
	output [WIDTH-1:0] pipeline_reg_31;
	output [WIDTH-1:0] pipeline_reg_32;
	output [WIDTH-1:0] pipeline_reg_33;
	output [WIDTH-1:0] pipeline_reg_34;
	output [WIDTH-1:0] pipeline_reg_35;
	output [WIDTH-1:0] pipeline_reg_36;
	output [WIDTH-1:0] pipeline_reg_37;
	output [WIDTH-1:0] pipeline_reg_38;
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
	reg [WIDTH-1:0] pipeline_reg_17;
	reg [WIDTH-1:0] pipeline_reg_18;
	reg [WIDTH-1:0] pipeline_reg_19;
	reg [WIDTH-1:0] pipeline_reg_20;
	reg [WIDTH-1:0] pipeline_reg_21;
	reg [WIDTH-1:0] pipeline_reg_22;
	reg [WIDTH-1:0] pipeline_reg_23;
	reg [WIDTH-1:0] pipeline_reg_24;
	reg [WIDTH-1:0] pipeline_reg_25;
	reg [WIDTH-1:0] pipeline_reg_26;
	reg [WIDTH-1:0] pipeline_reg_27;
	reg [WIDTH-1:0] pipeline_reg_28;
	reg [WIDTH-1:0] pipeline_reg_29;
	reg [WIDTH-1:0] pipeline_reg_30;
	reg [WIDTH-1:0] pipeline_reg_31;
	reg [WIDTH-1:0] pipeline_reg_32;
	reg [WIDTH-1:0] pipeline_reg_33;
	reg [WIDTH-1:0] pipeline_reg_34;
	reg [WIDTH-1:0] pipeline_reg_35;
	reg [WIDTH-1:0] pipeline_reg_36;
	reg [WIDTH-1:0] pipeline_reg_37;
	reg [WIDTH-1:0] pipeline_reg_38;
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
			pipeline_reg_17 <= 0;
			pipeline_reg_18 <= 0;
			pipeline_reg_19 <= 0;
			pipeline_reg_20 <= 0;
			pipeline_reg_21 <= 0;
			pipeline_reg_22 <= 0;
			pipeline_reg_23 <= 0;
			pipeline_reg_24 <= 0;
			pipeline_reg_25 <= 0;
			pipeline_reg_26 <= 0;
			pipeline_reg_27 <= 0;
			pipeline_reg_28 <= 0;
			pipeline_reg_29 <= 0;
			pipeline_reg_30 <= 0;
			pipeline_reg_31 <= 0;
			pipeline_reg_32 <= 0;
			pipeline_reg_33 <= 0;
			pipeline_reg_34 <= 0;
			pipeline_reg_35 <= 0;
			pipeline_reg_36 <= 0;
			pipeline_reg_37 <= 0;
			pipeline_reg_38 <= 0;
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
				pipeline_reg_17 <= pipeline_reg_16;
				pipeline_reg_18 <= pipeline_reg_17;
				pipeline_reg_19 <= pipeline_reg_18;
				pipeline_reg_20 <= pipeline_reg_19;
				pipeline_reg_21 <= pipeline_reg_20;
				pipeline_reg_22 <= pipeline_reg_21;
				pipeline_reg_23 <= pipeline_reg_22;
				pipeline_reg_24 <= pipeline_reg_23;
				pipeline_reg_25 <= pipeline_reg_24;
				pipeline_reg_26 <= pipeline_reg_25;
				pipeline_reg_27 <= pipeline_reg_26;
				pipeline_reg_28 <= pipeline_reg_27;
				pipeline_reg_29 <= pipeline_reg_28;
				pipeline_reg_30 <= pipeline_reg_29;
				pipeline_reg_31 <= pipeline_reg_30;
				pipeline_reg_32 <= pipeline_reg_31;
				pipeline_reg_33 <= pipeline_reg_32;
				pipeline_reg_34 <= pipeline_reg_33;
				pipeline_reg_35 <= pipeline_reg_34;
				pipeline_reg_36 <= pipeline_reg_35;
				pipeline_reg_37 <= pipeline_reg_36;
				pipeline_reg_38 <= pipeline_reg_37;
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
				//pipeline_reg_17 <= pipeline_reg_17;
				//pipeline_reg_18 <= pipeline_reg_18;
				//pipeline_reg_19 <= pipeline_reg_19;
				//pipeline_reg_20 <= pipeline_reg_20;
				//pipeline_reg_21 <= pipeline_reg_21;
				//pipeline_reg_22 <= pipeline_reg_22;
				//pipeline_reg_23 <= pipeline_reg_23;
				//pipeline_reg_24 <= pipeline_reg_24;
				//pipeline_reg_25 <= pipeline_reg_25;
				//pipeline_reg_26 <= pipeline_reg_26;
				//pipeline_reg_27 <= pipeline_reg_27;
				//pipeline_reg_28 <= pipeline_reg_28;
				//pipeline_reg_29 <= pipeline_reg_29;
				//pipeline_reg_30 <= pipeline_reg_30;
				//pipeline_reg_31 <= pipeline_reg_31;
				//pipeline_reg_32 <= pipeline_reg_32;
				//pipeline_reg_33 <= pipeline_reg_33;
				//pipeline_reg_34 <= pipeline_reg_34;
				//pipeline_reg_35 <= pipeline_reg_35;
				//pipeline_reg_36 <= pipeline_reg_36;
				//pipeline_reg_37 <= pipeline_reg_37;
				//pipeline_reg_38 <= pipeline_reg_38;
			//end
		end
	end
endmodule


module adder_with_1_reg (
	clk,
	clk_ena,
	dataa,
	datab,
	result);

	input	  clk;
	input	  clk_ena;
	input	[17:0]  dataa;
	input	[17:0]  datab;
	output	[17:0]  result;

	reg     [17:0]  result;

	always @(posedge clk) begin
		if(clk_ena) begin
			result <= dataa + datab;
		end
	end

endmodule


module multiplier_with_reg (
	clk,
	clk_ena,
	dataa,
	datab,
	result);

	input	  clk;
	input	  clk_ena;
	input	[17:0]  dataa;
	input	[17:0]  datab;
	output	[17:0]  result;

	reg     [17:0]  result;

	always @(posedge clk) begin
		if(clk_ena) begin
			result <= dataa * datab;
		end
	end

endmodule


module one_register (
	clk,
	clk_ena,
	dataa,
	result);

	input	  clk;
	input	  clk_ena;
	input	[17:0]  dataa;
	output	[17:0]  result;

	reg     [17:0]  result;

	always @(posedge clk) begin
		if(clk_ena) begin
			result <= dataa;
		end
	end

endmodule


