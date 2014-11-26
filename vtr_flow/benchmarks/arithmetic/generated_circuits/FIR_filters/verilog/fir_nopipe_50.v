// CONFIG:
// NUM_COEFF = 50
// PIPLINED = 0

// WARNING: more than enough COEFFICIENTS in array (there are 26, and we only need 25)
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
	parameter N = 50;
	parameter N_UNIQ = 25; // ciel(N/2) assuming symmetric filter coefficients

	//Number of extra valid cycles needed to align output (i.e. computation pipeline depth + input/output registers
	localparam N_VALID_REGS = 51;

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
	localparam N_INPUT_REGS = 50;

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
	reg [dw-1:0] COEFFICIENT_20;
	reg [dw-1:0] COEFFICIENT_21;
	reg [dw-1:0] COEFFICIENT_22;
	reg [dw-1:0] COEFFICIENT_23;
	reg [dw-1:0] COEFFICIENT_24;

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
		COEFFICIENT_20 <= 18'd1756;
		COEFFICIENT_21 <= 18'd1971;
		COEFFICIENT_22 <= 18'd2147;
		COEFFICIENT_23 <= 18'd2278;
		COEFFICIENT_24 <= 18'd2360;
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
	wire [dw-1:0] INPUT_PIPELINE_REG_39;
	wire [dw-1:0] INPUT_PIPELINE_REG_40;
	wire [dw-1:0] INPUT_PIPELINE_REG_41;
	wire [dw-1:0] INPUT_PIPELINE_REG_42;
	wire [dw-1:0] INPUT_PIPELINE_REG_43;
	wire [dw-1:0] INPUT_PIPELINE_REG_44;
	wire [dw-1:0] INPUT_PIPELINE_REG_45;
	wire [dw-1:0] INPUT_PIPELINE_REG_46;
	wire [dw-1:0] INPUT_PIPELINE_REG_47;
	wire [dw-1:0] INPUT_PIPELINE_REG_48;
	wire [dw-1:0] INPUT_PIPELINE_REG_49;

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
		.pipeline_reg_39(INPUT_PIPELINE_REG_39),
		.pipeline_reg_40(INPUT_PIPELINE_REG_40),
		.pipeline_reg_41(INPUT_PIPELINE_REG_41),
		.pipeline_reg_42(INPUT_PIPELINE_REG_42),
		.pipeline_reg_43(INPUT_PIPELINE_REG_43),
		.pipeline_reg_44(INPUT_PIPELINE_REG_44),
		.pipeline_reg_45(INPUT_PIPELINE_REG_45),
		.pipeline_reg_46(INPUT_PIPELINE_REG_46),
		.pipeline_reg_47(INPUT_PIPELINE_REG_47),
		.pipeline_reg_48(INPUT_PIPELINE_REG_48),
		.pipeline_reg_49(INPUT_PIPELINE_REG_49),
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
	wire [dw-1:0] L0_output_wires_20;
	wire [dw-1:0] L0_output_wires_21;
	wire [dw-1:0] L0_output_wires_22;
	wire [dw-1:0] L0_output_wires_23;
	wire [dw-1:0] L0_output_wires_24;

	adder_with_1_reg L0_adder_0and49(
		.dataa (INPUT_PIPELINE_REG_0),
		.datab (INPUT_PIPELINE_REG_49),
		.result(L0_output_wires_0)
	);

	adder_with_1_reg L0_adder_1and48(
		.dataa (INPUT_PIPELINE_REG_1),
		.datab (INPUT_PIPELINE_REG_48),
		.result(L0_output_wires_1)
	);

	adder_with_1_reg L0_adder_2and47(
		.dataa (INPUT_PIPELINE_REG_2),
		.datab (INPUT_PIPELINE_REG_47),
		.result(L0_output_wires_2)
	);

	adder_with_1_reg L0_adder_3and46(
		.dataa (INPUT_PIPELINE_REG_3),
		.datab (INPUT_PIPELINE_REG_46),
		.result(L0_output_wires_3)
	);

	adder_with_1_reg L0_adder_4and45(
		.dataa (INPUT_PIPELINE_REG_4),
		.datab (INPUT_PIPELINE_REG_45),
		.result(L0_output_wires_4)
	);

	adder_with_1_reg L0_adder_5and44(
		.dataa (INPUT_PIPELINE_REG_5),
		.datab (INPUT_PIPELINE_REG_44),
		.result(L0_output_wires_5)
	);

	adder_with_1_reg L0_adder_6and43(
		.dataa (INPUT_PIPELINE_REG_6),
		.datab (INPUT_PIPELINE_REG_43),
		.result(L0_output_wires_6)
	);

	adder_with_1_reg L0_adder_7and42(
		.dataa (INPUT_PIPELINE_REG_7),
		.datab (INPUT_PIPELINE_REG_42),
		.result(L0_output_wires_7)
	);

	adder_with_1_reg L0_adder_8and41(
		.dataa (INPUT_PIPELINE_REG_8),
		.datab (INPUT_PIPELINE_REG_41),
		.result(L0_output_wires_8)
	);

	adder_with_1_reg L0_adder_9and40(
		.dataa (INPUT_PIPELINE_REG_9),
		.datab (INPUT_PIPELINE_REG_40),
		.result(L0_output_wires_9)
	);

	adder_with_1_reg L0_adder_10and39(
		.dataa (INPUT_PIPELINE_REG_10),
		.datab (INPUT_PIPELINE_REG_39),
		.result(L0_output_wires_10)
	);

	adder_with_1_reg L0_adder_11and38(
		.dataa (INPUT_PIPELINE_REG_11),
		.datab (INPUT_PIPELINE_REG_38),
		.result(L0_output_wires_11)
	);

	adder_with_1_reg L0_adder_12and37(
		.dataa (INPUT_PIPELINE_REG_12),
		.datab (INPUT_PIPELINE_REG_37),
		.result(L0_output_wires_12)
	);

	adder_with_1_reg L0_adder_13and36(
		.dataa (INPUT_PIPELINE_REG_13),
		.datab (INPUT_PIPELINE_REG_36),
		.result(L0_output_wires_13)
	);

	adder_with_1_reg L0_adder_14and35(
		.dataa (INPUT_PIPELINE_REG_14),
		.datab (INPUT_PIPELINE_REG_35),
		.result(L0_output_wires_14)
	);

	adder_with_1_reg L0_adder_15and34(
		.dataa (INPUT_PIPELINE_REG_15),
		.datab (INPUT_PIPELINE_REG_34),
		.result(L0_output_wires_15)
	);

	adder_with_1_reg L0_adder_16and33(
		.dataa (INPUT_PIPELINE_REG_16),
		.datab (INPUT_PIPELINE_REG_33),
		.result(L0_output_wires_16)
	);

	adder_with_1_reg L0_adder_17and32(
		.dataa (INPUT_PIPELINE_REG_17),
		.datab (INPUT_PIPELINE_REG_32),
		.result(L0_output_wires_17)
	);

	adder_with_1_reg L0_adder_18and31(
		.dataa (INPUT_PIPELINE_REG_18),
		.datab (INPUT_PIPELINE_REG_31),
		.result(L0_output_wires_18)
	);

	adder_with_1_reg L0_adder_19and30(
		.dataa (INPUT_PIPELINE_REG_19),
		.datab (INPUT_PIPELINE_REG_30),
		.result(L0_output_wires_19)
	);

	adder_with_1_reg L0_adder_20and29(
		.dataa (INPUT_PIPELINE_REG_20),
		.datab (INPUT_PIPELINE_REG_29),
		.result(L0_output_wires_20)
	);

	adder_with_1_reg L0_adder_21and28(
		.dataa (INPUT_PIPELINE_REG_21),
		.datab (INPUT_PIPELINE_REG_28),
		.result(L0_output_wires_21)
	);

	adder_with_1_reg L0_adder_22and27(
		.dataa (INPUT_PIPELINE_REG_22),
		.datab (INPUT_PIPELINE_REG_27),
		.result(L0_output_wires_22)
	);

	adder_with_1_reg L0_adder_23and26(
		.dataa (INPUT_PIPELINE_REG_23),
		.datab (INPUT_PIPELINE_REG_26),
		.result(L0_output_wires_23)
	);

	adder_with_1_reg L0_adder_24and25(
		.dataa (INPUT_PIPELINE_REG_24),
		.datab (INPUT_PIPELINE_REG_25),
		.result(L0_output_wires_24)
	);

	// (25 main tree Adders)

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
	wire [dw-1:0] L1_mult_wires_20;
	wire [dw-1:0] L1_mult_wires_21;
	wire [dw-1:0] L1_mult_wires_22;
	wire [dw-1:0] L1_mult_wires_23;
	wire [dw-1:0] L1_mult_wires_24;

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

	multiplier_with_reg L1_mul_9(
		.dataa (L0_output_wires_9),
		.datab (COEFFICIENT_9),
		.result(L1_mult_wires_9)
	);

	multiplier_with_reg L1_mul_10(
		.dataa (L0_output_wires_10),
		.datab (COEFFICIENT_10),
		.result(L1_mult_wires_10)
	);

	multiplier_with_reg L1_mul_11(
		.dataa (L0_output_wires_11),
		.datab (COEFFICIENT_11),
		.result(L1_mult_wires_11)
	);

	multiplier_with_reg L1_mul_12(
		.dataa (L0_output_wires_12),
		.datab (COEFFICIENT_12),
		.result(L1_mult_wires_12)
	);

	multiplier_with_reg L1_mul_13(
		.dataa (L0_output_wires_13),
		.datab (COEFFICIENT_13),
		.result(L1_mult_wires_13)
	);

	multiplier_with_reg L1_mul_14(
		.dataa (L0_output_wires_14),
		.datab (COEFFICIENT_14),
		.result(L1_mult_wires_14)
	);

	multiplier_with_reg L1_mul_15(
		.dataa (L0_output_wires_15),
		.datab (COEFFICIENT_15),
		.result(L1_mult_wires_15)
	);

	multiplier_with_reg L1_mul_16(
		.dataa (L0_output_wires_16),
		.datab (COEFFICIENT_16),
		.result(L1_mult_wires_16)
	);

	multiplier_with_reg L1_mul_17(
		.dataa (L0_output_wires_17),
		.datab (COEFFICIENT_17),
		.result(L1_mult_wires_17)
	);

	multiplier_with_reg L1_mul_18(
		.dataa (L0_output_wires_18),
		.datab (COEFFICIENT_18),
		.result(L1_mult_wires_18)
	);

	multiplier_with_reg L1_mul_19(
		.dataa (L0_output_wires_19),
		.datab (COEFFICIENT_19),
		.result(L1_mult_wires_19)
	);

	multiplier_with_reg L1_mul_20(
		.dataa (L0_output_wires_20),
		.datab (COEFFICIENT_20),
		.result(L1_mult_wires_20)
	);

	multiplier_with_reg L1_mul_21(
		.dataa (L0_output_wires_21),
		.datab (COEFFICIENT_21),
		.result(L1_mult_wires_21)
	);

	multiplier_with_reg L1_mul_22(
		.dataa (L0_output_wires_22),
		.datab (COEFFICIENT_22),
		.result(L1_mult_wires_22)
	);

	multiplier_with_reg L1_mul_23(
		.dataa (L0_output_wires_23),
		.datab (COEFFICIENT_23),
		.result(L1_mult_wires_23)
	);

	multiplier_with_reg L1_mul_24(
		.dataa (L0_output_wires_24),
		.datab (COEFFICIENT_24),
		.result(L1_mult_wires_24)
	);

	// (25 Multipliers)

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
	wire [dw-1:0] L1_output_wires_10;
	wire [dw-1:0] L1_output_wires_11;
	wire [dw-1:0] L1_output_wires_12;

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

	adder_with_1_reg L1_adder_8and9(
		.dataa (L1_mult_wires_8),
		.datab (L1_mult_wires_9),
		.result(L1_output_wires_4)
	);

	adder_with_1_reg L1_adder_10and11(
		.dataa (L1_mult_wires_10),
		.datab (L1_mult_wires_11),
		.result(L1_output_wires_5)
	);

	adder_with_1_reg L1_adder_12and13(
		.dataa (L1_mult_wires_12),
		.datab (L1_mult_wires_13),
		.result(L1_output_wires_6)
	);

	adder_with_1_reg L1_adder_14and15(
		.dataa (L1_mult_wires_14),
		.datab (L1_mult_wires_15),
		.result(L1_output_wires_7)
	);

	adder_with_1_reg L1_adder_16and17(
		.dataa (L1_mult_wires_16),
		.datab (L1_mult_wires_17),
		.result(L1_output_wires_8)
	);

	adder_with_1_reg L1_adder_18and19(
		.dataa (L1_mult_wires_18),
		.datab (L1_mult_wires_19),
		.result(L1_output_wires_9)
	);

	adder_with_1_reg L1_adder_20and21(
		.dataa (L1_mult_wires_20),
		.datab (L1_mult_wires_21),
		.result(L1_output_wires_10)
	);

	adder_with_1_reg L1_adder_22and23(
		.dataa (L1_mult_wires_22),
		.datab (L1_mult_wires_23),
		.result(L1_output_wires_11)
	);

	// (12 main tree Adders)

	// ********* Byes ******** \\
	one_register L1_byereg_for_24(
		.dataa (L1_mult_wires_24),
		.result(L1_output_wires_12)
	);

	// (1 byes)

	// ************************* LEVEL 2 ************************* \\
	wire [dw-1:0] L2_output_wires_0;
	wire [dw-1:0] L2_output_wires_1;
	wire [dw-1:0] L2_output_wires_2;
	wire [dw-1:0] L2_output_wires_3;
	wire [dw-1:0] L2_output_wires_4;
	wire [dw-1:0] L2_output_wires_5;
	wire [dw-1:0] L2_output_wires_6;

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

	adder_with_1_reg L2_adder_4and5(
		.dataa (L1_output_wires_4),
		.datab (L1_output_wires_5),
		.result(L2_output_wires_2)
	);

	adder_with_1_reg L2_adder_6and7(
		.dataa (L1_output_wires_6),
		.datab (L1_output_wires_7),
		.result(L2_output_wires_3)
	);

	adder_with_1_reg L2_adder_8and9(
		.dataa (L1_output_wires_8),
		.datab (L1_output_wires_9),
		.result(L2_output_wires_4)
	);

	adder_with_1_reg L2_adder_10and11(
		.dataa (L1_output_wires_10),
		.datab (L1_output_wires_11),
		.result(L2_output_wires_5)
	);

	// (6 main tree Adders)

	// ********* Byes ******** \\
	one_register L2_byereg_for_12(
		.dataa (L1_output_wires_12),
		.result(L2_output_wires_6)
	);

	// (1 byes)

	// ************************* LEVEL 3 ************************* \\
	wire [dw-1:0] L3_output_wires_0;
	wire [dw-1:0] L3_output_wires_1;
	wire [dw-1:0] L3_output_wires_2;
	wire [dw-1:0] L3_output_wires_3;

	adder_with_1_reg L3_adder_0and1(
		.dataa (L2_output_wires_0),
		.datab (L2_output_wires_1),
		.result(L3_output_wires_0)
	);

	adder_with_1_reg L3_adder_2and3(
		.dataa (L2_output_wires_2),
		.datab (L2_output_wires_3),
		.result(L3_output_wires_1)
	);

	adder_with_1_reg L3_adder_4and5(
		.dataa (L2_output_wires_4),
		.datab (L2_output_wires_5),
		.result(L3_output_wires_2)
	);

	// (3 main tree Adders)

	// ********* Byes ******** \\
	one_register L3_byereg_for_6(
		.dataa (L2_output_wires_6),
		.result(L3_output_wires_3)
	);

	// (1 byes)

	// ************************* LEVEL 4 ************************* \\
	wire [dw-1:0] L4_output_wires_0;
	wire [dw-1:0] L4_output_wires_1;

	adder_with_1_reg L4_adder_0and1(
		.dataa (L3_output_wires_0),
		.datab (L3_output_wires_1),
		.result(L4_output_wires_0)
	);

	adder_with_1_reg L4_adder_2and3(
		.dataa (L3_output_wires_2),
		.datab (L3_output_wires_3),
		.result(L4_output_wires_1)
	);

	// (2 main tree Adders)

	// ************************* LEVEL 5 ************************* \\
	wire [dw-1:0] L5_output_wires_0;

	adder_with_1_reg L5_adder_0and1(
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
	reg     [17:0]  o_out;

	always @(posedge clk) begin
		if(clk_ena) begin
			o_out <= L5_output_wires_0;
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
	pipeline_reg_39,
	pipeline_reg_40,
	pipeline_reg_41,
	pipeline_reg_42,
	pipeline_reg_43,
	pipeline_reg_44,
	pipeline_reg_45,
	pipeline_reg_46,
	pipeline_reg_47,
	pipeline_reg_48,
	pipeline_reg_49,
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
	output [WIDTH-1:0] pipeline_reg_39;
	output [WIDTH-1:0] pipeline_reg_40;
	output [WIDTH-1:0] pipeline_reg_41;
	output [WIDTH-1:0] pipeline_reg_42;
	output [WIDTH-1:0] pipeline_reg_43;
	output [WIDTH-1:0] pipeline_reg_44;
	output [WIDTH-1:0] pipeline_reg_45;
	output [WIDTH-1:0] pipeline_reg_46;
	output [WIDTH-1:0] pipeline_reg_47;
	output [WIDTH-1:0] pipeline_reg_48;
	output [WIDTH-1:0] pipeline_reg_49;
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
	reg [WIDTH-1:0] pipeline_reg_39;
	reg [WIDTH-1:0] pipeline_reg_40;
	reg [WIDTH-1:0] pipeline_reg_41;
	reg [WIDTH-1:0] pipeline_reg_42;
	reg [WIDTH-1:0] pipeline_reg_43;
	reg [WIDTH-1:0] pipeline_reg_44;
	reg [WIDTH-1:0] pipeline_reg_45;
	reg [WIDTH-1:0] pipeline_reg_46;
	reg [WIDTH-1:0] pipeline_reg_47;
	reg [WIDTH-1:0] pipeline_reg_48;
	reg [WIDTH-1:0] pipeline_reg_49;
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
			pipeline_reg_39 <= 0;
			pipeline_reg_40 <= 0;
			pipeline_reg_41 <= 0;
			pipeline_reg_42 <= 0;
			pipeline_reg_43 <= 0;
			pipeline_reg_44 <= 0;
			pipeline_reg_45 <= 0;
			pipeline_reg_46 <= 0;
			pipeline_reg_47 <= 0;
			pipeline_reg_48 <= 0;
			pipeline_reg_49 <= 0;
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
				pipeline_reg_39 <= pipeline_reg_38;
				pipeline_reg_40 <= pipeline_reg_39;
				pipeline_reg_41 <= pipeline_reg_40;
				pipeline_reg_42 <= pipeline_reg_41;
				pipeline_reg_43 <= pipeline_reg_42;
				pipeline_reg_44 <= pipeline_reg_43;
				pipeline_reg_45 <= pipeline_reg_44;
				pipeline_reg_46 <= pipeline_reg_45;
				pipeline_reg_47 <= pipeline_reg_46;
				pipeline_reg_48 <= pipeline_reg_47;
				pipeline_reg_49 <= pipeline_reg_48;
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
				//pipeline_reg_39 <= pipeline_reg_39;
				//pipeline_reg_40 <= pipeline_reg_40;
				//pipeline_reg_41 <= pipeline_reg_41;
				//pipeline_reg_42 <= pipeline_reg_42;
				//pipeline_reg_43 <= pipeline_reg_43;
				//pipeline_reg_44 <= pipeline_reg_44;
				//pipeline_reg_45 <= pipeline_reg_45;
				//pipeline_reg_46 <= pipeline_reg_46;
				//pipeline_reg_47 <= pipeline_reg_47;
				//pipeline_reg_48 <= pipeline_reg_48;
				//pipeline_reg_49 <= pipeline_reg_49;
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


