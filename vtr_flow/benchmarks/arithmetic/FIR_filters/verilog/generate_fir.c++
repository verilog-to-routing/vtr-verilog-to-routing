#include <cmath>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

void make_adder(
	const size_t level,
	const string& input_wire,  const size_t inputa_index, const size_t inputb_index,
	const string& output_wire, const size_t output_index, const bool is_pipelined);

string make_output_wire_name(int level);

string coeffecient(size_t index);
string input_reg(size_t index);

namespace twodim {
	string index_into(const string& basename, size_t index);
	void decl(const string& type, const string& basename, size_t size);
	void nonblk_assign(const string& dest, const string& src, size_t size, bool index_src=true, int src_offset=0, size_t start_index=0);
}

namespace pipeline {
	string assign(
		const size_t width,
		const bool is_pipelined,
		const string& input,
		const string& output,
		const string& clk,
		const string& clk_ena
	);
}

namespace simplemodule {
	void declare(
		const string& name,
		const size_t num_inputs,
		const size_t width,
		const bool is_pipelined,
		const string& contents
	);
	void instantiate(
		const string& name,
		const vector<string>& inputs,
		const string& output,
		const bool is_pipelined
	);
}

struct ParsedParams {
	size_t num_coeff;
	bool piped;
	bool good;
	ParsedParams(int argc, char const *argv[]) {
		good = true;
		if (argc == 3) {
			if (strcmp(argv[1],"pipe") == 0) {
				piped = true;
			} else if (strcmp(argv[1], "nopipe") == 0) {
				piped = false;
			} else {
				cerr << "arg 1 should be \"pipe\" or \"nopipe\"\n";
				good = false;
				piped = true;
			}
			istringstream arg1parser(argv[2]);
			if (!(arg1parser >> num_coeff)) {
				cerr << "num_coeff isn't a number\n";
				good = false;
				num_coeff = 0;
			}
		} else {
			fputs("wrong number of arguments", stderr);
			good = false;
		}
	}
	string to_str() const {
		ostringstream builder;
		builder
		<<	"// CONFIG:\n"
		<<	"// NUM_COEFF = " << num_coeff << '\n'
		<<	"// PIPLINED = " << piped << '\n'
		<<	"\n"
		;
		return builder.str();
	}
};

ostream& operator<<(ostream& stream, const ParsedParams& params) {
	stream << params.to_str();
	return stream;
}

int main(int argc, char const *argv[]) {

	ParsedParams params(argc, argv);

	if (!params.good) {
		exit(1);
	}

	cout << params;

	const size_t DATA_WIDTH = 18;
	const size_t DW_ADD_INT = DATA_WIDTH;                      // Internal adder precision bits
	const size_t DW_MULT_INT = 2 * DATA_WIDTH;                 // Internal multiplier precision bits
	const size_t SCALE_FACTOR = DW_MULT_INT - DATA_WIDTH - 1;  // Multiplier normalization shift amount

	const bool IS_PIPELINED = params.piped;

	const size_t NUM_COEFF = params.num_coeff;//51;
	const size_t N_UNIQ = ceil(NUM_COEFF/2.0);

	      size_t NUM_ADDER_LEVELS = 0;

	{
		size_t p2 = 1; // power 2?
		while (true) {
			if (p2 >= NUM_COEFF) {
				break;
			} else {
				p2 *= 2;
				++NUM_ADDER_LEVELS;
			}
		}
	}
	const size_t N_VALID_REGS = NUM_COEFF + (IS_PIPELINED ? NUM_ADDER_LEVELS : 0) + 1; // plus one for the multiplier delay.
	
	const size_t N_EXTRA_INPUT_REG = 0; // (N_DSP_INST - N_DSP2_INST) / 2; //Should be 3
	const size_t N_INPUT_REGS = NUM_COEFF + N_EXTRA_INPUT_REG;


	// put DATA_WIDTH'd in front
	const vector<int> COEFFICIENTS = {
		88,
		0,
		-97,
		-197,
		-294,
		-380,
		-447,
		-490,
		-504,
		-481,
		-420,
		-319,
		-178,
		0,
		212,
		451,
		710,
		980,
		1252,
		1514,
		1756,
		1971,
		2147,
		2278,
		2360,
		2387,// the "center" coefficient
	};

	size_t EXPECTED_COEFFECIENTS = NUM_COEFF/2 + NUM_COEFF%2;
	if (COEFFICIENTS.size() < EXPECTED_COEFFECIENTS) {
		cerr << "// ERROR: not enough COEFFICIENTS in array (there are " << COEFFICIENTS.size() << ", and we need "<<EXPECTED_COEFFECIENTS<<")\n";
		exit(1);
	} else if (COEFFICIENTS.size() > EXPECTED_COEFFECIENTS) {
		ostringstream warningstream;
		warningstream << "// WARNING: more than enough COEFFICIENTS in array (there are " << COEFFICIENTS.size() << ", and we only need "<<EXPECTED_COEFFECIENTS<<")\n";
		cout << warningstream.str();
		// cerr << warningstream.str();
	}

	printf(
		"module fir (\n"
		"	clk,\n"
		"	reset,\n"
		"	clk_ena,\n"
		"	i_valid,\n"
		"	i_in,\n"
		"	o_valid,\n"
		"	o_out\n"
		");\n"
		"	// Data Width\n"
		"	parameter dw = %lu; //Data input/output bits\n"
		"\n"
		"	// Number of filter coefficients\n"
		"	parameter N = %lu;\n"
		"	parameter N_UNIQ = %lu; // ciel(N/2) assuming symmetric filter coefficients\n"
		"\n"
		"	//Number of extra valid cycles needed to align output (i.e. computation pipeline depth + input/output registers\n"
		"	localparam N_VALID_REGS = %lu;\n"
		"\n"
		"	input clk;\n"
		"	input reset;\n"
		"	input clk_ena;\n"
		"	input i_valid;\n"
		"	input [dw-1:0] i_in; // signed\n"
		"	output o_valid;\n"
		"	output [dw-1:0] o_out; // signed\n"
		"\n"
		"	// Data Width dervied parameters\n"
		"	localparam dw_add_int = %lu; //Internal adder precision bits\n"
		"	localparam dw_mult_int = %lu; //Internal multiplier precision bits\n"
		"	localparam scale_factor = %lu; //Multiplier normalization shift amount\n"
		"\n"
		"	// Number of extra registers in INPUT_PIPELINE_REG to prevent contention for CHAIN_END's chain adders\n"
		"	localparam N_INPUT_REGS = %lu;\n"
		"\n"
		,
		DATA_WIDTH,
		NUM_COEFF,
		N_UNIQ,

		N_VALID_REGS,
		
		DW_ADD_INT,
		DW_MULT_INT,
		SCALE_FACTOR,

		N_INPUT_REGS
	);

	puts(
		"	// Debug\n"
		"	// initial begin\n"
		"	// 	$display (\"Data Width: %d\", dw);\n"
		"	// 	$display (\"Data Width Add Internal: %d\", dw_add_int);\n"
		"	// 	$display (\"Data Width Mult Internal: %d\", dw_mult_int);\n"
		"	// 	$display (\"Scale Factor: %d\", scale_factor);\n"
		"	// end\n"
	);

	twodim::decl("reg [dw-1:0]", "COEFFICIENT", N_UNIQ);

	puts(
		"\n"
		"	always@(posedge clk) begin"
	);

	for (size_t i = 0; i < N_UNIQ; ++i) {
		cout
		<<	"		"<<coeffecient(i)<<" <= "<<(COEFFICIENTS[i] < 0 ? "-" : "")<<DATA_WIDTH<<"'d"<<abs(COEFFICIENTS[i])<<";\n";
	}

	puts(
		"	end\n"
	);

	puts(
		"	////******************************************************\n"
		"	// *\n"
		"	// * Valid Delay Pipeline\n"
		"	// *\n"
		"	// *****************************************************\n"
		"	//Input valid signal is pipelined to become output valid signal\n"
		"\n"
		"	//Valid registers\n"
		"	reg [N_VALID_REGS-1:0] VALID_PIPELINE_REGS;\n"
		"\n"
		"	always@(posedge clk or posedge reset) begin\n"
		"		if(reset) begin\n"
		"			VALID_PIPELINE_REGS <= 0;\n"
		"		end else begin\n"
		"			if(clk_ena) begin\n"
		"				VALID_PIPELINE_REGS <= {VALID_PIPELINE_REGS[N_VALID_REGS-2:0], i_valid};\n"
		"			end else begin\n"
		"				VALID_PIPELINE_REGS <= VALID_PIPELINE_REGS;\n"
		"			end\n"
		"		end\n"
		"	end\n"
	);

	puts(
		"	////******************************************************\n"
		"	// *\n"
		"	// * Input Register Pipeline\n"
		"	// *\n"
		"	// *****************************************************\n"
		"	//Pipelined input values\n"
		"\n"
		"	//Input value registers\n"
	);
	
	twodim::decl("wire [dw-1:0]", "INPUT_PIPELINE_REG", N_INPUT_REGS);

	puts(
		"\n"
		"	input_pipeline in_pipe(\n"
		"		.clk(clk), .clk_ena(clk_ena),\n"
		"		.in_stream(i_in),"
	);

	for (size_t i = 0; i < N_INPUT_REGS; ++i) {
		cout <<
		"		"<<twodim::index_into(".pipeline_reg",i)<<'('<<twodim::index_into("INPUT_PIPELINE_REG", i)<<"),\n";
	}

	cout
	<<	"		.reset(reset)"
	<<	"	);\n"
	<<	"	defparam in_pipe.WIDTH = "<<DATA_WIDTH<<"; // = dw\n"
	;

	puts(
		"	////******************************************************\n"
		"	// *\n"
		"	// * Computation Pipeline\n"
		"	// *\n"
		"	// *****************************************************\n"
	);
	// generate FIR circuitry - adders and multipliers and such
	for (size_t level = 0; level < NUM_ADDER_LEVELS; ++level) {
		const size_t NUM_INPUTS_TO_LEVEL   = ceil(NUM_COEFF/pow(2,level));
		const size_t LEVEL_WIDTH_WO_EXTRAS = NUM_INPUTS_TO_LEVEL / 2;
		const size_t NUM_EXTRA_IN_LEVEL    = NUM_INPUTS_TO_LEVEL % 2;
		const size_t FULL_LEVEL_WIDTH      = LEVEL_WIDTH_WO_EXTRAS + NUM_EXTRA_IN_LEVEL;

		// Create names of input and output wires for this level

		string level_output_wires = make_output_wire_name(level);
		string level_input_wires = make_output_wire_name(level-1);		

		cout << "	// ************************* LEVEL "<<level<<" ************************* \\\\\n";

		// MULTIPLIERS (only present on L1)
		if (level == 1) {
			string L1_mult_wires = "L1_mult_wires";

			cout << "	// **************** Multipliers **************** \\\\\n";

			twodim::decl("wire [dw-1:0]", L1_mult_wires, NUM_INPUTS_TO_LEVEL);

			cout << "\n";
			for (size_t i = 0; i < NUM_INPUTS_TO_LEVEL; ++i) {
				ostringstream namebuilder;
				namebuilder << "multiplier_with_reg L"<<level<<"_mul_"<<i;
				simplemodule::instantiate(
					namebuilder.str(),
					{
						twodim::index_into(level_input_wires, i),
						coeffecient(i)
					},
					twodim::index_into(L1_mult_wires, i),
					IS_PIPELINED
				);
			}

			level_input_wires = L1_mult_wires;

			cout
			<<	"	// ("<<NUM_INPUTS_TO_LEVEL<<" Multipliers)\n"
			<<	"\n"
			<<	"	// **************** Adders **************** \\\\\n";
		}

		twodim::decl("wire [dw-1:0]", level_output_wires, FULL_LEVEL_WIDTH);

		cout << "\n";

		// MAIN TREE ADDERS
		for (size_t i = 0; i < LEVEL_WIDTH_WO_EXTRAS; ++i) {
			if (level == 0) {
				make_adder(level, level_input_wires, i, NUM_INPUTS_TO_LEVEL-i-1, level_output_wires, i, IS_PIPELINED);
			} else {
				make_adder(level, level_input_wires, i*2, i*2+1, level_output_wires, i, IS_PIPELINED);
			}
		}

		cout << "	// ("<<LEVEL_WIDTH_WO_EXTRAS<<" main tree Adders)\n\n";

		// Byes for the extra inputs 
		if (NUM_EXTRA_IN_LEVEL > 0) {
			cout << "	// ********* Byes ******** \\\\\n";
			size_t offset = -1;
			if (level == 0) {
				offset = LEVEL_WIDTH_WO_EXTRAS; // in L0 the bye is the center one
			} else {
				offset = LEVEL_WIDTH_WO_EXTRAS*2; // else it's the last one
			}
			for (size_t i = 0; i < NUM_EXTRA_IN_LEVEL; ++i) {
				size_t extra_index = offset + i;
				
				ostringstream namebuilder;
				namebuilder << "one_register L"<<level<<"_byereg_for_"<<extra_index;
				
				simplemodule::instantiate(
					namebuilder.str(),
					{ twodim::index_into(level_input_wires, extra_index) },
					twodim::index_into(level_output_wires, LEVEL_WIDTH_WO_EXTRAS + i),
					IS_PIPELINED
				);
			}

			cout << "	// ("<<NUM_EXTRA_IN_LEVEL<<" byes)\n\n";
		}

	}

	cout
	<<	"	////******************************************************\n"
	<<	"	// *\n"
	<<	"	// * Output Logic\n"
	<<	"	// *\n"
	<<	"	// *****************************************************\n"
	<<	"	//Actual outputs\n"
	;

	cout <<
	pipeline::assign(
		DATA_WIDTH,
		!IS_PIPELINED,
		twodim::index_into(make_output_wire_name(NUM_ADDER_LEVELS-1), 0),
		"o_out",
		"clk",
		"clk_ena"
	);

	cout
	<<	"	assign o_valid = VALID_PIPELINE_REGS[N_VALID_REGS-1];\n"
	<<	"\n"
	<<	"endmodule\n"
	<<	"\n"
	<<	"\n"
	;

	/************************
	 * Begin Input Pipeline module
	 ************************/

	cout
	<<	"module input_pipeline (\n"
	<<	"	clk,\n"
	<<	"	clk_ena,\n"
	<<	"	in_stream,\n"
	;

	for (size_t i = 0; i < N_INPUT_REGS; ++i) {
		cout
		<<	"	"<<twodim::index_into("pipeline_reg", i)<<",\n";
	}

	cout
	<<	"	reset"
	<<	");\n"
	<<	"	parameter WIDTH = 1;\n"
	<<	"	//Input value registers\n"
	<<	"	input clk;\n"
	<<	"	input clk_ena;\n"
	<<	"	input [WIDTH-1:0] in_stream;\n"
	;

	twodim::decl("output [WIDTH-1:0]", "pipeline_reg", N_INPUT_REGS); // signed
	twodim::decl("reg [WIDTH-1:0]", "pipeline_reg", N_INPUT_REGS); //signed
	
	cout
	<<	"	input reset;\n"
	<<	"\n"
	<<	"	always@(posedge clk or posedge reset) begin\n"
	<<	"		if(reset) begin\n"
	;

	twodim::nonblk_assign("\t\tpipeline_reg", "0", N_INPUT_REGS, false);

	cout
	<<	"		end else begin\n"
	<<	"			if(clk_ena) begin\n"
	<<	"				"<<twodim::index_into("pipeline_reg", 0)<<" <= in_stream;\n"
	;

	twodim::nonblk_assign("\t\t\tpipeline_reg", "pipeline_reg", N_INPUT_REGS, true, -1 ,1);
	
	cout
	<<	"			end //else begin\n"
	;
	
	twodim::nonblk_assign("\t\t\t//pipeline_reg", "pipeline_reg", N_INPUT_REGS);

	cout
	<<	"			//end\n"
	<<	"		end\n"
	<<	"	end\n"
	<<	"endmodule\n"
	<<	"\n"
	<<	"\n"
	;

	/************************
	 * Begin Other modules
	 ************************/

	simplemodule::declare(
		"adder_with_1_reg",
		2,
		DATA_WIDTH,
		IS_PIPELINED,
		pipeline::assign(DATA_WIDTH, IS_PIPELINED, "dataa + datab", "result", "clk", "clk_ena")
	);

	simplemodule::declare(
		"multiplier_with_reg",
		2,
		DATA_WIDTH,
		IS_PIPELINED,
		pipeline::assign(DATA_WIDTH, IS_PIPELINED, "dataa * datab", "result", "clk", "clk_ena")
	);
	
	// if NUM_COEFF is a power of 2, we don't need any bye regs,
	// and the unused module confuses ODIN as it thinks there are 2 top level modules.
	if (NUM_COEFF != pow(2, NUM_ADDER_LEVELS)) {
		simplemodule::declare(
			"one_register",
			1,
			DATA_WIDTH,
			IS_PIPELINED,
			pipeline::assign(DATA_WIDTH, IS_PIPELINED, "dataa", "result", "clk", "clk_ena")
		);
	}

}

void make_adder(
	const size_t level,
	const string& input_wire,  const size_t inputa_index, const size_t inputb_index,
	const string& output_wire, const size_t output_index, const bool is_pipelined) {

	ostringstream namebuilder;
	namebuilder << "adder_with_1_reg L"<<level<<"_adder_"<<inputa_index<<"and"<<inputb_index;

	simplemodule::instantiate(
		namebuilder.str(),
		{
			twodim::index_into(input_wire,  inputa_index),
			twodim::index_into(input_wire,  inputb_index)
		},
		twodim::index_into(output_wire, output_index),
		is_pipelined
	);
}

string make_output_wire_name(int level) {
	if (level < 0) {
		return "INPUT_PIPELINE_REG";
	} else {
		ostringstream builder;
		builder << "L"<<level<<"_output_wires";
		return builder.str();
	}
}


string coeffecient(size_t index) {
	return twodim::index_into("COEFFICIENT", index);
}

string input_reg(size_t index) {
	return twodim::index_into("INPUT_PIPELINE_REG", index);
}

string twodim::index_into(const string& basename, size_t index) {
	ostringstream builder;
	// builder << basename << '[' << index << ']';
	builder << basename << '_' << index;
	return builder.str();
}

void twodim::decl(const string& type, const string& basename, size_t size) {
	// cout << "	"<<type<<' '<<basename<<'['<<size<<"-1:0];\n";
	for (size_t i = 0; i < size; ++i) {
		cout << 
		"	"<<type<<' '<<basename<<'_'<<i<<";\n";
	}
}

void twodim::nonblk_assign(const string& dest, const string& src, size_t size, bool index_src, int src_offset, size_t start_index) {
	// cout << "	"<<dest<<'['<<size<<"-1:"<<start_index<<"] <= "<<src<<'['<<(size-start_index+src_offset)<<":0];\n";
	for (size_t i = start_index; i < size; ++i) {
		string indexed_src;
		if (index_src) {
			indexed_src = twodim::index_into(src, i + src_offset);
		} else {
			indexed_src = src;
		}
		cout << 
		"	"<<twodim::index_into(dest,i)<<" <= "<<indexed_src<<";\n";
	}	
}

string pipeline::assign(
		const size_t width,
		const bool is_pipelined,
		const string& input,
		const string& output,
		const string& clk,
		const string& clk_ena) {

	ostringstream builder;
	if (is_pipelined) {
		builder
		<<	"	reg     ["<<(width-1)<<":0]  "<<output<<";\n"
		<<	"\n"
		<<	"	always @(posedge "<<clk<<") begin\n"
		<<	"		if("<<clk_ena<<") begin\n"
		<<	"			"<<output<<" <= "<<input<<";\n"
		<<	"		end\n"
		<<	"	end\n"
		<<	"\n"
		;
	} else {
		builder
		<<	"	assign "<<output<<" = "<<input<<";\n" 
		<<	"\n"
		;
	}
	return builder.str();
}

void simplemodule::declare(
		const string& name,
		const size_t num_inputs,
		const size_t width,
		const bool is_pipelined,
		const string& contents) {

	cout
	<<	"module "<<name<<" (\n"
	;
	if (is_pipelined) {
		cout
		<<	"	clk,\n"
		<<	"	clk_ena,\n"
		;
	}
	for (size_t i = 0; i < num_inputs; ++i) {
		cout
		<<	"	data"<<(char)('a' + i)<<",\n"
		;
	}
	cout
	<<	"	result);\n"
	<<	"\n"
	<<	"	input	  clk;\n"
	<<	"	input	  clk_ena;\n"
	;
	for (size_t i = 0; i < num_inputs; ++i) {
		cout
		<<	"	input	["<<(width-1)<<":0]  data" <<(char)('a' + i)<<";\n"
		;
	}
	cout
	<<	"	output	["<<(width-1)<<":0]  result;\n"
	<<	"\n"
	<<	contents
	<<	"endmodule\n"
	<<	"\n"
	<<	"\n"
	;
}

void simplemodule::instantiate(
	const string& name,
	const vector<string>& inputs,
	const string& output,
	const bool is_pipelined) {
	
	cout
	<<	"	"<<name<<"(\n"
	;
	if (is_pipelined) {
		cout
		<<	"		.clk(clk), .clk_ena(clk_ena),\n"
		;
	}
	for (size_t i = 0; i < inputs.size(); ++i) {
		cout
		<<	"		.data"<<(char)('a' + i)<<" ("<<inputs[i]<<"),\n"
		;
	}
	cout
	<<	"		.result("<<output<<")\n"
	<<	"	);\n"
	<<	"\n"
	;
}
