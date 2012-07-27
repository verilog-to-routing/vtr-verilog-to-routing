#include <string.h>
#include <stdio.h>
#include <time.h>
#include "assert.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "read_blif.h"
#include "arch_types.h"
#include "ReadOptions.h"
#include "hash.h"

/* PRINT_PIN_NETS */

struct s_model_stats {t_model * model; int count;};

#define MAX_ATOM_PARSE 200000000

/* This source file will read in a FLAT blif netlist consisting     *
 * of .inputs, .outputs, .names and .latch commands.  It currently   *
 * does not handle hierarchical blif files.  Hierarchical            *
 * blif files can be flattened via the read_blif and write_blif      *
 * commands of sis.  LUT circuits should only have .names commands;  *
 * there should be no gates.  This parser performs limited error     *
 * checking concerning the consistency of the netlist it obtains.    *
 * .inputs and .outputs statements must be given; this parser does   *
 * not infer primary inputs and outputs from non-driven and fanout   *
 * free nodes.  This parser can be extended to do this if necessary, *
 * or the sis read_blif and write_blif commands can be used to put a *
 * netlist into the standard format.                                 *
 * V. Betz, August 25, 1994.                                         *
 * Added more error checking, March 30, 1995, V. Betz                */

static int *num_driver, *temp_num_pins;
static int *logical_block_input_count, *logical_block_output_count;
static int num_blif_models;
static int num_luts = 0, num_latches = 0, num_subckts = 0;

/* # of .input, .output, .model and .end lines */
static int ilines, olines, model_lines, endlines;
static struct s_hash **blif_hash;
static char *model;
static FILE *blif;

static int add_vpack_net(char *ptr, int type, int bnum, int bport, int bpin,
		boolean is_global, int doall);
static void get_blif_tok(char *buffer, int doall, boolean *done,
		boolean *add_truth_table, INP t_model* inpad_model,
		INP t_model* outpad_model, INP t_model* logic_model,
		INP t_model* latch_model, INP t_model* user_models);
static void init_parse(int doall);
static void check_net(boolean sweep_hanging_nets_and_inputs);
static void free_parse(void);
static void io_line(int in_or_out, int doall, t_model *io_model);
static boolean add_lut(int doall, t_model *logic_model);
static void add_latch(int doall, INP t_model *latch_model);
static void add_subckt(int doall, INP t_model *user_models);
static void check_and_count_models(int doall, const char* model_name,
		t_model* user_models);
static void load_default_models(INP t_model *library_models,
		OUTP t_model** inpad_model, OUTP t_model** outpad_model,
		OUTP t_model** logic_model, OUTP t_model** latch_model);

static void read_blif (char *blif_file, boolean sweep_hanging_nets_and_inputs,
		t_model *user_models, t_model *library_models);

static void absorb_buffer_luts(void);
static void compress_netlist(void);
static void show_blif_stats(t_model *user_models, t_model *library_models);



static void read_blif (char *blif_file, boolean sweep_hanging_nets_and_inputs,
		t_model *user_models, t_model *library_models) {
	char buffer[BUFSIZE];
	int doall;
	boolean done;
	boolean add_truth_table;
	t_model *inpad_model, *outpad_model, *logic_model, *latch_model;
	clock_t begin,end;

	blif = fopen(blif_file, "r");
	if (blif == NULL) {
		vpr_printf(TIO_MESSAGE_ERROR, "Failed to open blif file %s\n", blif_file);
		exit(1);
	}
	load_default_models(library_models, &inpad_model, &outpad_model,
			&logic_model, &latch_model);

	/* doall = 0 means do a counting pass, doall = 1 means allocate and load data structures */
	for (doall = 0; doall <= 1; doall++) {
		begin = clock();

		init_parse(doall);
		
		end = clock();
		#ifdef CLOCKS_PER_SEC
			vpr_printf(TIO_MESSAGE_INFO, "Loop for doall = %d, init_parse took %g seconds.\n", doall, (float)(end - begin) / CLOCKS_PER_SEC);
		#else
			vpr_printf(TIO_MESSAGE_INFO, "Loop for doall = %d, init_parse took %g seconds.\n", doall, (float)(end - begin) / CLK_PER_SEC);
		#endif
		
		begin = clock();
		file_line_number = 0; /* Reset line number. */
		done = FALSE;
		add_truth_table = FALSE;
		model_lines = 0;
		while (my_fgets(buffer, BUFSIZE, blif) != NULL) {
			get_blif_tok(buffer, doall, &done, &add_truth_table,
					inpad_model, outpad_model, logic_model, latch_model,
					user_models);
		}
		rewind(blif); /* Start at beginning of file again */

		end = clock();
		#ifdef CLOCKS_PER_SEC
			vpr_printf(TIO_MESSAGE_INFO, "Loop for doall = %d took %g seconds.\n", doall, (float)(end - begin) / CLOCKS_PER_SEC);
		#else
			vpr_printf(TIO_MESSAGE_INFO, "Loop for doall = %d took %g seconds.\n", doall, (float)(end - begin) / CLK_PER_SEC);
		#endif

	}

	
	/*checks how well the hash function is performing*/
	get_hash_stats(blif_hash, "blif_hash");

	fclose(blif);
	check_net(sweep_hanging_nets_and_inputs);
	free_parse();
}

static void init_parse(int doall) {

	/* Allocates and initializes the data structures needed for the parse. */

	int i;
	struct s_hash *h_ptr;

	if (!doall) { /* Initialization before first (counting) pass */
		num_logical_nets = 0;
		blif_hash = (struct s_hash **) my_calloc(
				sizeof(struct s_hash *), HASHSIZE);
	}
	/* Allocate memory for second (load) pass */
	else {
		vpack_net = (struct s_net *) my_calloc(num_logical_nets,
				sizeof(struct s_net));
		logical_block = (struct s_logical_block *) my_calloc(num_logical_blocks,
				sizeof(struct s_logical_block));
		num_driver = (int *) my_malloc(num_logical_nets * sizeof(int));
		temp_num_pins = (int *) my_malloc(num_logical_nets * sizeof(int));

		logical_block_input_count = (int *) my_calloc(num_logical_blocks, sizeof(int));
		logical_block_output_count = (int *) my_calloc(num_logical_blocks, sizeof(int));

		for (i = 0; i < num_logical_nets; i++) {
			num_driver[i] = 0;
			vpack_net[i].num_sinks = 0;
			vpack_net[i].name = NULL;
			vpack_net[i].node_block = NULL;
			vpack_net[i].node_block_port = NULL;
			vpack_net[i].node_block_pin = NULL;
			vpack_net[i].is_global = FALSE;
		}

		for (i = 0; i < num_logical_blocks; i++) {
			logical_block[i].index = i;
		}

		for (i = 0; i < HASHSIZE; i++) {
			h_ptr = blif_hash[i];
			while (h_ptr != NULL) {
				vpack_net[h_ptr->index].node_block = (int *) my_malloc(
						h_ptr->count * sizeof(int));
				vpack_net[h_ptr->index].node_block_port = (int *) my_malloc(
						h_ptr->count * sizeof(int));
				vpack_net[h_ptr->index].node_block_pin = (int *) my_malloc(
						h_ptr->count * sizeof(int));

				/* For avoiding assigning values beyond end of pins array. */
				temp_num_pins[h_ptr->index] = h_ptr->count;
				vpack_net[h_ptr->index].name = my_strdup(h_ptr->name);
				h_ptr = h_ptr->next;
			}
		}
#ifdef PRINT_PIN_NETS
		vpr_printf(TIO_MESSAGE_INFO, "i\ttemp_num_pins\n\n");
		for (i = 0;i < num_logical_nets;i++) {
			vpr_printf(TIO_MESSAGE_INFO, "%d\t%d\n",i,temp_num_pins[i]);
		}
		vpr_printf(TIO_MESSAGE_INFO, "num_logical_nets %d\n", num_logical_nets);
#endif
	}

	/* Initializations for both passes. */

	ilines = 0;
	olines = 0;
	model_lines = 0;
	endlines = 0;
	num_p_inputs = 0;
	num_p_outputs = 0;
	num_luts = 0;
	num_latches = 0;
	num_logical_blocks = 0;
	num_blif_models = 0;
	num_subckts = 0;
}

static void get_blif_tok(char *buffer, int doall, boolean *done,
		boolean *add_truth_table, INP t_model* inpad_model,
		INP t_model* outpad_model, INP t_model* logic_model,
		INP t_model* latch_model, INP t_model* user_models) {

	/* Figures out which, if any token is at the start of this line and *
	 * takes the appropriate action.                                    */

#define BLIF_TOKENS " \t\n"
	char *ptr;
	char *fn;
	struct s_linked_vptr *data;

	ptr = my_strtok(buffer, TOKENS, blif, buffer);
	if (ptr == NULL)
		return;

	if (*add_truth_table) {
		if (ptr[0] == '0' || ptr[0] == '1' || ptr[0] == '-') {
			data = (struct s_linked_vptr*)my_malloc(sizeof(struct s_linked_vptr));
			fn = ptr;
			ptr = my_strtok(NULL, BLIF_TOKENS, blif, buffer);
			if (!ptr || strlen(ptr) != 1) {
				if (strlen(fn) == 1) {
					/* constant generator */
					data->next =
							logical_block[num_logical_blocks - 1].truth_table;
					data->data_vptr = my_malloc(strlen(fn) + 4);
					sprintf((char*)(data->data_vptr), " %s", fn);
					logical_block[num_logical_blocks - 1].truth_table = data;
					ptr = fn;
				} else {
					vpr_printf(TIO_MESSAGE_ERROR, "Unknown truth table data %s %s\n", fn, ptr);
					exit(1);
				}
			} else {
				data->next = logical_block[num_logical_blocks - 1].truth_table;
				data->data_vptr = my_malloc(strlen(fn) + 3);
				sprintf((char*)data->data_vptr, "%s %s", fn, ptr);
				logical_block[num_logical_blocks - 1].truth_table = data;
			}
		}
	}

	if (strcmp(ptr, ".names") == 0) {
		*add_truth_table = FALSE;
		*add_truth_table = add_lut(doall, logic_model);
		return;
	}

	if (strcmp(ptr, ".latch") == 0) {
		*add_truth_table = FALSE;
		add_latch(doall, latch_model);
		return;
	}

	if (strcmp(ptr, ".model") == 0) {
		*add_truth_table = FALSE;
		ptr = my_strtok(NULL, TOKENS, blif, buffer);
		if (doall) {
			if (ptr != NULL) {
				model = (char *) my_malloc((strlen(ptr) + 1) * sizeof(char));
				strcpy(model, ptr);
				if (blif_circuit_name == NULL) {
					blif_circuit_name = my_strdup(model);
				}
			} else {
				model = (char *) my_malloc(sizeof(char));
				model[0] = '\0';
			}
		}

		if (model_lines > 0) {
			check_and_count_models(doall, ptr, user_models);
		} else {
			dum_parse(buffer);
		}
		model_lines++;
		return;
	}

	if (strcmp(ptr, ".inputs") == 0) {
		*add_truth_table = FALSE;
		/* packing can only one fully defined model */
		if (model_lines == 1) {
			io_line(DRIVER, doall, inpad_model);
			*done = (boolean)1;
		} 
		if (doall)
			ilines++; /* Error checking only */
		return;
	}

	if (strcmp(ptr, ".outputs") == 0) {
		*add_truth_table = FALSE;
		/* packing can only one fully defined model */
		if (model_lines == 1) {
			io_line(RECEIVER, doall, outpad_model);
			*done = (boolean)1;
		} 
		if (doall)
			olines++; /* Make sure only one .output line */
		 /* For error checking only */
		return;
	}
	if (strcmp(ptr, ".end") == 0) {
		*add_truth_table = FALSE;
		if (doall) {
			endlines++; /* Error checking only */
		}
		return;
	}

	if (strcmp(ptr, ".subckt") == 0) {
		*add_truth_table = FALSE;
		add_subckt(doall, user_models);
	}

	/* Could have numbers following a .names command, so not matching any *
	 * of the tokens above is not an error.                               */

}

void dum_parse(char *buf) {

	/* Continue parsing to the end of this (possibly continued) line. */

	while (my_strtok(NULL, TOKENS, blif, buf) != NULL)
		;
}

static boolean add_lut(int doall, t_model *logic_model) {

	/* Adds a LUT as VPACK_COMB from (.names) currently being parsed to the logical_block array.  Adds *
	 * its pins to the nets data structure by calling add_vpack_net.  If doall is *
	 * zero this is a counting pass; if it is 1 this is the final (loading) *
	 * pass.                                                                */

	char *ptr, **saved_names, buf[BUFSIZE];
	int i, j, output_net_index;

	saved_names = (char**) alloc_matrix(0, logic_model->inputs->size, 0,
			BUFSIZE - 1, sizeof(char));

	num_logical_blocks++;

	/* Count # nets connecting */
	i = 0;
	while ((ptr = my_strtok(NULL, TOKENS, blif, buf)) != NULL) {
		if (i > logic_model->inputs->size) {
			vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] .names %s ... %s has a LUT size that exceeds the maximum LUT size (%d) of the architecture\n", file_line_number, saved_names[0], ptr, logic_model->inputs->size);
			exit(1);
		}
		strcpy(saved_names[i], ptr);
		i++;
	}
	output_net_index = i - 1;
	if (strcmp(saved_names[output_net_index], "unconn") == 0) {
		/* unconn is a keyword to pad unused pins, ignore this block */
		free_matrix(saved_names, 0, logic_model->inputs->size, 0, sizeof(char));
		num_logical_blocks--;
		return FALSE;
	}

	if (!doall) { /* Counting pass only ... */
		for (j = 0; j <= output_net_index; j++)
			/* On this pass it doesn't matter if RECEIVER or DRIVER.  Just checking if in hash.  [0] should be DRIVER */
			add_vpack_net(saved_names[j], RECEIVER, num_logical_blocks - 1, 0,
					j, FALSE, doall);
		free_matrix(saved_names, 0, logic_model->inputs->size, 0, sizeof(char));
		return FALSE;
	}

	logical_block[num_logical_blocks - 1].model = logic_model;

	if (output_net_index > logic_model->inputs->size) {
		vpr_printf(
				TIO_MESSAGE_ERROR, "LUT size of %d in .blif file is too big for FPGA which has a maximum LUT size of %d\n",
				output_net_index, logic_model->inputs->size);
		exit(1);
	}
	assert(logic_model->inputs->next == NULL);
	assert(logic_model->outputs->next == NULL);
	assert(logic_model->outputs->size == 1);

	logical_block[num_logical_blocks - 1].input_nets = (int **) my_malloc(
			sizeof(int*));
	logical_block[num_logical_blocks - 1].output_nets = (int **) my_malloc(
			sizeof(int*));
	logical_block[num_logical_blocks - 1].clock_net = OPEN;

	logical_block[num_logical_blocks - 1].input_nets[0] = (int *) my_malloc(
			logic_model->inputs->size * sizeof(int));
	logical_block[num_logical_blocks - 1].output_nets[0] = (int *) my_malloc(
			sizeof(int));

	logical_block[num_logical_blocks - 1].type = VPACK_COMB;
	for (i = 0; i < output_net_index; i++) /* Do inputs */
		logical_block[num_logical_blocks - 1].input_nets[0][i] = add_vpack_net(
				saved_names[i], RECEIVER, num_logical_blocks - 1, 0, i, FALSE,
				doall);
	logical_block[num_logical_blocks - 1].output_nets[0][0] = add_vpack_net(
			saved_names[output_net_index], DRIVER, num_logical_blocks - 1, 0, 0,
			FALSE, doall);

	for (i = output_net_index; i < logic_model->inputs->size; i++)
		logical_block[num_logical_blocks - 1].input_nets[0][i] = OPEN;

	logical_block[num_logical_blocks - 1].name = my_strdup(
			saved_names[output_net_index]);
	logical_block[num_logical_blocks - 1].truth_table = NULL;
	num_luts++;

	free_matrix(saved_names, 0, logic_model->inputs->size, 0, sizeof(char));
	return (boolean)doall;
}

static void add_latch(int doall, INP t_model *latch_model) {

	/* Adds the flipflop (.latch) currently being parsed to the logical_block array.  *
	 * Adds its pins to the nets data structure by calling add_vpack_net.  If doall *
	 * is zero this is a counting pass; if it is 1 this is the final          * 
	 * (loading) pass.  Blif format for a latch is:                           *
	 * .latch <input> <output> <type (latch on)> <control (clock)> <init_val> *
	 * The latch pins are in .nets 0 to 2 in the order: Q D CLOCK.            */

	char *ptr, buf[BUFSIZE], saved_names[6][BUFSIZE];
	int i;

	num_logical_blocks++;

	/* Count # parameters, making sure we don't go over 6 (avoids memory corr.) */
	/* Note that we can't rely on the tokens being around unless we copy them.  */

	for (i = 0; i < 6; i++) {
		ptr = my_strtok(NULL, TOKENS, blif, buf);
		if (ptr == NULL)
			break;
		strcpy(saved_names[i], ptr);
	}

	if (i != 5) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Error:  .latch does not have 5 parameters.\n" "check the netlist, line %d.\n",
				file_line_number);
		exit(1);
	}

	if (!doall) { /* If only a counting pass ... */
		add_vpack_net(saved_names[0], RECEIVER, num_logical_blocks - 1, 0, 0,
				FALSE, doall); /* D */
		add_vpack_net(saved_names[1], DRIVER, num_logical_blocks - 1, 0, 0,
				FALSE, doall); /* Q */
		add_vpack_net(saved_names[3], RECEIVER, num_logical_blocks - 1, 0, 0,
				TRUE, doall); /* Clock */
		return;
	}

	logical_block[num_logical_blocks - 1].model = latch_model;
	logical_block[num_logical_blocks - 1].type = VPACK_LATCH;

	logical_block[num_logical_blocks - 1].input_nets = (int **) my_malloc(
			sizeof(int*));
	logical_block[num_logical_blocks - 1].output_nets = (int **) my_malloc(
			sizeof(int*));

	logical_block[num_logical_blocks - 1].input_nets[0] = (int *) my_malloc(
			sizeof(int));
	logical_block[num_logical_blocks - 1].output_nets[0] = (int *) my_malloc(
			sizeof(int));

	logical_block[num_logical_blocks - 1].output_nets[0][0] = add_vpack_net(
			saved_names[1], DRIVER, num_logical_blocks - 1, 0, 0, FALSE, doall); /* Q */
	logical_block[num_logical_blocks - 1].input_nets[0][0] = add_vpack_net(
			saved_names[0], RECEIVER, num_logical_blocks - 1, 0, 0, FALSE,
			doall); /* D */
	logical_block[num_logical_blocks - 1].clock_net = add_vpack_net(
			saved_names[3], RECEIVER, num_logical_blocks - 1, 0, 0, TRUE,
			doall); /* Clock */

	logical_block[num_logical_blocks - 1].name = my_strdup(saved_names[1]);
	logical_block[num_logical_blocks - 1].truth_table = NULL;
	num_latches++;
}

static void add_subckt(int doall, t_model *user_models) {
	char *ptr;
	char *close_bracket;
	char subckt_name[BUFSIZE];
	char buf[BUFSIZE];
	//fpos_t current_subckt_pos;
	int i, j, iparse;
	int subckt_index_signals = 0;
	char **subckt_signal_name = NULL;
	char *port_name, *pin_number;
	char **circuit_signal_name = NULL;
	char *subckt_logical_block_name = NULL;
	short toggle = 0;
	int input_net_count, output_net_count, input_port_count, output_port_count;
	t_model *cur_model;
	t_model_ports *port;
	boolean found_subckt_signal;

	num_logical_blocks++;
	num_subckts++;

	/* now we have to find the matching subckt */
	/* find the name we are looking for */
	strcpy(subckt_name, my_strtok(NULL, TOKENS, blif, buf));
	/* get all the signals in the form z=r */
	iparse = 0;
	while (iparse < MAX_ATOM_PARSE) {
		iparse++;
		/* Assumption is that it will be "signal1, =, signal1b, spacing, and repeat" */
		ptr = my_strtok(NULL, " \t\n=", blif, buf);

		if (ptr == NULL && toggle == 0)
			break;
		else if (ptr == NULL && toggle == 1) {
			vpr_printf(TIO_MESSAGE_ERROR, "subckt %s formed incorrectly with signal=signal at %s\n",
					subckt_name, buf);
			exit(-1);
		} else if (toggle == 0) {
			/* ELSE - parse in one or the other */
			/* allocate a new spot for both the circuit_signal name and the subckt_signal name */
			subckt_signal_name = (char**) my_realloc(subckt_signal_name,
					(subckt_index_signals + 1) * sizeof(char**));
			circuit_signal_name = (char**) my_realloc(circuit_signal_name,
					(subckt_index_signals + 1) * sizeof(char**));

			/* copy in the subckt_signal name */
			subckt_signal_name[subckt_index_signals] = my_strdup(ptr);

			toggle = 1;
		} else if (toggle == 1) {
			/* copy in the circuit_signal name */
			circuit_signal_name[subckt_index_signals] = my_strdup(ptr);
			if (!doall) {
				/* Counting pass, does not matter if driver or receiver and pin number does not matter */
				add_vpack_net(circuit_signal_name[subckt_index_signals],
						RECEIVER, num_logical_blocks - 1, 0, 0, FALSE, doall);
			}

			toggle = 0;
			subckt_index_signals++;
		}
	}
	assert(iparse < MAX_ATOM_PARSE);
	/* record the position of the parse so far so when we resume we will move to the next item */
	//if (fgetpos(blif, &current_subckt_pos) != 0) {
	//	vpr_printf(TIO_MESSAGE_ERROR, "In file pointer read - read_blif.c\n");
	//	exit(-1);
	//}

	input_net_count = 0;
	output_net_count = 0;

	if (doall) {
		/* get the matching model to this subckt */

		cur_model = user_models;
		while (cur_model != NULL) {
			if (strcmp(cur_model->name, subckt_name) == 0) {
				break;
			}
			cur_model = cur_model->next;
		}
		if (cur_model == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "Didn't find matching model to subckt %s\n",
					subckt_name);
			exit(-1);
		}

		/* IF - do all then we need to allocate a string to hold all the subckt info */

		/* initialize the logical_block structure */

		/* record model info */
		logical_block[num_logical_blocks - 1].model = cur_model;

		/* allocate space for inputs and initialize all input nets to OPEN */
		input_port_count = 0;
		port = cur_model->inputs;
		while (port) {
			if (!port->is_clock) {
				input_port_count++;
			}
			port = port->next;
		}
		logical_block[num_logical_blocks - 1].input_nets = (int**)my_malloc(
				input_port_count * sizeof(int *));

		port = cur_model->inputs;
		while (port) {
			if (port->is_clock) {
				/* Clock ports are different from regular input ports, skip */
				port = port->next;
				continue;
			}
			assert(port->size >= 0);
			logical_block[num_logical_blocks - 1].input_nets[port->index] = (int*)
					my_malloc(port->size * sizeof(int));
			for (j = 0; j < port->size; j++) {
				logical_block[num_logical_blocks - 1].input_nets[port->index][j] =
						OPEN;
			}
			port = port->next;
		}
		assert(port == NULL || (port->is_clock && port->next == NULL));

		/* allocate space for outputs and initialize all output nets to OPEN */
		output_port_count = 0;
		port = cur_model->outputs;
		while (port) {
			port = port->next;
			output_port_count++;
		}
		logical_block[num_logical_blocks - 1].output_nets = (int**)my_malloc(
				output_port_count * sizeof(int *));

		port = cur_model->outputs;
		while (port) {
			assert(port->size >= 0);
			logical_block[num_logical_blocks - 1].output_nets[port->index] = (int*)
					my_malloc(port->size * sizeof(int));
			for (j = 0; j < port->size; j++) {
				logical_block[num_logical_blocks - 1].output_nets[port->index][j] =
						OPEN;
			}
			port = port->next;
		}
		assert(port == NULL);

		/* initialize clock data */
		logical_block[num_logical_blocks - 1].clock_net = OPEN;

		logical_block[num_logical_blocks - 1].type = VPACK_COMB;
		logical_block[num_logical_blocks - 1].truth_table = NULL;

		/* setup the index signal if open or not */

		for (i = 0; i < subckt_index_signals; i++) {
			found_subckt_signal = FALSE;
			/* determine the port name and the pin_number of the subckt */
			port_name = my_strdup(subckt_signal_name[i]);
			pin_number = strrchr(port_name, '[');
			if (pin_number == NULL) {
				pin_number = "0"; /* default to 0 */
			} else {
				/* The pin numbering is port_name[pin_number] so need to go one to the right of [ then NULL out ] */
				*pin_number = '\0';
				pin_number++;
				close_bracket = pin_number;
				while (*close_bracket != '\0' && *close_bracket != ']') {
					close_bracket++;
				}
				*close_bracket = '\0';
			}

			port = cur_model->inputs;
			while (port) {
				if (strcmp(port_name, port->name) == 0) {
					if (found_subckt_signal) {
						vpr_printf(
								TIO_MESSAGE_ERROR, "Two instances of %s subckt signal found in subckt %s\n",
								subckt_signal_name[i], subckt_name);
					}
					found_subckt_signal = TRUE;
					if (port->is_clock) {
						assert(
								logical_block[num_logical_blocks-1].clock_net == OPEN);
						assert(my_atoi(pin_number) == 0);
						logical_block[num_logical_blocks - 1].clock_net =
								add_vpack_net(circuit_signal_name[i], RECEIVER,
										num_logical_blocks - 1, port->index,
										my_atoi(pin_number), TRUE, doall);
					} else {
						logical_block[num_logical_blocks - 1].input_nets[port->index][my_atoi(
								pin_number)] = add_vpack_net(
								circuit_signal_name[i], RECEIVER,
								num_logical_blocks - 1, port->index,
								my_atoi(pin_number), FALSE, doall);
						input_net_count++;
					}
				}
				port = port->next;
			}

			port = cur_model->outputs;
			while (port) {
				if (strcmp(port_name, port->name) == 0) {
					if (found_subckt_signal) {
						vpr_printf(
								TIO_MESSAGE_ERROR, "Two instances of %s subckt signal found in subckt %s\n",
								subckt_signal_name[i], subckt_name);
					}
					found_subckt_signal = TRUE;
					logical_block[num_logical_blocks - 1].output_nets[port->index][my_atoi(
							pin_number)] = add_vpack_net(circuit_signal_name[i],
							DRIVER, num_logical_blocks - 1, port->index,
							my_atoi(pin_number), FALSE, doall);
					if (subckt_logical_block_name == NULL
							&& circuit_signal_name[i] != NULL) {
						subckt_logical_block_name = circuit_signal_name[i];
					}
					output_net_count++;
				}
				port = port->next;
			}

			/* record the name to be first output net parsed */
			logical_block[num_logical_blocks - 1].name = my_strdup(
					subckt_logical_block_name);

			if (!found_subckt_signal) {
				vpr_printf(TIO_MESSAGE_ERROR, "Unknown subckt port %s\n", subckt_signal_name[i]);
				exit(1);
			}
		}
	}

	for (i = 0; i < subckt_index_signals; i++) {
		free(subckt_signal_name[i]);
		free(circuit_signal_name[i]);
	}
	free(subckt_signal_name);
	free(circuit_signal_name);

	/* now that you've done the analysis, move the file pointer back */
	//if (fsetpos(blif, &current_subckt_pos) != 0) {
	//	vpr_printf(TIO_MESSAGE_ERROR, "In moving back file pointer - read_blif.c\n");
	//	exit(-1);
	//}
}

static void io_line(int in_or_out, int doall, t_model *io_model) {

	/* Adds an input or output logical_block to the logical_block data structures.           *
	 * in_or_out:  DRIVER for input, RECEIVER for output.                    *
	 * doall:  1 for final pass when structures are loaded.  0 for           *
	 * first pass when hash table is built and pins, nets, etc. are counted. */

	char *ptr;
	char buf2[BUFSIZE];
	int nindex, len, iparse;

	iparse = 0;
	while (iparse < MAX_ATOM_PARSE) {
		iparse++;
		ptr = my_strtok(NULL, TOKENS, blif, buf2);
		if (ptr == NULL)
			return;
		num_logical_blocks++;

		nindex = add_vpack_net(ptr, in_or_out, num_logical_blocks - 1, 0, 0,
				FALSE, doall);
		/* zero offset indexing */
		if (!doall)
			continue; /* Just counting things when doall == 0 */

		logical_block[num_logical_blocks - 1].clock_net = OPEN;
		logical_block[num_logical_blocks - 1].input_nets = NULL;
		logical_block[num_logical_blocks - 1].output_nets = NULL;
		logical_block[num_logical_blocks - 1].model = io_model;

		len = strlen(ptr);
		if (in_or_out == RECEIVER) { /* output pads need out: prefix 
		 * to make names unique from LUTs */
			logical_block[num_logical_blocks - 1].name = (char *) my_malloc(
					(len + 1 + 4) * sizeof(char)); /* Space for out: at start */
			strcpy(logical_block[num_logical_blocks - 1].name, "out:");
			strcat(logical_block[num_logical_blocks - 1].name, ptr);
			logical_block[num_logical_blocks - 1].input_nets =
					(int **) my_malloc(sizeof(int*));
			logical_block[num_logical_blocks - 1].input_nets[0] =
					(int *) my_malloc(sizeof(int));
			logical_block[num_logical_blocks - 1].input_nets[0][0] = OPEN;
		} else {
			assert(in_or_out == DRIVER);
			logical_block[num_logical_blocks - 1].name = (char *) my_malloc(
					(len + 1) * sizeof(char));
			strcpy(logical_block[num_logical_blocks - 1].name, ptr);
			logical_block[num_logical_blocks - 1].output_nets =
					(int **) my_malloc(sizeof(int*));
			logical_block[num_logical_blocks - 1].output_nets[0] =
					(int *) my_malloc(sizeof(int));
			logical_block[num_logical_blocks - 1].output_nets[0][0] = OPEN;
		}

		if (in_or_out == DRIVER) { /* processing .inputs line */
			num_p_inputs++;
			logical_block[num_logical_blocks - 1].type = VPACK_INPAD;
			logical_block[num_logical_blocks - 1].output_nets[0][0] = nindex;
		} else { /* processing .outputs line */
			num_p_outputs++;
			logical_block[num_logical_blocks - 1].type = VPACK_OUTPAD;
			logical_block[num_logical_blocks - 1].input_nets[0][0] = nindex;
		}
		logical_block[num_logical_blocks - 1].truth_table = NULL;
	}
	assert(iparse < MAX_ATOM_PARSE);
}

static void check_and_count_models(int doall, const char* model_name,
		t_model *user_models) {
	fpos_t start_pos;
	t_model *user_model;

	num_blif_models++;
	if (doall) {
		/* get start position to do two passes on model */
		if (fgetpos(blif, &start_pos) != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "in file pointer read - read_blif.c\n");
			exit(-1);
		}

		/* get corresponding architecture model */
		user_model = user_models;
		while (user_model) {
			if (0 == strcmp(model_name, user_model->name)) {
				break;
			}
			user_model = user_model->next;
		}
		if (user_model == NULL) {
			vpr_printf(
					TIO_MESSAGE_ERROR, "No corresponding model %s in architecture description \n",
					model_name);
			exit(1);
		}

		/* check ports */
	}
}

static int add_vpack_net(char *ptr, int type, int bnum, int bport, int bpin,
		boolean is_global, int doall) {

	/* This routine is given a vpack_net name in *ptr, either DRIVER or RECEIVER *
	 * specifying whether the logical_block number (bnum) and the output pin (bpin) is driving this   *
	 * vpack_net or in the fan-out and doall, which is 0 for the counting pass   *
	 * and 1 for the loading pass.  It updates the vpack_net data structure and  *
	 * returns the vpack_net number so the calling routine can update the logical_block  *
	 * data structure.                                                     */

	struct s_hash *h_ptr, *prev_ptr;
	int index, j, nindex;

	if (strcmp(ptr, "open") == 0) {
		vpr_printf(TIO_MESSAGE_ERROR, "net name \"open\" is a reserved keyword in VPR");
		exit(1);
	}

	if (strcmp(ptr, "unconn") == 0) {
		return OPEN;
	}
	index = hash_value(ptr);

	if (doall) {
		if (type == RECEIVER && !is_global) {
			logical_block_input_count[bnum]++;
		} else if (type == DRIVER) {
			logical_block_output_count[bnum]++;
		}
	}

	h_ptr = blif_hash[index];
	prev_ptr = h_ptr;

	while (h_ptr != NULL) {
		if (strcmp(h_ptr->name, ptr) == 0) { /* Net already in hash table */
			nindex = h_ptr->index;

			if (!doall) { /* Counting pass only */
				(h_ptr->count)++;
				return (nindex);
			}

			if (type == DRIVER) {
				num_driver[nindex]++;
				j = 0; /* Driver always in position 0 of pinlist */
			} else {
				vpack_net[nindex].num_sinks++;
				if ((num_driver[nindex] < 0) || (num_driver[nindex] > 1)) {
					vpr_printf(TIO_MESSAGE_ERROR, "number of drivers for net #%d (%s) has %d drivers.\n",
							nindex, ptr, num_driver[index]);
				}
				j = vpack_net[nindex].num_sinks;

				/* num_driver is the number of signal drivers of this vpack_net. *
				 * should always be zero or 1 unless the netlist is bad.   */
				if ((vpack_net[nindex].num_sinks - num_driver[nindex])
						>= temp_num_pins[nindex]) {
					vpr_printf(TIO_MESSAGE_ERROR, 
							"Net #%d (%s) has no driver and will cause memory corruption.\n",
							nindex, ptr);
					exit(1);
				}
			}
			vpack_net[nindex].node_block[j] = bnum;
			vpack_net[nindex].node_block_port[j] = bport;
			vpack_net[nindex].node_block_pin[j] = bpin;
			vpack_net[nindex].is_global = is_global;
			return (nindex);
		}
		prev_ptr = h_ptr;
		h_ptr = h_ptr->next;
	}

	/* Net was not in the hash table. */

	if (doall == 1) {
		vpr_printf(TIO_MESSAGE_ERROR, "In add_vpack_net:  the second (load) pass could not\n"
							"find vpack_net %s in the symbol table.\n", ptr);
		exit(1);
	}

	/* Add the vpack_net (only counting pass will add nets to symbol table). */

	num_logical_nets++;
	h_ptr = (struct s_hash *) my_malloc(
			sizeof(struct s_hash));
	if (prev_ptr == NULL) {
		blif_hash[index] = h_ptr;
	} else {
		prev_ptr->next = h_ptr;
	}
	h_ptr->next = NULL;
	h_ptr->index = num_logical_nets - 1;
	h_ptr->count = 1;
	h_ptr->name = my_strdup(ptr);
	return (h_ptr->index);
}

void echo_input(char *blif_file, char *echo_file, t_model *library_models) {

	/* Echo back the netlist data structures to file input.echo to *
	 * allow the user to look at the internal state of the program *
	 * and check the parsing.                                      */

	int i, j;
	FILE *fp;
	t_model_ports *port;
	t_model *latch_model;
	t_model *logic_model;
	t_model *cur;
	int *lut_distribution;
	int num_absorbable_latch;
	int inet;

	cur = library_models;
	logic_model = latch_model = NULL;
	while (cur) {
		if (strcmp(cur->name, MODEL_LOGIC) == 0) {
			logic_model = cur;
			assert(logic_model->inputs->next == NULL);
		} else if (strcmp(cur->name, MODEL_LATCH) == 0) {
			latch_model = cur;
			assert(latch_model->inputs->size == 1);
		}
		cur = cur->next;
	}

	lut_distribution = (int*)my_calloc(logic_model->inputs[0].size + 1, sizeof(int));
	num_absorbable_latch = 0;
	for (i = 0; i < num_logical_blocks; i++) {
		if (logical_block[i].model == logic_model) {
			if (logic_model == NULL)
				continue;
			for (j = 0; j < logic_model->inputs[0].size; j++) {
				if (logical_block[i].input_nets[0][j] == OPEN) {
					break;
				}
			}
			lut_distribution[j]++;
		} else if (logical_block[i].model == latch_model) {
			if (latch_model == NULL)
				continue;
			inet = logical_block[i].input_nets[0][0];
			if (vpack_net[inet].num_sinks == 1
					&& logical_block[vpack_net[inet].node_block[0]].model
							== logic_model) {
				num_absorbable_latch++;
			}
		}
	}

	vpr_printf(TIO_MESSAGE_INFO, "Input netlist file: %s  Model: %s\n", blif_file, model);
	vpr_printf(TIO_MESSAGE_INFO, "Primary Inputs: %d.  Primary Outputs: %d.\n", num_p_inputs,
			num_p_outputs);
	vpr_printf(TIO_MESSAGE_INFO, "LUTs: %d.  Latches: %d Subckts: %d.\n", num_luts, num_latches,
			num_subckts);
	vpr_printf(TIO_MESSAGE_INFO, "# Standard absorbable latches: %d\n", num_absorbable_latch);
	for (i = 0; i < logic_model->inputs[0].size + 1; i++) {
		vpr_printf(TIO_MESSAGE_INFO, "LUT size %d = %d, ", i, lut_distribution[i]);
	}
	vpr_printf(TIO_MESSAGE_INFO, "\n");
	vpr_printf(TIO_MESSAGE_INFO, "Total Blocks: %d.  Total Nets: %d\n", num_logical_blocks,
			num_logical_nets);

	fp = my_fopen(echo_file, "w", 0);

	fprintf(fp, "Input netlist file: %s  Model: %s\n", blif_file, model);
	fprintf(fp,
			"num_p_inputs: %d, num_p_outputs: %d, num_luts: %d," " num_latches: %d\n",
			num_p_inputs, num_p_outputs, num_luts, num_latches);
	fprintf(fp, "num_logical_blocks: %d, num_logical_nets: %d\n",
			num_logical_blocks, num_logical_nets);

	fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\tRecvs.\n");
	for (i = 0; i < num_logical_nets; i++) {
		fprintf(fp, "\n%d\t%s\t", i, vpack_net[i].name);
		if (strlen(vpack_net[i].name) < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%d", vpack_net[i].num_sinks + 1);
		for (j = 0; j <= vpack_net[i].num_sinks; j++)
			fprintf(fp, "\t(%d,%d,%d)", vpack_net[i].node_block[j],
					vpack_net[i].node_block_port[j],
					vpack_net[i].node_block_pin[j]);
	}

	fprintf(fp, "\n\n\nBlocks\t\t\tBlock Type Legend:\n");
	fprintf(fp, "\t\t\tINPAD = %d\tOUTPAD = %d\n", VPACK_INPAD, VPACK_OUTPAD);
	fprintf(fp, "\t\t\tCOMB = %d\t\tLATCH = %d\n", VPACK_COMB, VPACK_LATCH);
	fprintf(fp, "\t\t\tEMPTY = %d\n", VPACK_EMPTY);

	for (i = 0; i < num_logical_blocks; i++) {
		fprintf(fp, "\nblock %d\t%s\t", i, logical_block[i].name);
		fprintf(fp, "type: %d\n", logical_block[i].type);
		fprintf(fp, "model name: %s\n", logical_block[i].model->name);

		port = logical_block[i].model->inputs;

		while (port) {
			fprintf(fp, "\tInput Port: %s\n", port->name);
			for (j = 0; j < port->size; j++) {
				if (logical_block[i].input_nets[port->index][j] == OPEN)
					fprintf(fp, "\tOPEN");
				else
					fprintf(fp, "\t%d",
							logical_block[i].input_nets[port->index][j]);
			}
			port = port->next;
		}

		port = logical_block[i].model->outputs;
		while (port) {
			fprintf(fp, "\tOutput Port: %s\n", port->name);
			for (j = 0; j < port->size; j++) {
				if (logical_block[i].output_nets[port->index][j] == OPEN) {
					fprintf(fp, "\tOPEN");
				} else {
					fprintf(fp, "\t%d",
							logical_block[i].output_nets[port->index][j]);
				}
			}
			port = port->next;
		}

		fprintf(fp, "\n\tclock net %d\n", logical_block[i].clock_net);
	}

	fprintf(fp, "\n");
	fclose(fp);
}

/* load default vpack models (inpad, outpad, logic) */
static void load_default_models(INP t_model *library_models,
		OUTP t_model** inpad_model, OUTP t_model** outpad_model,
		OUTP t_model** logic_model, OUTP t_model** latch_model) {
	t_model *cur_model;
	cur_model = library_models;
	*inpad_model = *outpad_model = *logic_model = *latch_model = NULL;
	while (cur_model) {
		if (strcmp(MODEL_INPUT, cur_model->name) == 0) {
			assert(cur_model->inputs == NULL);
			assert(cur_model->outputs->next == NULL);
			assert(cur_model->outputs->size == 1);
			*inpad_model = cur_model;
		} else if (strcmp(MODEL_OUTPUT, cur_model->name) == 0) {
			assert(cur_model->outputs == NULL);
			assert(cur_model->inputs->next == NULL);
			assert(cur_model->inputs->size == 1);
			*outpad_model = cur_model;
		} else if (strcmp(MODEL_LOGIC, cur_model->name) == 0) {
			assert(cur_model->inputs->next == NULL);
			assert(cur_model->outputs->next == NULL);
			assert(cur_model->outputs->size == 1);
			*logic_model = cur_model;
		} else if (strcmp(MODEL_LATCH, cur_model->name) == 0) {
			assert(cur_model->outputs->next == NULL);
			assert(cur_model->outputs->size == 1);
			*latch_model = cur_model;
		} else {
			assert(0);
		}
		cur_model = cur_model->next;
	}
}

static void check_net(boolean sweep_hanging_nets_and_inputs) {

	/* Checks the input netlist for obvious errors. */

	int i, j, k, error, iblk, ipin, iport, inet, L_check_net;
	boolean found;
	int count_inputs, count_outputs;
	int explicit_vpack_models;
	t_model_ports *port;
	struct s_linked_vptr *p_io_removed;
	int removed_nets;

	explicit_vpack_models = num_blif_models + 1;

	error = 0;
	removed_nets = 0;

	if (ilines != explicit_vpack_models) {
		vpr_printf(TIO_MESSAGE_ERROR, "Found %d .inputs lines; expected %d.\n", ilines,
				explicit_vpack_models);
		error++;
	}

	if (olines != explicit_vpack_models) {
		vpr_printf(TIO_MESSAGE_ERROR, "Found %d .outputs lines; expected %d.\n", olines,
				explicit_vpack_models);
		error++;
	}

	if (model_lines != explicit_vpack_models) {
		vpr_printf(TIO_MESSAGE_ERROR, "Found %d .model lines; expected %d.\n", model_lines,
				num_blif_models + 1);
		error++;
	}

	if (endlines != explicit_vpack_models) {
		vpr_printf(TIO_MESSAGE_ERROR, "Found %d .end lines; expected %d.\n", endlines,
				explicit_vpack_models);
		error++;
	}
	for (i = 0; i < num_logical_nets; i++) {

		if (num_driver[i] != 1) {
			vpr_printf(TIO_MESSAGE_ERROR, "vpack_net %s has" " %d signals driving it.\n",
					vpack_net[i].name, num_driver[i]);
			error++;
		}

		if (vpack_net[i].num_sinks == 0) {

			/* If this is an input pad, it is unused and I just remove it with  *
			 * a warning message.  Lots of the mcnc circuits have this problem. 

			 Also, subckts from ODIN often have unused driven nets
			 */

			iblk = vpack_net[i].node_block[0];
			iport = vpack_net[i].node_block_port[0];
			ipin = vpack_net[i].node_block_pin[0];

			assert((vpack_net[i].num_sinks - num_driver[i]) == -1);

			/* All nets should connect to inputs of block except output pads */
			if (logical_block[iblk].type != VPACK_OUTPAD) {
				if (sweep_hanging_nets_and_inputs) {
					removed_nets++;
					vpack_net[i].node_block[0] = OPEN;
					vpack_net[i].node_block_port[0] = OPEN;
					vpack_net[i].node_block_pin[0] = OPEN;
					logical_block[iblk].output_nets[iport][ipin] = OPEN;
					logical_block_output_count[iblk]--;
				} else {
					vpr_printf(TIO_MESSAGE_WARNING, "vpack_net %s has no fanout.\n",
							vpack_net[i].name);
				}
				continue;
			}
		}

		if (strcmp(vpack_net[i].name, "open") == 0
				|| strcmp(vpack_net[i].name, "unconn") == 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "vpack_net #%d has the reserved name %s.\n", i,
					vpack_net[i].name);
			error++;
		}

		for (j = 0; j <= vpack_net[i].num_sinks; j++) {
			iblk = vpack_net[i].node_block[j];
			iport = vpack_net[i].node_block_port[j];
			ipin = vpack_net[i].node_block_pin[j];
			if (ipin == OPEN) {
				/* Clocks are not connected to regular pins on a block hence open */
				L_check_net = logical_block[iblk].clock_net;
				if (L_check_net != i) {
					vpr_printf(TIO_MESSAGE_ERROR, 
							"clock net for block %s #%d is net %s #%d but connecting net is %s #%d\n",
							logical_block[iblk].name, iblk,
							vpack_net[L_check_net].name, L_check_net,
							vpack_net[i].name, i);
					error++;
				}

			} else {
				if (j == 0) {
					L_check_net = logical_block[iblk].output_nets[iport][ipin];
					if (L_check_net != i) {
						vpr_printf(TIO_MESSAGE_ERROR, 
								"output net for block %s #%d is net %s #%d but connecting net is %s #%d\n",
								logical_block[iblk].name, iblk,
								vpack_net[L_check_net].name, L_check_net,
								vpack_net[i].name, i);
						error++;
					}
				} else {
					if (vpack_net[i].is_global) {
						L_check_net = logical_block[iblk].clock_net;
					} else {
						L_check_net =
								logical_block[iblk].input_nets[iport][ipin];
					}
					if (L_check_net != i) {
						vpr_printf(TIO_MESSAGE_ERROR, 
								"Input net for block %s #%d is net %s #%d but connecting net is %s #%d\n",
								logical_block[iblk].name, iblk,
								vpack_net[L_check_net].name, L_check_net,
								vpack_net[i].name, i);
						error++;
					}
				}
			}
		}
	}
	vpr_printf(TIO_MESSAGE_INFO, "Swept away %d nets with no fanout\n", removed_nets);
	for (i = 0; i < num_logical_blocks; i++) {
		/* This block has no output and is not an output pad so it has no use, hence we remove it */
		if ((logical_block_output_count[i] == 0)
				&& (logical_block[i].type != VPACK_OUTPAD)) {
			vpr_printf(TIO_MESSAGE_WARNING, "logical_block %s #%d has no fanout.\n",
					logical_block[i].name, i);
			if (sweep_hanging_nets_and_inputs
					&& (logical_block[i].type == VPACK_INPAD)) {
				logical_block[i].type = VPACK_EMPTY;
				vpr_printf(TIO_MESSAGE_INFO, "  Removing input\n");
				p_io_removed = (struct s_linked_vptr*)my_malloc(sizeof(struct s_linked_vptr));
				p_io_removed->data_vptr = my_strdup(logical_block[i].name);
				p_io_removed->next = circuit_p_io_removed;
				circuit_p_io_removed = p_io_removed;
				continue;
			} else {
				vpr_printf(
						TIO_MESSAGE_ERROR, "   sweep hanging nodes in your logic synthesis tool because VPR can't do this yet\n");
				error++;
			}
		}
		count_inputs = 0;
		count_outputs = 0;
		port = logical_block[i].model->inputs;
		while (port) {
			if (port->is_clock) {
				port = port->next;
				continue;
			}

			for (j = 0; j < port->size; j++) {
				if (logical_block[i].input_nets[port->index][j] == OPEN)
					continue;
				count_inputs++;
				inet = logical_block[i].input_nets[port->index][j];
				found = FALSE;
				for (k = 1; k <= vpack_net[inet].num_sinks; k++) {
					if (vpack_net[inet].node_block[k] == i) {
						if (vpack_net[inet].node_block_port[k] == port->index) {
							if (vpack_net[inet].node_block_pin[k] == j) {
								found = TRUE;
							}
						}
					}
				}
				assert(found == TRUE);
			}
			port = port->next;
		}
		assert(count_inputs == logical_block_input_count[i]);
		logical_block[i].used_input_pins = count_inputs;

		port = logical_block[i].model->outputs;
		while (port) {
			for (j = 0; j < port->size; j++) {
				if (logical_block[i].output_nets[port->index][j] == OPEN)
					continue;
				count_outputs++;
				inet = logical_block[i].output_nets[port->index][j];
				vpack_net[inet].is_const_gen = FALSE;
				if (count_inputs == 0 && logical_block[i].type != VPACK_INPAD
						&& logical_block[i].type != VPACK_OUTPAD
						&& logical_block[i].clock_net == OPEN) {
					vpr_printf(TIO_MESSAGE_INFO, "Net is a constant generator: %s\n",
							vpack_net[inet].name);
					vpack_net[inet].is_const_gen = TRUE;
				}
				found = FALSE;
				if (vpack_net[inet].node_block[0] == i) {
					if (vpack_net[inet].node_block_port[0] == port->index) {
						if (vpack_net[inet].node_block_pin[0] == j) {
							found = TRUE;
						}
					}
				}
				assert(found == TRUE);
			}
			port = port->next;
		}
		assert(count_outputs == logical_block_output_count[i]);

		if (logical_block[i].type == VPACK_LATCH) {
			if (logical_block_input_count[i] != 1) {
				vpr_printf(
						TIO_MESSAGE_ERROR, "Latch #%d with output %s has %d input pin(s), expected one (D).\n",
						i, logical_block[i].name, logical_block_input_count[i]);
				error++;
			}
			if (logical_block_output_count[i] != 1) {
				vpr_printf(
						TIO_MESSAGE_ERROR, "Latch #%d with output %s has %d output pin(s), expected one (Q).\n",
						i, logical_block[i].name,
						logical_block_output_count[i]);
				error++;
			}
			if (logical_block[i].clock_net == OPEN) {
				vpr_printf(TIO_MESSAGE_ERROR, "Latch #%d with output %s has no clock.\n", i,
						logical_block[i].name);
				error++;
			}
		}

		else if (logical_block[i].type == VPACK_INPAD) {
			if (logical_block_input_count[i] != 0) {
				vpr_printf(
						TIO_MESSAGE_ERROR, "io inpad logical_block #%d name %s of type %d" "has %d input pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_input_count[i]);
				error++;
			}
			if (logical_block_output_count[i] != 1) {
				vpr_printf(
						TIO_MESSAGE_ERROR, "io inpad logical_block #%d name %s of type %d" "has %d output pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_output_count[i]);
				error++;
			}
			if (logical_block[i].clock_net != OPEN) {
				vpr_printf(TIO_MESSAGE_ERROR, "io inpad #%d with output %s has clock.\n", i,
						logical_block[i].name);
				error++;
			}
		} else if (logical_block[i].type == VPACK_OUTPAD) {
			if (logical_block_input_count[i] != 1) {
				vpr_printf(
						TIO_MESSAGE_ERROR, "io outpad logical_block #%d name %s of type %d" "has %d input pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_input_count[i]);
				error++;
			}
			if (logical_block_output_count[i] != 0) {
				vpr_printf(
						TIO_MESSAGE_ERROR, "io outpad logical_block #%d name %s of type %d" "has %d output pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_output_count[i]);
				error++;
			}
			if (logical_block[i].clock_net != OPEN) {
				vpr_printf(TIO_MESSAGE_ERROR, "io outpad #%d with name %s has clock.\n", i,
						logical_block[i].name);
				error++;
			}
		} else if (logical_block[i].type == VPACK_COMB) {
			if (logical_block_input_count[i] <= 0) {
				vpr_printf(TIO_MESSAGE_WARNING, 
						"logical_block #%d with output %s has only %d pin.\n",
						i, logical_block[i].name, logical_block_input_count[i]);

				if (logical_block_input_count[i] < 0) {
					error++;
				} else {
					if (logical_block_output_count[i] > 0) {
						vpr_printf(TIO_MESSAGE_WARNING, 
								"\tBlock contains output -- may be a constant generator.\n");
					} else {
						vpr_printf(TIO_MESSAGE_ERROR, "\tBlock contains no output.\n");
						error++;
					}
				}
			}

			if (strcmp(logical_block[i].model->name, MODEL_LOGIC) == 0) {
				if (logical_block_output_count[i] != 1) {
					vpr_printf(
							TIO_MESSAGE_ERROR, "logical_block #%d name %s of model %s \n has %d output pins instead of 1.\n",
							i, logical_block[i].name,
							logical_block[i].model->name,
							logical_block_output_count[i]);
					error++;
				}
			}
		} else {
			vpr_printf(TIO_MESSAGE_ERROR, "Unknown type for logical_block #%d %s\n", i,
					logical_block[i].name);
		}
	}

	if (error != 0) {
		vpr_printf(TIO_MESSAGE_ERROR, "Found %d fatal errors in the input netlist.\n", error);
		exit(1);
	}
}

static void free_parse(void) {

	/* Release memory needed only during blif network parsing. */

	int i;
	struct s_hash *h_ptr, *temp_ptr;

	for (i = 0; i < HASHSIZE; i++) {
		h_ptr = blif_hash[i];
		while (h_ptr != NULL) {
			free((void *) h_ptr->name);
			temp_ptr = h_ptr->next;
			free((void *) h_ptr);
			h_ptr = temp_ptr;
		}
	}
	free((void *) num_driver);
	free((void *) blif_hash);
	free((void *) temp_num_pins);
}


static void absorb_buffer_luts(void) {
	/* This routine uses a simple pattern matching algorithm to remove buffer LUTs where possible (single-input LUTs that are programmed to be a wire) */

	int bnum, in_blk, out_blk, ipin, out_net, in_net;
	int removed = 0;

	/* Pin ordering for the clb blocks (1 VPACK_LUT + 1 FF in each logical_block) is      *
	 * output, n VPACK_LUT inputs, clock input.                                   */

	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (strcmp(logical_block[bnum].model->name, "names") == 0) {
			if (logical_block[bnum].truth_table != NULL
					&& logical_block[bnum].truth_table->data_vptr) {
				if (strcmp("0 0",
						(char*) logical_block[bnum].truth_table->data_vptr) == 0
						|| strcmp("1 1",
								(char*) logical_block[bnum].truth_table->data_vptr)
								== 0) {
					for (ipin = 0;
							ipin < logical_block[bnum].model->inputs->size;
							ipin++) {
						if (logical_block[bnum].input_nets[0][ipin] == OPEN)
							break;
					}
					assert(ipin == 1);

					assert(logical_block[bnum].clock_net == OPEN);
					assert(logical_block[bnum].model->inputs->next == NULL);
					assert(logical_block[bnum].model->outputs->size == 1);
					assert(logical_block[bnum].model->outputs->next == NULL);

					in_net = logical_block[bnum].input_nets[0][0]; /* Net driving the buffer */
					out_net = logical_block[bnum].output_nets[0][0]; /* Net the buffer us driving */
					out_blk = vpack_net[out_net].node_block[1];
					in_blk = vpack_net[in_net].node_block[0];

					assert(in_net != OPEN);
					assert(out_net != OPEN);
					assert(out_blk != OPEN);
					assert(in_blk != OPEN);

					/* TODO: Make this handle general cases, due to time reasons I can only handle buffers with single outputs */
					if (vpack_net[out_net].num_sinks == 1) {
						for (ipin = 1; ipin <= vpack_net[in_net].num_sinks;
								ipin++) {
							if (vpack_net[in_net].node_block[ipin] == bnum) {
								break;
							}
						}
						assert(ipin <= vpack_net[in_net].num_sinks);

						vpack_net[in_net].node_block[ipin] =
								vpack_net[out_net].node_block[1]; /* New output */
						vpack_net[in_net].node_block_port[ipin] =
								vpack_net[out_net].node_block_port[1];
						vpack_net[in_net].node_block_pin[ipin] =
								vpack_net[out_net].node_block_pin[1];

						assert(
								logical_block[out_blk].input_nets[vpack_net[out_net].node_block_port[1]][vpack_net[out_net].node_block_pin[1]] == out_net);
						logical_block[out_blk].input_nets[vpack_net[out_net].node_block_port[1]][vpack_net[out_net].node_block_pin[1]] =
								in_net;

						vpack_net[out_net].node_block[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].node_block_pin[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].node_block_port[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].num_sinks = 0; /* This vpack_net disappears; mark. */

						logical_block[bnum].type = VPACK_EMPTY; /* Mark logical_block that had LUT */

						/* error checking */
						for (ipin = 0; ipin <= vpack_net[out_net].num_sinks;
								ipin++) {
							assert(vpack_net[out_net].node_block[ipin] != bnum);
						}
						removed++;
					}
				}
			}
		}
	}
	vpr_printf(TIO_MESSAGE_INFO, "Removed %d LUT buffers \n", removed);
}

static void compress_netlist(void) {

	/* This routine removes all the VPACK_EMPTY blocks and OPEN nets that *
	 * may have been left behind post synthesis.  After this    *
	 * routine, all the VPACK blocks that exist in the netlist     *
	 * are in a contiguous list with no unused spots.  The same     *
	 * goes for the list of nets.  This means that blocks and nets  *
	 * have to be renumbered somewhat.                              */

	int inet, iblk, index, ipin, new_num_nets, new_num_blocks, i;
	int *net_remap, *block_remap;
	int L_num_nets;
	t_model_ports *port;
	struct s_linked_vptr *tvptr, *next;

	new_num_nets = 0;
	new_num_blocks = 0;
	net_remap = (int *) my_malloc(num_logical_nets * sizeof(int));
	block_remap = (int *) my_malloc(num_logical_blocks * sizeof(int));

	for (inet = 0; inet < num_logical_nets; inet++) {
		if (vpack_net[inet].node_block[0] != OPEN) {
			net_remap[inet] = new_num_nets;
			new_num_nets++;
		} else {
			net_remap[inet] = OPEN;
		}
	}

	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (logical_block[iblk].type != VPACK_EMPTY) {
			block_remap[iblk] = new_num_blocks;
			new_num_blocks++;
		} else {
			block_remap[iblk] = OPEN;
		}
	}

	if (new_num_nets != num_logical_nets
			|| new_num_blocks != num_logical_blocks) {

		for (inet = 0; inet < num_logical_nets; inet++) {
			if (vpack_net[inet].node_block[0] != OPEN) {
				index = net_remap[inet];
				vpack_net[index] = vpack_net[inet];
				for (ipin = 0; ipin <= vpack_net[index].num_sinks; ipin++) {
					vpack_net[index].node_block[ipin] =
							block_remap[vpack_net[index].node_block[ipin]];
				}
			} else {
				free(vpack_net[inet].name);
				free(vpack_net[inet].node_block);
				free(vpack_net[inet].node_block_port);
				free(vpack_net[inet].node_block_pin);
			}
		}

		num_logical_nets = new_num_nets;
		vpack_net = (struct s_net *) my_realloc(vpack_net,
				num_logical_nets * sizeof(struct s_net));

		for (iblk = 0; iblk < num_logical_blocks; iblk++) {
			if (logical_block[iblk].type != VPACK_EMPTY) {
				index = block_remap[iblk];
				if (index != iblk) {
					logical_block[index] = logical_block[iblk];
					logical_block[index].index = index; /* array index moved */
				}

				L_num_nets = 0;
				port = logical_block[index].model->inputs;
				while (port) {
					for (ipin = 0; ipin < port->size; ipin++) {
						if (port->is_clock) {
							assert(
									port->size == 1 && port->index == 0 && ipin == 0);
							if (logical_block[index].clock_net == OPEN)
								continue;
							logical_block[index].clock_net =
									net_remap[logical_block[index].clock_net];
						} else {
							if (logical_block[index].input_nets[port->index][ipin]
									== OPEN)
								continue;
							logical_block[index].input_nets[port->index][ipin] =
									net_remap[logical_block[index].input_nets[port->index][ipin]];
						}
						L_num_nets++;
					}
					port = port->next;
				}

				port = logical_block[index].model->outputs;
				while (port) {
					for (ipin = 0; ipin < port->size; ipin++) {
						if (logical_block[index].output_nets[port->index][ipin]
								== OPEN)
							continue;
						logical_block[index].output_nets[port->index][ipin] =
								net_remap[logical_block[index].output_nets[port->index][ipin]];
						L_num_nets++;
					}
					port = port->next;
				}
			}

			else {
				free(logical_block[iblk].name);
				port = logical_block[iblk].model->inputs;
				i = 0;
				while (port) {
					if (!port->is_clock) {
						if (logical_block[iblk].input_nets) {
							if (logical_block[iblk].input_nets[i]) {
								free(logical_block[iblk].input_nets[i]);
								logical_block[iblk].input_nets[i] = NULL;
							}
						}
						i++;
					}
					port = port->next;
				}
				if (logical_block[iblk].input_nets)
					free(logical_block[iblk].input_nets);
				port = logical_block[iblk].model->outputs;
				i = 0;
				while (port) {
					if (logical_block[iblk].output_nets) {
						if (logical_block[iblk].output_nets[i]) {
							free(logical_block[iblk].output_nets[i]);
							logical_block[iblk].output_nets[i] = NULL;
						}
					}
					i++;
					port = port->next;
				}
				if (logical_block[iblk].output_nets)
					free(logical_block[iblk].output_nets);
				tvptr = logical_block[iblk].truth_table;
				while (tvptr != NULL) {
					if (tvptr->data_vptr)
						free(tvptr->data_vptr);
					next = tvptr->next;
					free(tvptr);
					tvptr = next;
				}
			}
		}

		vpr_printf(TIO_MESSAGE_INFO, "Sweeped away %d nodes\n", num_logical_blocks - new_num_blocks);

		num_logical_blocks = new_num_blocks;
		logical_block = (struct s_logical_block *) my_realloc(logical_block,
				num_logical_blocks * sizeof(struct s_logical_block));
	}

	/* Now I have to recompute the number of primary inputs and outputs, since *
	 * some inputs may have been unused and been removed.  No real need to     *
	 * recount primary outputs -- it's just done as defensive coding.          */

	num_p_inputs = 0;
	num_p_outputs = 0;

	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (logical_block[iblk].type == VPACK_INPAD)
			num_p_inputs++;
		else if (logical_block[iblk].type == VPACK_OUTPAD)
			num_p_outputs++;
	}

	free(net_remap);
	free(block_remap);
}

/* Read blif file and perform basic sweep/accounting on it */
void read_and_process_blif (char *blif_file, boolean sweep_hanging_nets_and_inputs,
		t_model *user_models, t_model *library_models) {

	/* begin parsing blif input file */
	read_blif (blif_file, sweep_hanging_nets_and_inputs, user_models,	library_models);
	
	/* TODO: Do check blif here 
	 eg. 
	 for (i = 0; i < num_logical_blocks; i++) {
	 if (logical_block[i].model->num_inputs > max_subblock_inputs) {
	 vpr_printf(TIO_MESSAGE_ERROR, "logical_block %s of model %s has %d inputs but architecture only supports subblocks up to %d inputs\n",
	 logical_block[i].name, logical_block[i].model->name, logical_block[i].model->num_inputs, max_subblock_inputs);
	 exit(1);
	 }
	 }
	 */

	if (GetEchoOption() && isEchoOptionEnable("blif_input.echo")) {
		echo_input(blif_file, "blif_input.echo", library_models);
	} else
		;

	absorb_buffer_luts();
	compress_netlist(); /* remove unused inputs */
	/* NB:  It's important to mark clocks and such *after* compressing the   *
	 * netlist because the vpack_net numbers, etc. may be changed by removing      *
	 * unused inputs .  */

	show_blif_stats(user_models, library_models);
	free(logical_block_input_count);
	free(logical_block_output_count);
	free(model);
	logical_block_input_count = NULL;
	logical_block_output_count = NULL;
	model = NULL;
}

/* Output blif statistics */
static void show_blif_stats(t_model *user_models, t_model *library_models) {
	struct s_model_stats *model_stats;
	struct s_model_stats *lut_model;
	int num_model_stats;
	t_model *cur;
	int MAX_LUT_INPUTS;
	int i, j, iblk, ipin, num_pins;
	int *num_lut_of_size;

	/* Store data structure for all models in FPGA */
	num_model_stats = 0;

	cur = library_models;
	while (cur) {
		num_model_stats++;
		cur = cur->next;
	}

	cur = user_models;
	while (cur) {
		num_model_stats++;
		cur = cur->next;
	}

	model_stats = (struct s_model_stats*)my_calloc(num_model_stats, sizeof(struct s_model_stats));

	num_model_stats = 0;

	lut_model = NULL;
	cur = library_models;
	while (cur) {
		model_stats[num_model_stats].model = cur;
		if (strcmp(cur->name, "names") == 0) {
			lut_model = &model_stats[num_model_stats];
		}
		num_model_stats++;
		cur = cur->next;
	}

	cur = user_models;
	while (cur) {
		model_stats[num_model_stats].model = cur;
		num_model_stats++;
		cur = cur->next;
	}

	/* Gather statistics from circuit */
	MAX_LUT_INPUTS = 0;
	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (strcmp(logical_block[iblk].model->name, "names") == 0) {
			MAX_LUT_INPUTS = logical_block[iblk].model->inputs->size;
			break;
		}
	}
	num_lut_of_size = (int*) my_calloc(MAX_LUT_INPUTS + 1, sizeof(int));


	for (i = 0; i < num_logical_blocks; i++) {
		for (j = 0; j < num_model_stats; j++) {
			if (logical_block[i].model == model_stats[j].model) {
				break;
			}
		}
		assert(j < num_model_stats);
		model_stats[j].count++;
		if (&model_stats[j] == lut_model) {
			num_pins = 0;
			for (ipin = 0; ipin < logical_block[i].model->inputs->size;
					ipin++) {
				if (logical_block[i].input_nets[0][ipin] != OPEN) {
					num_pins++;
				}
			}
			num_lut_of_size[num_pins]++;
		}
	}

	/* Print blif circuit stats */

	vpr_printf(TIO_MESSAGE_INFO, "Blif circuit stats: \n\n");

	for (i = 0; i <= MAX_LUT_INPUTS; i++) {
		vpr_printf(TIO_MESSAGE_INFO, "%d LUTs of size %d\n", num_lut_of_size[i], i);
	}
	vpr_printf(TIO_MESSAGE_INFO, "\n");

	for (i = 0; i < num_model_stats; i++) {
		vpr_printf(TIO_MESSAGE_INFO, "%d of type %s\n", model_stats[i].count, model_stats[i].model->name);
	}

	free(model_stats);
	free(num_lut_of_size);
}



