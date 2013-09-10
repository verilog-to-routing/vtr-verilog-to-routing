/*********************************************************************
 *  The following code is part of the power modelling feature of VTR.
 *
 * For support:
 * http://code.google.com/p/vtr-verilog-to-routing/wiki/Power
 *
 * or email:
 * vtr.power.estimation@gmail.com
 *
 * If you are using power estimation for your researach please cite:
 *
 * Jeffrey Goeders and Steven Wilton.  VersaPower: Power Estimation
 * for Diverse FPGA Architectures.  In International Conference on
 * Field Programmable Technology, 2012.
 *
 ********************************************************************/

/**
 * This file provides utility functions used by power estimation.
 */

/************************* INCLUDES *********************************/
#include <cstring>
#include <map>
using namespace std;

#include <assert.h>

#include "power_util.h"
#include "globals.h"

/************************* GLOBALS **********************************/

/************************* FUNCTION DECLARATIONS*********************/
static void log_msg(t_log * log_ptr, char * msg);
static void int_2_binary_str(char * binary_str, int value, int str_length);
static void init_mux_arch_default(t_mux_arch * mux_arch, int levels,
		int num_inputs, float transistor_size);
static void alloc_and_load_mux_graph_recursive(t_mux_node * node,
		int num_primary_inputs, int level, int starting_pin_idx);
static t_mux_node * alloc_and_load_mux_graph(int num_inputs, int levels);

/************************* FUNCTION DEFINITIONS *********************/
void power_zero_usage(t_power_usage * power_usage) {
	power_usage->dynamic = 0.;
	power_usage->leakage = 0.;
}

void power_add_usage(t_power_usage * dest, const t_power_usage * src) {
	dest->dynamic += src->dynamic;
	dest->leakage += src->leakage;
}

void power_scale_usage(t_power_usage * power_usage, float scale_factor) {
	power_usage->dynamic *= scale_factor;
	power_usage->leakage *= scale_factor;
}

float power_sum_usage(t_power_usage * power_usage) {
	return power_usage->dynamic + power_usage->leakage;
}

float power_perc_dynamic(t_power_usage * power_usage) {
	return power_usage->dynamic / power_sum_usage(power_usage);
}

void power_log_msg(e_power_log_type log_type, char * msg) {
	log_msg(&g_power_output->logs[log_type], msg);
}

char * transistor_type_name(e_tx_type type) {
	if (type == NMOS) {
		return "NMOS";
	} else if (type == PMOS) {
		return "PMOS";
	} else {
		return "Unknown";
	}
}

float pin_dens(t_pb * pb, t_pb_graph_pin * pin) {
	float density = 0.;

	if (pb) {
		int net_num;
		net_num = pb->rr_graph[pin->pin_count_in_cluster].net_num;

		if (net_num != OPEN) {
			density = vpack_net_power[net_num].density;
		}
	}

	return density;
}

float pin_prob(t_pb * pb, t_pb_graph_pin * pin) {
	/* Assumed pull-up on unused interconnect */
	float prob = 1.;

	if (pb) {
		int net_num;
		net_num = pb->rr_graph[pin->pin_count_in_cluster].net_num;

		if (net_num != OPEN) {
			prob = vpack_net_power[net_num].probability;
		}
	}

	return prob;
}

/**
 * This function determines the values of the selectors in a static mux, based
 * on the routing information.
 * - selector_values: (Return values) selected index at each mux level
 * - mux_node:
 * - selected_input_pin: The input index to the multi-level mux that is chosen
 */
boolean mux_find_selector_values(int * selector_values, t_mux_node * mux_node,
		int selected_input_pin) {
	if (mux_node->level == 0) {
		if ((selected_input_pin >= mux_node->starting_pin_idx)
				&& (selected_input_pin
						<= (mux_node->starting_pin_idx + mux_node->num_inputs))) {
			selector_values[mux_node->level] = selected_input_pin
					- mux_node->starting_pin_idx;
			return TRUE;
		}
	} else {
		int input_idx;
		for (input_idx = 0; input_idx < mux_node->num_inputs; input_idx++) {
			if (mux_find_selector_values(selector_values,
					&mux_node->children[input_idx], selected_input_pin)) {
				selector_values[mux_node->level] = input_idx;
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void log_msg(t_log * log_ptr, char * msg) {
	int msg_idx;

	/* Check if this message is already in the log */
	for (msg_idx = 0; msg_idx < log_ptr->num_messages; msg_idx++) {
		if (strcmp(log_ptr->messages[msg_idx], msg) == 0) {
			return;
		}
	}

	if (log_ptr->num_messages <= MAX_LOGS) {
		log_ptr->num_messages++;
		log_ptr->messages = (char**) my_realloc(log_ptr->messages,
				log_ptr->num_messages * sizeof(char*));
	} else {
		/* Can't add any more messages */
		return;
	}

	if (log_ptr->num_messages == (MAX_LOGS + 1)) {
		const char * full_msg = "\n***LOG IS FULL***\n";
		log_ptr->messages[log_ptr->num_messages - 1] = (char*) my_calloc(
				strlen(full_msg) + 1, sizeof(char));
		strncpy(log_ptr->messages[log_ptr->num_messages - 1], full_msg,
				strlen(full_msg));
	} else {
		log_ptr->messages[log_ptr->num_messages - 1] = (char*) my_calloc(
				strlen(msg) + 1, sizeof(char));
		strncpy(log_ptr->messages[log_ptr->num_messages - 1], msg, strlen(msg));
	}
}

/**
 * Calculates the number of buffer stages required, to achieve a given buffer fanout
 * final_stage_size: Size of the final inverter in the buffer, relative to a min size
 * desired_stage_effort: The desired gain between stages, typically 4
 */
int power_calc_buffer_num_stages(float final_stage_size,
		float desired_stage_effort) {
	int N = 1;

	if (final_stage_size <= 1.0) {
		N = 1;
	} else if (final_stage_size < desired_stage_effort)
		N = 2;
	else {
		N = (int) (log(final_stage_size) / log(desired_stage_effort) + 1);

		/* We always round down.
		 * Perhaps N+1 would be closer to the desired stage effort, but the delay savings
		 * would likely not be worth the extra power/area
		 */
	}

	return N;
}

/**
 * Calculates the required effort of each stage of a buffer
 * - N: The number of stages of the buffer
 * - final_stage_size: Size of the final inverter in the buffer, relative to a min size
 */
float calc_buffer_stage_effort(int N, float final_stage_size) {
	if (N > 1)
		return pow((double) final_stage_size, (1.0 / ((double) N - 1)));
	else
		return 1.0;
}

#if 0
void integer_to_SRAMvalues(int SRAM_num, int input_integer, char SRAM_values[]) {
	char binary_str[20];
	int binary_str_counter;
	int local_integer;
	int i;

	binary_str_counter = 0;

	local_integer = input_integer;

	while (local_integer > 0) {
		if (local_integer % 2 == 0) {
			SRAM_values[binary_str_counter++] = '0';
		} else {
			SRAM_values[binary_str_counter++] = '1';
		}
		local_integer = local_integer / 2;
	}

	while (binary_str_counter < SRAM_num) {
		SRAM_values[binary_str_counter++] = '0';
	}

	SRAM_values[binary_str_counter] = '\0';

	for (i = 0; i < binary_str_counter; i++) {
		binary_str[i] = SRAM_values[binary_str_counter - 1 - i];
	}

	binary_str[binary_str_counter] = '\0';

}
#endif

/**
 * This function converts an integer to a binary string
 * - binary_str: (Return value) The created binary string
 * - value: The integer value to convert
 * - str_length: The length of the binary string
 */
static void int_2_binary_str(char * binary_str, int value, int str_length) {
	int i;
	int odd;

	binary_str[str_length] = '\0';

	for (i = str_length - 1; i >= 0; i--, value >>= 1) {
		odd = value % 2;
		if (odd == 0) {
			binary_str[i] = '0';
		} else {
			binary_str[i] = '1';
		}
	}
}

/**
 * This functions returns the LUT SRAM values from the given logic terms
 *  - LUT_size: The number of LUT inputs
 *  - truth_table: The logic terms saved from the BLIF file, in a linked list format
 */
char * alloc_SRAM_values_from_truth_table(int LUT_size,
		t_linked_vptr * truth_table) {
	char * SRAM_values;
	int i;
	int num_SRAM_bits;
	char * binary_str;
	char ** terms;
	char * buffer;
	char * str_loc;
	boolean on_set;
	t_linked_vptr * list_ptr;
	int num_terms;
	int term_idx;
	int bit_idx;
	int dont_care_start_pos;

	num_SRAM_bits = 1 << LUT_size;
	SRAM_values = (char*) my_calloc(num_SRAM_bits + 1, sizeof(char));
	SRAM_values[num_SRAM_bits] = '\0';

	if (!truth_table) {
		for (i = 0; i < num_SRAM_bits; i++) {
			SRAM_values[i] = '1';
		}
		return SRAM_values;
	}

	binary_str = (char*) my_calloc(LUT_size + 1, sizeof(char));
	buffer = (char*) my_calloc(LUT_size + 10, sizeof(char));

	strcpy(buffer, (char*) truth_table->data_vptr);

	/* Check if this is an unconnected node - hopefully these will be
	 * ignored by VPR in the future
	 */
	if (strcmp(buffer, " 0") == 0) {
		free(binary_str);
		free(buffer);
		return SRAM_values;
	} else if (strcmp(buffer, " 1") == 0) {
		for (i = 0; i < num_SRAM_bits; i++) {
			SRAM_values[i] = '1';
		}
		free(binary_str);
		free(buffer);
		return SRAM_values;
	}

	/* If the LUT is larger than the terms, the lower significant bits will be don't cares */
	str_loc = strtok(buffer, " \t");
	dont_care_start_pos = strlen(str_loc);

	/* Find out if the truth table provides the ON-set or OFF-set */
	str_loc = strtok(NULL, " \t");
	on_set = TRUE;
	if (str_loc[0] == '1') {
	} else if (str_loc[0] == '0') {
		on_set = FALSE;
	} else {
		assert(0);
	}

	/* Count truth table terms */
	num_terms = 0;
	for (list_ptr = truth_table; list_ptr != NULL; list_ptr = list_ptr->next) {
		num_terms++;
	}
	terms = (char**) my_calloc(num_terms, sizeof(char *));

	/* Extract truth table terms */
	for (list_ptr = truth_table, term_idx = 0; list_ptr != NULL; list_ptr =
			list_ptr->next, term_idx++) {
		terms[term_idx] = (char*) my_calloc(LUT_size + 1, sizeof(char));

		strcpy(buffer, (char*) list_ptr->data_vptr);
		str_loc = strtok(buffer, " \t");
		strcpy(terms[term_idx], str_loc);

		/* Fill don't cares for lower bits (when LUT is larger than term size) */
		for (bit_idx = dont_care_start_pos; bit_idx < LUT_size; bit_idx++) {
			terms[term_idx][bit_idx] = '-';
		}

		/* Verify on/off consistency */
		str_loc = strtok(NULL, " \t");
		if (on_set) {
			assert(str_loc[0] == '1');
		} else {
			assert(str_loc[0] == '0');
		}
	}

	/* Loop through all SRAM bits */
	for (i = 0; i < num_SRAM_bits; i++) {
		/* Set default value */
		if (on_set) {
			SRAM_values[i] = '0';
		} else {
			SRAM_values[i] = '1';
		}

		/* Get binary number representing this SRAM index */
		int_2_binary_str(binary_str, i, LUT_size);

		/* Loop through truth table terms */
		for (term_idx = 0; term_idx < num_terms; term_idx++) {
			boolean match = TRUE;

			for (bit_idx = 0; bit_idx < LUT_size; bit_idx++) {
				if ((terms[term_idx][bit_idx] != '-')
						&& (terms[term_idx][bit_idx] != binary_str[bit_idx])) {
					match = FALSE;
					break;
				}
			}

			if (match) {
				if (on_set) {
					SRAM_values[i] = '1';
				} else {
					SRAM_values[i] = '0';
				}

				/* No need to check the other terms, already matched */
				break;
			}
		}

	}
	free(binary_str);
	free(buffer);
	for (term_idx = 0; term_idx < num_terms; term_idx++) {
		free(terms[term_idx]);
	}
	free(terms);

	return SRAM_values;
}

/* Reduce mux levels for multiplexers that are too small for the preset number of levels */
void mux_arch_fix_levels(t_mux_arch * mux_arch) {
	while (((1 << mux_arch->levels) > mux_arch->num_inputs)
			&& (mux_arch->levels > 1)) {
		mux_arch->levels--;
	}
}

float clb_net_density(int net_idx) {
	if (net_idx == OPEN) {
		return 0.;
	} else {
		return clb_net_power[net_idx].density;
	}
}

float clb_net_prob(int net_idx) {
	if (net_idx == OPEN) {
		return 0.;
	} else {
		return clb_net_power[net_idx].probability;
	}
}

char * interconnect_type_name(enum e_interconnect type) {
	switch (type) {
	case COMPLETE_INTERC:
		return "complete";
	case MUX_INTERC:
		return "mux";
	case DIRECT_INTERC:
		return "direct";
	default:
		return "";
	}
}

void output_log(t_log * log_ptr, FILE * fp) {
	int msg_idx;

	for (msg_idx = 0; msg_idx < log_ptr->num_messages; msg_idx++) {
		fprintf(fp, "%s\n", log_ptr->messages[msg_idx]);
	}
}

void output_logs(FILE * fp, t_log * logs, int num_logs) {
	int log_idx;

	for (log_idx = 0; log_idx < num_logs; log_idx++) {
		if (logs[log_idx].num_messages) {
			power_print_title(fp, logs[log_idx].name);
			output_log(&logs[log_idx], fp);
			fprintf(fp, "\n");
		}
	}
}

float power_buffer_size_from_logical_effort(float C_load) {
	return max(1.0f,
			C_load / g_power_commonly_used->INV_1X_C_in
					/ (2 * g_power_arch->logical_effort_factor));
}

void power_print_title(FILE * fp, char * title) {
	int i;
	const int width = 80;

	int firsthalf = (width - strlen(title) - 2) / 2;
	int secondhalf = width - strlen(title) - 2 - firsthalf;

	for (i = 1; i <= firsthalf; i++)
		fprintf(fp, "-");
	fprintf(fp, " %s ", title);
	for (i = 1; i <= secondhalf; i++)
		fprintf(fp, "-");
	fprintf(fp, "\n");
}

t_mux_arch * power_get_mux_arch(int num_mux_inputs, float transistor_size) {
	int i;

	t_power_mux_info * mux_info = NULL;

	/* Find the mux archs for the given transistor size */
	std::map<float, t_power_mux_info*>::iterator it;

	it = g_power_commonly_used->mux_info.find(transistor_size);

	if (it == g_power_commonly_used->mux_info.end()) {
		mux_info = new t_power_mux_info;
		mux_info->mux_arch = NULL;
		mux_info->mux_arch_max_size = 0;
		g_power_commonly_used->mux_info[transistor_size] = mux_info;
	} else {
		mux_info = it->second;
	}

	if (num_mux_inputs > mux_info->mux_arch_max_size) {
		mux_info->mux_arch = (t_mux_arch*) my_realloc(mux_info->mux_arch,
				(num_mux_inputs + 1) * sizeof(t_mux_arch));

		for (i = mux_info->mux_arch_max_size + 1; i <= num_mux_inputs; i++) {
			init_mux_arch_default(&mux_info->mux_arch[i], 2, i,
					transistor_size);
		}
		mux_info->mux_arch_max_size = num_mux_inputs;
	}
	return &mux_info->mux_arch[num_mux_inputs];
}

/**
 * Generates a default multiplexer architecture of given size and number of levels
 */
static void init_mux_arch_default(t_mux_arch * mux_arch, int levels,
		int num_inputs, float transistor_size) {

	mux_arch->levels = levels;
	mux_arch->num_inputs = num_inputs;

	mux_arch_fix_levels(mux_arch);

	mux_arch->transistor_size = transistor_size;

	mux_arch->mux_graph_head = alloc_and_load_mux_graph(num_inputs,
			mux_arch->levels);
}

/**
 * Allocates a builds a multiplexer graph with given # inputs and levels
 */
static t_mux_node * alloc_and_load_mux_graph(int num_inputs, int levels) {
	t_mux_node * node;

	node = (t_mux_node*) my_malloc(sizeof(t_mux_node));
	alloc_and_load_mux_graph_recursive(node, num_inputs, levels - 1, 0);

	return node;
}

static void alloc_and_load_mux_graph_recursive(t_mux_node * node,
		int num_primary_inputs, int level, int starting_pin_idx) {
	int child_idx;
	int pin_idx = starting_pin_idx;

	node->num_inputs = (int) (pow(num_primary_inputs, 1 / ((float) level + 1))
			+ 0.5);
	node->level = level;
	node->starting_pin_idx = starting_pin_idx;

	if (level != 0) {
		node->children = (t_mux_node*) my_calloc(node->num_inputs,
				sizeof(t_mux_node));
		for (child_idx = 0; child_idx < node->num_inputs; child_idx++) {
			int num_child_pi = num_primary_inputs / node->num_inputs;
			if (child_idx < (num_primary_inputs % node->num_inputs)) {
				num_child_pi++;
			}
			alloc_and_load_mux_graph_recursive(&node->children[child_idx],
					num_child_pi, level - 1, pin_idx);
			pin_idx += num_child_pi;
		}
	}
}

boolean power_method_is_transistor_level(
		e_power_estimation_method estimation_method) {
	switch (estimation_method) {
	case POWER_METHOD_AUTO_SIZES:
	case POWER_METHOD_SPECIFY_SIZES:
		return TRUE;
	default:
		return FALSE;
	}
}

boolean power_method_is_recursive(e_power_estimation_method method) {
	switch (method) {
	case POWER_METHOD_IGNORE:
	case POWER_METHOD_TOGGLE_PINS:
	case POWER_METHOD_C_INTERNAL:
	case POWER_METHOD_ABSOLUTE:
		return FALSE;
	case POWER_METHOD_AUTO_SIZES:
	case POWER_METHOD_SPECIFY_SIZES:
	case POWER_METHOD_SUM_OF_CHILDREN:
		return TRUE;
	case POWER_METHOD_UNDEFINED:
	default:
		assert(0);
	}

// to get rid of warning
	return FALSE;
}

