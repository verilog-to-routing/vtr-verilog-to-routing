
DATA_WIDTH=16
DW_ADD_INT="$DATA_WIDTH"                  #Internal adder precision bits
DW_MULT_INT=$[ 2 * DATA_WIDTH]                 #Internal multiplier precision bits
SCALE_FACTOR=$[ DW_MULT_INT - DATA_WIDTH - 1]  #Multiplier normalization shift amount

NUM_COEFF=51
N_UNIQ=26
N_EXTRA_VALID=13

N_VALID_REGS=$[NUM_COEFF + N_EXTRA_VALID]
N_INPUT_REGS=$[N + N_EXTRA_INPUT_REG]

COEFFICIENTS=(
	"16'd111"
	"16'd0"
	"-16'd122"
	"-16'd247"
	"-16'd368"
	"-16'd475"
	"-16'd559"
	"-16'd613"
	"-16'd630"
	"-16'd602"
	"-16'd526"
	"-16'd399"
	"-16'd223"
	"16'd0"
	"16'd265"
	"16'd564"
	"16'd888"
	"16'd1226"
	"16'd1565"
	"16'd1893"
	"16'd2196"
	"16'd2464"
	"16'd2684"
	"16'd2848"
	"16'd2950"
	"16'd2984"
	"16'd2950"
)

echo \
"module fir (
	clk,
	reset,
	clk_ena,
	i_valid,
	i_in,
	o_valid,
	o_out
);
	// Data Width
	parameter dw = $DATA_WIDTH; //Data input/output bits

	// Number of filter coefficients
	parameter N = $NUM_COEFF;
	parameter N_UNIQ = $N_UNIQ; // ciel(N/2) assuming symmetric filter coefficients

	// DSP Info
	parameter N_DSP_INST = 7;	//Total number of dsp_mac_4mult and dsp_mac_2mult instances
	parameter N_DSP2_INST = 1; //Number of dsp_mac_2mult instances

	//Number of extra valid cycles needed to align output (i.e. computation pipeline depth + input/output registers
	parameter N_EXTRA_VALID = $N_EXTRA_VALID;
	localparam N_VALID_REGS = $N_VALID_REGS;

	input clk;
	input reset;
	input clk_ena;
	input i_valid;
	input [dw-1:0] i_in; // signed
	output o_valid;
	output [dw-1:0] o_out; // signed

	// Data Width dervied parameters
	localparam dw_add_int = $DW_ADD_INT; //Internal adder precision bits
	localparam dw_mult_int = $DW_MULT_INT; //Internal multiplier precision bits
	localparam scale_factor = $SCALE_FACTOR; //Multiplier normalization shift amount

	// DSP output width
	localparam DSP_RESULT_WIDTH = 44;

	// Number of extra registers in INPUT_PIPELINE_REG to prevent contention for CHAIN_END's chain adders
	// localparam N_EXTRA_INPUT_REG = (N_DSP_INST - N_DSP2_INST) / 2; //Should be 3
	localparam N_INPUT_ADD_CYCLES = 1; //Number of cycles for input adders

	// Debug
	// initial begin
	// 	\$display (\"Data Width: %d\", dw);
	// 	\$display (\"Data Width Add Internal: %d\", dw_add_int);
	// 	\$display (\"Data Width Mult Internal: %d\", dw_mult_int);
	// 	\$display (\"Scale Factor: %d\", scale_factor);
	// end

	////******************************************************
	// *
	// * Valid Delay Pipeline
	// *
	// *****************************************************
	//Input valid signal is pipelined to become output valid signal

	//Valid registers
	reg [N_VALID_REGS:0] VALID_PIPELINE_REGS;
"

# typeset -i i
# let i=0
# while ((i < N_VALID_REGS)); do
	
# 	if (( i == 0 )); then # FIRST_STAGE_VALID

# 		echo \
# "		always@(posedge clk or posedge reset) begin
# 			if(reset)
# 				VALID_PIPELINE_REGS[$i] <= 0;
# 			else
# 				if(clk_ena)
# 					VALID_PIPELINE_REGS[$i] <= i_valid;
# 				else
# 					VALID_PIPELINE_REGS[$i] <= VALID_PIPELINE_REGS[$i];
# 		end"

# 	else # LATER_STAGE_VALID - Later stages (i > 0)

# 		echo \
# "		always@(posedge clk or posedge reset) begin
# 			if(reset)
# 				VALID_PIPELINE_REGS[$i] <= 0;
# 			else
# 				if(clk_ena)
# 					VALID_PIPELINE_REGS[$i] <= VALID_PIPELINE_REGS[$i-1];
# 				else
# 					VALID_PIPELINE_REGS[$i] <= VALID_PIPELINE_REGS[$i];
# 		end"
# 	fi
# 	let i++
# done

	echo \
"	always@(posedge clk or posedge reset) begin
		if(reset)
			VALID_PIPELINE_REGS <= 0;
		else
			if(clk_ena)
				VALID_PIPELINE_REGS <= {VALID_PIPELINE_REGS[N_VALID_REGS-1:0], i_valid};
			else
				VALID_PIPELINE_REGS <= VALID_PIPELINE_REGS;
	end"

echo \
"	////******************************************************
	// *
	// * Input Register Pipeline
	// *
	// *****************************************************
	//Pipelined input values

	//Input value registers
	reg [dw-1:0] INPUT_PIPELINE_REG[N + N_EXTRA_INPUT_REG:0]; //signed
"

# typeset -i k
# let k=0
# while ((k < N_INPUT_REGS)); do # INPUT_PIPELINE_REG
# 	if ((k == 0)); then # FIRST_STAGE_INPUT //First stage
# 		echo \
# "		always@(posedge clk or posedge reset) begin
# 			if(reset)
# 				INPUT_PIPELINE_REG[$k] <= 0;
# 			else
# 				if(clk_ena)
# 					INPUT_PIPELINE_REG[$k] <= i_in;
# 				else
# 					INPUT_PIPELINE_REG[$k] <= INPUT_PIPELINE_REG[$k];
# 		end"
# 	else # LATER_STAGE_INPUT // Later stages (i > 0)
# 		echo \
# "		always@(posedge clk or posedge reset) begin
# 			if(reset)
# 				INPUT_PIPELINE_REG[$k] <= 0;
# 			else begin
# 				if(clk_ena)
# 					INPUT_PIPELINE_REG[$k] <= INPUT_PIPELINE_REG[$k-1];
# 				else
# 					INPUT_PIPELINE_REG[$k] <= INPUT_PIPELINE_REG[$k];
# 			end
# 		end"
# 	fi
# 	let k++
# done

	echo \
"	always@(posedge clk or posedge reset) begin
		if(reset)
			INPUT_PIPELINE_REG <= 0;
		else begin
			if(clk_ena)
				INPUT_PIPELINE_REG <= {INPUT_PIPELINE_REG[N_INPUT_REGS-1:0], i_in};
			else
				INPUT_PIPELINE_REG <= INPUT_PIPELINE_REG;
		end
	end"

	echo \
"	////******************************************************
	// *
	// * Computation Pipeline
	// *
	// *****************************************************"
# generate for (j = 0; j < N_DSP_INST; j++) begin : DSP_INST
# typeset -i j
# let j=0
# while ((j < N_DSP_INST)); do
# 		echo \
# "		localparam N_INPUTS_4MULT = 4;

# 		//Offsets required to account for extra cycle delay caused
# 		// by DSP block chaining
# 		localparam head_offset_even = 0;
# 		localparam head_offset_odd = 1;
# 		localparam tail_offset_even = 0;
# 		localparam tail_offset_odd = 1;
# 		localparam last_offset = 2;

# 		//Intermediate wires
# 		wire [DSP_RESULT_WIDTH-1:0] w_result; //signed
# 		wire [DSP_RESULT_WIDTH-1:0] w_norm_result_full_width; //signed
# 		wire [dw_add_int-1:0] w_norm_result; //signed
# 		wire [dw-1:0] w_shiftout; //signed

# 		//Merge inputs due to symmetric filter coefficients
# 		wire [dw_add_int-1:0] w_inadder_out_0; //signed
# 		wire [dw_add_int-1:0] w_inadder_out_1; //signed
# 		wire [dw_add_int-1:0] w_inadder_out_2; //signed
# 		wire [dw_add_int-1:0] w_inadder_out_3; //signed"

 		if (( $[ $[j+1] * N_INPUTS_4MULT] <= N_UNIQ )); then # INTERMEDIATE //All DSPs except for last

# 			if ((j % 2 == 0)); then # CHAIN_START
# 				echo \
# "				//Start of a DSP block chain pair

# 				//Input adders merging samples with common coefficients
# 				input_adder	input_adder_inst0 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[$j*N_INPUTS_4MULT+0+head_offset_even]),
# 					.datab 	(INPUT_PIPELINE_REG[N-($j*N_INPUTS_4MULT)-1+tail_offset_even]),
# 					.result 	(w_inadder_out_0)
# 				);
# 				input_adder	input_adder_inst1 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[$j*N_INPUTS_4MULT+1+head_offset_even]),
# 					.datab 	(INPUT_PIPELINE_REG[N-($j*N_INPUTS_4MULT)-2+tail_offset_even]),
# 					.result 	(w_inadder_out_1)
# 				);
# 				input_adder	input_adder_inst2 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[$j*N_INPUTS_4MULT+2+head_offset_even]),
# 					.datab 	(INPUT_PIPELINE_REG[N-($j*N_INPUTS_4MULT)-3+tail_offset_even]),
# 					.result 	(w_inadder_out_2)
# 				);
# 				input_adder	input_adder_inst3 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[$j*N_INPUTS_4MULT+3+head_offset_even]),
# 					.datab 	(INPUT_PIPELINE_REG[N-($j*N_INPUTS_4MULT)-4+tail_offset_even]),
# 					.result 	(w_inadder_out_3)
# 				);

# 				//Half of a Stratix IV DSP block
# 				dsp_4mult_add   dsp_4mult_add_inst (
# 					.clock0	(clk),
# 					.ena0		(clk_ena),
# 					.chainin (), //Un-connected
# 					.dataa_0 (w_inadder_out_0),
# 					.dataa_1 (w_inadder_out_1),
# 					.dataa_2 (w_inadder_out_2),
# 					.dataa_3 (w_inadder_out_3),
# 					.datab_0 (${COEFFICIENTS[$[j * N_INPUTS_4MULT    ]]}),
# 					.datab_1 (${COEFFICIENTS[$[j * N_INPUTS_4MULT + 1]]}),
# 					.datab_2 (${COEFFICIENTS[$[j * N_INPUTS_4MULT + 2]]}),
# 					.datab_3 (${COEFFICIENTS[$[j * N_INPUTS_4MULT + 3]]}),
# 					.result 	(w_result)
# 				);"
# 			else # CHAIN_END
# 				echo \
# "				//End of a DSP block chain pair

# 				input_adder	input_adder_inst0 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[j*N_INPUTS_4MULT+0+head_offset_odd]),
# 					.datab 	(INPUT_PIPELINE_REG[N-(j*N_INPUTS_4MULT)-1+tail_offset_odd]),
# 					.result 	(w_inadder_out_0)
# 				);
# 				input_adder	input_adder_inst1 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[j*N_INPUTS_4MULT+1+head_offset_odd]),
# 					.datab 	(INPUT_PIPELINE_REG[N-(j*N_INPUTS_4MULT)-2+tail_offset_odd]),
# 					.result 	(w_inadder_out_1)
# 				);
# 				input_adder	input_adder_inst2 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[j*N_INPUTS_4MULT+2+head_offset_odd]),
# 					.datab 	(INPUT_PIPELINE_REG[N-(j*N_INPUTS_4MULT)-3+tail_offset_odd]),
# 					.result 	(w_inadder_out_2)
# 				);
# 				input_adder	input_adder_inst3 (
# 					.clock 	(clk),
# 					.clken	(clk_ena),
# 					.dataa 	(INPUT_PIPELINE_REG[j*N_INPUTS_4MULT+3+head_offset_odd]),
# 					.datab 	(INPUT_PIPELINE_REG[N-(j*N_INPUTS_4MULT)-4+tail_offset_odd]),
# 					.result 	(w_inadder_out_3)
# 				);

# 				dsp_4mult_add   dsp_4mult_add_inst (
# 					.clock0	(clk),
# 					.ena0		(clk_ena),
# 					.chainin (DSP_INST[j-1].w_result), //Chain from previous stage
# 					.dataa_0 (w_inadder_out_0),
# 					.dataa_1 (w_inadder_out_1),
# 					.dataa_2 (w_inadder_out_2),
# 					.dataa_3 (w_inadder_out_3),
# 					.datab_0 (${COEFFICIENTS[$[j * N_INPUTS_4MULT    ]]}),
# 					.datab_1 (${COEFFICIENTS[$[j * N_INPUTS_4MULT + 1]]}),
# 					.datab_2 (${COEFFICIENTS[$[j * N_INPUTS_4MULT + 2]]}),
# 					.datab_3 (${COEFFICIENTS[$[j * N_INPUTS_4MULT + 3]]}),
# 					.result 	(w_result)
# 				);"
# 			fi
# 		else # LAST
# 			echo \
# "			input_adder	input_adder_inst0 (
# 				.clock 	(clk),
# 				.clken	(clk_ena),
# 				.dataa 	(INPUT_PIPELINE_REG[j*N_INPUTS_4MULT+0+last_offset]),
# 				.datab 	(INPUT_PIPELINE_REG[N-(j*N_INPUTS_4MULT)-1+last_offset]),
# 				.result 	(w_inadder_out_0)
# 			);

# 			//Last coefficient is not repeated, so no adder
# 			assign w_inadder_out_1 = INPUT_PIPELINE_REG[j*N_INPUTS_4MULT+1+last_offset+N_INPUT_ADD_CYCLES];

# 			dsp_2mult_add	dsp_2mult_add_inst (
# 				.clock0 	(clk),
# 				.ena0		(clk_ena),
# 				.dataa_0 (w_inadder_out_0),
# 				.dataa_1 (w_inadder_out_1),
# 				.datab_0 (${COEFFICIENTS[$[N _ UNIQ - 2]]}),
# 				.datab_1 (${COEFFICIENTS[$[N _ UNIQ - 1]]}),
# 				.result 	(w_result)
# 			);"
# 		fi

# 		echo \
# "		assign w_norm_result_full_width = w_result >>> scale_factor; //Full width for chain
# 		assign w_norm_result = w_result >>> scale_factor; //Reduced width for output"

# 	let j++
# done

echo \
"	////******************************************************
	// *
	// * Output Adder Tree
	// *
	// *****************************************************
	wire [dw_add_int-1:0] w_sum;

	//The last portion of the adder tree that doesn't fit in
	// DSP blocks
	adder_tree out_adder_tree (
		.clk(clk),
		.clk_ena(clk_ena),
		.reset(reset),
		.i_a(DSP_INST[1].w_norm_result),
		.i_b(DSP_INST[3].w_norm_result),
		.i_c(DSP_INST[5].w_norm_result),
		.i_d(DSP_INST[6].w_norm_result),
		.o_sum(w_sum)
	);
	out_adder_tree.WIDTH(dw_add_int)

	////******************************************************
	// *
	// * Output Logic
	// *
	// *****************************************************
	//Actual outputs
	assign o_out = w_sum;
	assign o_valid = VALID_PIPELINE_REGS[N+N_EXTRA_VALID-1];

endmodule"

echo \
"
//
//  A 4 input adder tree (2 levels)
//
//  Inputs are not registered (should be fed by
//  registered outputs of DSP blocks).
//
//  Output is registered.
//
//  Each level is pipelined to improve frequency.
//

module adder_tree (
	clk,
	clk_ena,
	reset,
	i_a, // signed
	i_b, // signed
	i_c, // signed
	i_d, // signed
	o_sum // signed
);

	parameter WIDTH = 17;
	input clk;
	input clk_ena;
	input reset;
	input [WIDTH-1:0] i_a; // signed
	input [WIDTH-1:0] i_b; // signed
	input [WIDTH-1:0] i_c; // signed
	input [WIDTH-1:0] i_d; // signed
	output [WIDTH-1:0] o_sum; // signed

	//Unregistered adder outputs
	wire [WIDTH-1:0] w_add1_sum; // signed
	wire [WIDTH-1:0] w_add2_sum; // signed
	wire [WIDTH-1:0] w_add3_sum; // signed

	//Registered adder outputs
	reg [WIDTH-1:0] r_add1_sum; // signed
	reg [WIDTH-1:0] r_add2_sum; // signed
	reg [WIDTH-1:0] r_add3_sum; // signed

	//Level 1
	output_adder output_adder_inst1 (
		.clock 	(clk),
		.clken	(clk_ena),
		.dataa 	(i_a),
		.datab 	(i_b),
		.result 	(w_add1_sum)
	);

	output_adder output_adder_inst2 (
		.clock 	(clk),
		.clken	(clk_ena),
		.dataa 	(i_c),
		.datab 	(i_d),
		.result 	(w_add2_sum)
	);

	//Level 1 Output registers
	always@(posedge clk or posedge reset) begin
		if(reset) begin
			r_add1_sum <= 0;
			r_add2_sum <= 0;
		end
		else begin
			if(clk_ena) begin
				r_add1_sum <= w_add1_sum;
				r_add2_sum <= w_add2_sum;
			end else begin
				r_add1_sum <= r_add1_sum;
				r_add2_sum <= r_add2_sum;
			end
		end
	end

	//Level 2
	output_adder	output_adder_inst3 (
		.clock 	(clk),
		.clken	(clk_ena),
		.dataa 	(r_add1_sum),
		.datab 	(r_add2_sum),
		.result 	(w_add3_sum)
	);

	//Level 2 Output registers
	always@(posedge clk or posedge reset) begin
		if (reset)
			r_add3_sum <= 0;
		else
			if(clk_ena)
				r_add3_sum <= w_add3_sum;
			else
				r_add3_sum <= r_add3_sum;
	end

	//Final output
	assign o_sum = r_add3_sum;

endmodule

module input_adder (
	clken,
	clock,
	dataa,
	datab,
	result);

	input	  clken;
	input	  clock;
	input	[15:0]  dataa;
	input	[15:0]  datab;
	output	[15:0]  result;
	reg     [15:0]  result;

	always @(posedge clk) begin
		if(clken) begin
			result <= dataa + datab;
		end
	end

endmodule

module output_adder (
	clken,
	clock,
	dataa,
	datab,
	result);

	input	  clken;
	input	  clock;
	input	[15:0]  dataa;
	input	[15:0]  datab;
	output	[15:0]  result;
	reg     [15:0]  result;

	reg [15:0] pipereg1;

	always @(posedge clk) begin
		if (clken) begin
			pipereg1 <= dataa + datab;
			result <= pipereg1;
		end
	end
endmodule

"
