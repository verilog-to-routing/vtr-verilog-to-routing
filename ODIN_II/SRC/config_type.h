typedef struct config_t_t config_t;
struct config_t_t
{
	char **list_of_file_names;
	int num_list_of_file_names;

	char *output_type; // string name of the type of output file

	char *debug_output_path; // path for where to output the debug outputs
	short output_ast_graphs; // switch that outputs ast graphs per node for use with GRaphViz tools
	short output_netlist_graphs; // switch that outputs netlist graphs per node for use with GraphViz tools
	short print_parse_tokens; // switch that controls whether or not each token is printed during parsing
	short output_preproc_source; // switch that outputs the pre-processed source
	int min_hard_multiplier; // threshold from hard to soft logic
	int mult_padding; // setting how multipliers are padded to fit fixed size
	// Flag for fixed or variable hard mult (1 or 0)
	int fixed_hard_multiplier;
	// Flag for splitting hard multipliers If fixed_hard_multiplier is set, this must be 1.
	int split_hard_multiplier;
	// 1 to split memory width down to a size of 1. 0 to split to arch width.
	char split_memory_width;
	// Set to a positive integer to split memory depth to that address width. 0 to split to arch width.
	int split_memory_depth;

	//add by Sen
	// Threshold from hard to soft logic(extra bits)
	int min_hard_adder;
	int add_padding; // setting how multipliers are padded to fit fixed size
	// Flag for fixed or variable hard mult (1 or 0)
	int fixed_hard_adder;
	// Flag for splitting hard multipliers If fixed_hard_multiplier is set, this must be 1.
	int split_hard_adder;
	//  Threshold from hard to soft logic
	int min_threshold_adder;

	// If the memory is smaller than both of these, it will be converted to soft logic.
	int soft_logic_memory_depth_threshold;
	int soft_logic_memory_width_threshold;

	char *arch_file; // Name of the FPGA architecture file
};