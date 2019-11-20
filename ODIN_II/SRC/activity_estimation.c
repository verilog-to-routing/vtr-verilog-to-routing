/*

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/ 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "odin_util.h"
#include "ast_util.h"
#include "util.h"
#include "activity_estimation.h"
#include "netlist_check.h"

#define DEFAULT_STATIC_PROBABILITY .5
#define DEFAULT_TRANSITION_DENSITY .5

#define ACT_NUM_ITER 3 

#define activation_t struct activation_t_t
activation_t
{
	double *static_probability;
	double *transition_probability;
	double *transition_density;
};

void initialize_probabilities(char *input_file, netlist_t *netlist);
void calc_transition_density(netlist_t *netlist);
void calc_probabilities_and_init_act_data(netlist_t *netlist);
short *boolean_difference(nnode_t *node, int variable_spot);
double calc_density(nnode_t *node, int variable_spot, short *boolean_difference);
void output_activation_file_ace_and_function_file(char *output_filename, int lut_size, netlist_t *LUT_netlist, netlist_t *CLUSTER_netlist);
void cleanup_activation(netlist_t *netlist);

/*---------------------------------------------------------------------------------------------
 * (function: activity_estimation)
 *-------------------------------------------------------------------------------------------*/
void activity_estimation(char *input_filename, char *output_filename, int lut_size, netlist_t *LUT_netlist, netlist_t *CLUSTER_netlist)
{
	/* levelize the graph. Note, can't levelize the ClUSTER_netlist since there are loops */
	levelize_and_check_for_combinational_loop_and_liveness(FALSE, LUT_netlist);

	/* initializes the data structures and the PI */
	initialize_probabilities(input_filename, LUT_netlist);

	/* Calculate the probabilities for each of the nodes in the LUT_netlist */
	calc_probabilities_and_init_act_data(LUT_netlist);

	/* calculate the transition density for each node */
	calc_transition_density(LUT_netlist);

	/* Output file with the transition densities */
	output_activation_file_ace_and_function_file(output_filename, lut_size, LUT_netlist, CLUSTER_netlist);

	/* cleanup the data structures so we can use node_data in another algorithm */
	cleanup_activation(LUT_netlist);

	/* Path is where we are */
//	graphVizOutputNetlist(configuration.debug_output_path, "blif", 1, blif_gnd_node, blif_vcc_node, blif_input_nodes, num_blif_input_nodes);
}

/*---------------------------------------------------------------------------------------------
 * (function: calc_transition_density)
 *-------------------------------------------------------------------------------------------*/
void calc_transition_density(netlist_t *netlist)
{
	int i, j, m;
	activation_t *act_data;

	/* progress from the primary inputs (in the forward level 0) to each stage */
	for (i = 0; i < netlist->num_forward_levels; i++)
	{
		for (j = 0; j < netlist->num_at_forward_level[i]; j++)
		{
			/* initialize the activation data */
			nnode_t *current_node = netlist->forward_levels[i][j];
			act_data = (activation_t*)current_node->node_data;
	
			/* All other levels we pass through the probabilities and calculate Transition prbability for the DEnsity calculation */
			if (current_node->type == BLIF_FUNCTION)
			{
				/* only one output */
				act_data->transition_density = (double*)malloc(sizeof(double)); // only one output
	
				if (current_node->num_input_pins == 1)
				{
					/* If this is a Constant, NOT or a BUFFER then the transition density is easy to calculate */
					nnode_t *input_node = current_node->input_pins[0]->net->driver_pin->node;
					int input_node_pin = current_node->input_pins[0]->net->driver_pin->pin_node_idx;
					activation_t *input_data = (activation_t*)input_node->node_data;
					oassert(input_node->unique_node_data_id == ACTIVATION);

					if ((current_node->associated_function[0] == 0) && (current_node->associated_function[1] == 0))
						/* CONSTANT 0 */
						act_data->transition_density[0] = 0;
					if ((current_node->associated_function[0] == 0) && (current_node->associated_function[1] == 1))
						/* BUFFER */
						act_data->transition_density[0] = input_data->transition_density[input_node_pin];
					if ((current_node->associated_function[0] == 1) && (current_node->associated_function[1] == 0))
						/* NOT */
						act_data->transition_density[0] = input_data->transition_density[input_node_pin];
					if ((current_node->associated_function[0] == 1) && (current_node->associated_function[1] == 1))
						/* CONSTANT 1 */
						act_data->transition_density[0] = 0;
				}
				else
				{
					double density_val = 0.0;

					/* calculate the transition densities sumof(D(y)*P(boolean_diff, y) */
					for (m = 0; m < current_node->num_input_pins; m++)
					{
						short *boolean_difference_function;

						/* calculate hte boolean difference */
						boolean_difference_function = boolean_difference(current_node, m);

						/* now calculate the denisty of this input ... sum of */
						density_val = density_val + calc_density(current_node, m, boolean_difference_function);

						/* free the array */
						free(boolean_difference_function);
					}

					act_data->transition_density[0] = density_val;
				}
			}
			else if (current_node->type == FF_NODE)
			{
				nnode_t *input_node = current_node->input_pins[0]->net->driver_pin->node;
				int input_node_pin = current_node->input_pins[0]->net->driver_pin->pin_node_idx;
				activation_t *input_data = (activation_t*)input_node->node_data;
				oassert(input_node->unique_node_data_id == ACTIVATION);
		
				/* just store since allocated in the initialization */
				act_data->transition_density = (double*)malloc(sizeof(double));
				act_data->transition_density[0] = 2 * (input_data->static_probability[input_node_pin] * (1-input_data->static_probability[input_node_pin])); 
			}
			else if (current_node->type == OUTPUT_NODE)
			{
				nnode_t *input_node = current_node->input_pins[0]->net->driver_pin->node;
				int input_node_pin = current_node->input_pins[0]->net->driver_pin->pin_node_idx;
				activation_t *input_data = (activation_t*)input_node->node_data;
				oassert(input_node->unique_node_data_id == ACTIVATION);
		
				/* allocate and stre through */
				act_data->transition_density = (double*)malloc(sizeof(double));
				act_data->transition_density[0] = input_data->static_probability[input_node_pin];
			}
			else if ((current_node->type == INPUT_NODE) || (current_node->type == VCC_NODE) || (current_node->type == GND_NODE))
			{
				oassert(current_node->unique_node_data_id == ACTIVATION);
			}
			else
			{
				oassert(FALSE);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: initialize_probabilities)
 *-------------------------------------------------------------------------------------------*/
void initialize_probabilities(char *input_file, netlist_t *netlist)
{
	int i, j;
	activation_t *act_data;

	for (i = 0; i < netlist->num_forward_levels; i++)
	{
		for (j = 0; j < netlist->num_at_forward_level[i]; j++)
		{
			/* initialize the activation data */
			nnode_t *current_node = netlist->forward_levels[i][j];
			oassert(current_node->unique_node_data_id == RESET);

			current_node->unique_node_data_id = ACTIVATION;
			act_data = (activation_t*)malloc(sizeof(activation_t));
			current_node->node_data = (void*)act_data;
	
			if (current_node->type == INPUT_NODE)
			{
				oassert(i == 0);
				/* IF - PI and FFs, then we set their probability */
				if (input_file != NULL)
				{
					/* READ input probabilities */
					oassert(FALSE);
	
					/* READ in the input file */

					/* exit since all read */
					i = netlist->num_forward_levels;
				}
				else
				{
					/* initialize all the initial probabilities */
					act_data->static_probability = (double*)malloc(sizeof(double));
					act_data->transition_probability = (double*)malloc(sizeof(double));
					act_data->transition_density = (double*)malloc(sizeof(double));

					act_data->static_probability[0] = DEFAULT_STATIC_PROBABILITY;
					act_data->transition_probability[0] = -1;
					act_data->transition_density[0] = DEFAULT_TRANSITION_DENSITY;
				}
			}
			else if (current_node->type == GND_NODE)
			{
				/* initialize all the initial probabilities */
				act_data->static_probability = (double*)malloc(sizeof(double));
				act_data->transition_probability = (double*)malloc(sizeof(double));
				act_data->transition_density = (double*)malloc(sizeof(double));

				act_data->static_probability[0] = 0.0;
				act_data->transition_probability[0] = -1;
				act_data->transition_density[0] = 0;

			}
			else if (current_node->type == VCC_NODE)
			{
				/* initialize all the initial probabilities */
				act_data->static_probability = (double*)malloc(sizeof(double));
				act_data->transition_probability = (double*)malloc(sizeof(double));
				act_data->transition_density = (double*)malloc(sizeof(double));

				act_data->static_probability[0] = 1.0;
				act_data->transition_probability[0] = -1;
				act_data->transition_density[0] = 0;

			}
			else if (current_node->type == FF_NODE)
			{
				/* initialize all the initial probabilities */
				act_data->static_probability = (double*)malloc(sizeof(double));
				act_data->transition_probability = (double*)malloc(sizeof(double));
				act_data->transition_density = (double*)malloc(sizeof(double));

				act_data->static_probability[0] = DEFAULT_STATIC_PROBABILITY;
				act_data->transition_probability[0] = -1;
				act_data->transition_density[0] = -1;
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: calc_probabilities_and_init_act_data)
 *  Calculates the static probability for each output pin P(y)
 *  Calculates the transition probabiliy for each output pin Pt(y) = 2*P(y)*(1-P(y))
 *-------------------------------------------------------------------------------------------*/
void calc_probabilities_and_init_act_data(netlist_t *netlist)
{
	int i, j, k, l, rep;
	activation_t *act_data;

	for (rep = 0; rep < ACT_NUM_ITER; rep++)
	{
		/* progress from the primary inputs (in the forward level 0) to each stage */
		for (i = 0; i < netlist->num_forward_levels; i++)
		{
			for (j = 0; j < netlist->num_at_forward_level[i]; j++)
			{
				/* initialize the activation data */
				nnode_t *current_node = netlist->forward_levels[i][j];
				act_data = (activation_t*)current_node->node_data;

				/* All other levels we pass through the probabilities and calculate Transition prbability for the DEnsity calculation */
				if (current_node->type == BLIF_FUNCTION)
				{
					double totalProb = 0;
					int function_size = pow2(current_node->num_input_pins);
	
					if (rep == 0)
					{
						/* only one output */
						act_data->static_probability = (double*)malloc(sizeof(double));
					}
	
					for (k = 0; k < function_size; k++)
					{
						long long int place = 1;
						double probVal = 1;
	
						if (current_node->associated_function[k] == 1)
						{
							/* IF - this function value is on then calculate the probability */
							for (l = 0; l < current_node->num_input_pins; l++)
							{
								nnode_t *input_node = current_node->input_pins[l]->net->driver_pin->node;
								int input_node_pin = current_node->input_pins[l]->net->driver_pin->pin_node_idx;
								activation_t *input_data = (activation_t*)input_node->node_data;
								oassert(input_node->unique_node_data_id == ACTIVATION);
	
								if ((k & place) > 0) // if this bit is high
								{
									probVal = probVal * input_data->static_probability[input_node_pin];
								}
								else
								{
									probVal = probVal * (1-input_data->static_probability[input_node_pin]);
								}
								/* move the bit collumn */
								place = place << 1;
							}	

							totalProb += probVal;
						}
	
					}
					/* store the total probability */
					act_data->static_probability[0] = totalProb;
				}
				else if (current_node->type == FF_NODE)
				{
					if (rep != 0)
					{
						/* ONLY do calculations after first repetition */
						nnode_t *input_node = current_node->input_pins[0]->net->driver_pin->node;
						int input_node_pin = current_node->input_pins[0]->net->driver_pin->pin_node_idx;
						activation_t *input_data = (activation_t*)input_node->node_data;
						oassert(input_node->unique_node_data_id == ACTIVATION);
	
						act_data->static_probability[0] = input_data->static_probability[input_node_pin];
					}
				}
				else if (current_node->type == OUTPUT_NODE)
				{
					nnode_t *input_node = current_node->input_pins[0]->net->driver_pin->node;
					int input_node_pin = current_node->input_pins[0]->net->driver_pin->pin_node_idx;
					activation_t *input_data = (activation_t*)input_node->node_data;
					oassert(input_node->unique_node_data_id == ACTIVATION);
	
					if (rep == 0)
					{
						/* only one output */
						act_data->static_probability = (double*)malloc(sizeof(double));
					}
	
					act_data->static_probability[0] = input_data->static_probability[input_node_pin];
				}
				else if ((current_node->type == INPUT_NODE) || (current_node->type == VCC_NODE) || (current_node->type == GND_NODE))
				{
					oassert(current_node->unique_node_data_id == ACTIVATION);
					continue; // skip transition probability calculation 
				}
				else
				{
					oassert(FALSE);
				}

				if (rep == 0)
				{
					/* calculate transition probability */
					act_data->transition_probability = (double*)malloc(sizeof(double)*current_node->num_output_pins);
				}

				for (k = 0; k < current_node->num_output_pins; k++)
				{
					act_data->transition_probability[k] = 2 * (act_data->static_probability[k] * (1-act_data->static_probability[k])); 
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: boolean_difference)
 *-------------------------------------------------------------------------------------------*/
short *boolean_difference(nnode_t *node, int variable_spot)
{
	int i, l;
	short *return_function;
	int function_size;
	long long skip_size = 1 << variable_spot;
	int index;

	oassert(node->num_input_pins < sizeof (long long int)*8);
	oassert(node->associated_function != NULL);
	oassert(node->num_input_pins > 1);

	/* calculate the size of the boolean difference */
	function_size = pow2(node->num_input_pins-1);

	return_function = (short*)calloc(sizeof(short), function_size);

	for (i = 0; i < function_size; i++)
	{
		long long int place = 1;
		long long int index_plus = 1;
		index = 0;
	
		/* IF - this function value is on then calculate the probability */
		for (l = 0; l < node->num_input_pins-1; l++)
		{
			/* move the bit collumn */
			if (l == variable_spot)
			{
				index_plus = index_plus << 1;
			}

			if ((i & place) > 0) // if this bit is high
			{
				index += index_plus;
			}

			place = place << 1;
			index_plus = index_plus << 1;
		}

		/* do the boolean xor of this element */
		return_function[i] = node->associated_function[index] ^ node->associated_function[index+skip_size];
	}

	return return_function;
}

/*---------------------------------------------------------------------------------------------
 * (function: calc_density)
 *-------------------------------------------------------------------------------------------*/
double calc_density(nnode_t *node, int variable_spot, short *boolean_difference)
{
	int i, l;
	int function_size;
	double totalProb = 0;
	double input_density = 0;

	oassert(node->num_input_pins < sizeof (long long int)*8);

	if ((node->input_pins[variable_spot] != NULL) && (node->input_pins[variable_spot]->net->driver_pin != NULL) && (node->input_pins[variable_spot]->net->driver_pin->node != NULL))
	{
		nnode_t *input_node = node->input_pins[variable_spot]->net->driver_pin->node;
		int input_node_pin = node->input_pins[variable_spot]->net->driver_pin->pin_node_idx;
		activation_t *input_data = (activation_t*)input_node->node_data;
		oassert(input_node->unique_node_data_id == ACTIVATION);

		input_density = input_data->transition_density[input_node_pin];
	}
	else
	{
		oassert(FALSE);
	}

	/* calculate the size of the boolean difference */
	function_size = pow2(node->num_input_pins-1);

	for (i = 0; i < function_size; i++)
	{
		long long int place = 1;
		double probVal = 1;
		int index = -1;
	
		if (boolean_difference[i] == 1)
		{
			/* IF - this function value is on then calculate the probability */
			for (l = 0; l < node->num_input_pins-1; l++)
			{
				nnode_t *input_node;
				activation_t *input_data;
				int input_node_pin;

				/* move the bit collumn */
				if (l == variable_spot)
				{
					/* skip this one */
					index += 2;
				}
				else
				{
					index ++;
				}
	
				input_node = node->input_pins[index]->net->driver_pin->node;
				input_node_pin = node->input_pins[index]->net->driver_pin->pin_node_idx;
				input_data = (activation_t*)input_node->node_data;
				oassert(input_node->unique_node_data_id == ACTIVATION);

				if ((i & place) > 0) // if this bit is high
				{
					probVal = probVal * input_data->static_probability[input_node_pin];
				}
				else
				{
					probVal = probVal * (1-input_data->static_probability[input_node_pin]);
				}

				place = place << 1;
			}
			totalProb += probVal;
		}
	}

	oassert(totalProb <= 1.1);
	if (totalProb > 1)
		totalProb = 1; // for rounding errors

//	oassert(totalProb * input_density < 1);
	return totalProb * input_density;
}

/*---------------------------------------------------------------------------------------------
 * (function: output_activation_file_ace_and_function_file)
 *	Creates the 2 files (+1 .ace file) for the power estimation.
 *	Traverses the blif_netlist, net_netlist to print out tranisition densities
 * 	and static probabilities.  Also, pushes out functions based on the clustered
 *	netlist.
 *-------------------------------------------------------------------------------------------*/
void output_activation_file_ace_and_function_file(char *output_filename, int lut_size, netlist_t *LUT_netlist, netlist_t *CLUSTER_netlist)
{
	char *ace_file_name = (char*)malloc(sizeof(char)*(strlen(output_filename)+4+1));
	char *ac2_file_name = (char*)malloc(sizeof(char)*(strlen(output_filename)+4+1));
	char *function_file_name = (char*)malloc(sizeof(char)*(strlen(output_filename)+4+1));
	int i, j, k, l;
	FILE *ace_out;
	FILE *ac2_out;
	FILE *function_out;
	long sc_spot;
	nnode_t *lut_node;
	activation_t *act_data;

	sprintf(ace_file_name, "%s.ace", output_filename);
	sprintf(ac2_file_name, "%s.ac2", output_filename);
	sprintf(function_file_name, "%s.fun", output_filename);

	ace_out = fopen(ace_file_name, "w");
	if (ace_out == NULL)
	{
		error_message(ACTIVATION_ERROR, -1, -1, "Could not open output file %s\n", ace_file_name);
	}
	ac2_out = fopen(ac2_file_name, "w");
	if (ac2_out == NULL)
	{
		error_message(ACTIVATION_ERROR, -1, -1, "Could not open output file %s\n", ac2_file_name);
	}
	function_out = fopen(function_file_name, "w");
	if (function_out == NULL)
	{
		error_message(ACTIVATION_ERROR, -1, -1, "Could not open output file %s\n", function_file_name);
	}

	/* Go through the LUT netlist and print out the ace files */
	for (i = 0; i < LUT_netlist->num_forward_levels; i++)
	{
		for (j = 0; j < LUT_netlist->num_at_forward_level[i]; j++)
		{
			/* initialize the activation data */
			nnode_t *current_node = LUT_netlist->forward_levels[i][j];
			activation_t *act_data = (activation_t*)current_node->node_data;

			if (current_node->type != OUTPUT_NODE)
			{
				for (k = 0; k < current_node->num_output_pins; k++)
				{
					if (current_node->output_pins[0]->net->num_fanout_pins != 0)
					{
						/* IF this node fans out */
						fprintf(ace_out, "%s %f %f\n", current_node->name, act_data->static_probability[k], act_data->transition_density[k]);
					}
				}
			}
			else
			{
				fprintf(ace_out, "out:%s %f %f\n", current_node->name, act_data->static_probability[0], act_data->transition_density[0]);
			}
		}
	}

	fclose(ace_out);

	/* now create the ac2 file and function file */

	/* first we spit out the clocks */
	for (i = 0; i < CLUSTER_netlist->num_clocks; i++)
	{
		nnode_t *cluster_node = CLUSTER_netlist->clocks[i];
		fprintf (ac2_out, "global_net_probability %s 0.5\n", cluster_node->name);
		fprintf (ac2_out, "global_net_density %s 2.0\n", cluster_node->name);
	}

	/* next we make the intercluster probabilities for inputs */
	for (i = 0; i < CLUSTER_netlist->num_top_input_nodes; i++)
	{
		nnode_t *cluster_node = CLUSTER_netlist->top_input_nodes[i];

		if (cluster_node->type == CLOCK_NODE)
		{
			continue;
		}

		/* find the equivalent blif point */
		if ((sc_spot = sc_lookup_string(LUT_netlist->nodes_sc, cluster_node->name)) == -1)
		{
			warning_message(ACTIVATION_ERROR, -1, -1, "Could not find %s INPUT in LUT netlist ...\n", cluster_node->name);
			continue;
		}
		lut_node = (nnode_t *)LUT_netlist->nodes_sc->data[sc_spot];	
		act_data = (activation_t*)lut_node->node_data;

		oassert(lut_node->num_output_pins == 1); /* assumption now, but will change with heterognenous blocks */
		fprintf (ac2_out, "intercluster_net_probability %s %f\n", cluster_node->name, act_data->static_probability[0]);
		fprintf (ac2_out, "intercluster_net_density %s %f\n", cluster_node->name, act_data->transition_density[0]);
	}

	/* next we make the intercluster probabilities for inputs */
	for (i = 0; i < CLUSTER_netlist->num_top_output_nodes; i++)
	{
		nnode_t *cluster_node = CLUSTER_netlist->top_output_nodes[i];

		/* find the equivalent blif point */
		if ((sc_spot = sc_lookup_string(LUT_netlist->nodes_sc, cluster_node->name)) == -1)
		{
			warning_message(ACTIVATION_ERROR, -1, -1, "Could not find %s OUTPUT in LUT netlist ...\n", cluster_node->name);
			continue;
		}
		lut_node = (nnode_t *)LUT_netlist->nodes_sc->data[sc_spot];	
		act_data = (activation_t*)lut_node->node_data;

		oassert(lut_node->num_output_pins == 0); /* assumption now, but will change with heterognenous blocks */
		fprintf (ac2_out, "intercluster_net_probability %s %f\n", (cluster_node->name+4), act_data->static_probability[0]); /* +4 to skip "out:" that isn't needed in output */
		fprintf (ac2_out, "intercluster_net_density %s %f\n", (cluster_node->name+4), act_data->transition_density[0]);
	}

	/* next we make the intercluster probabilities */
	for (i = 0; i < CLUSTER_netlist->num_internal_nodes; i++)
	{
		nnode_t *cluster_node = CLUSTER_netlist->internal_nodes[i];
		netlist_t* internal_subblocks = cluster_node->internal_netlist;

		if (internal_subblocks == NULL)
		{
			/* INPUT/OUTPUT pins and globals (clocks) have no subblocks */
			continue;
		}

		/* now we process the internal structure of the node, which represents the subblocks.  This is stored in an internall netlist. */
		for (k = 0; k < internal_subblocks->num_internal_nodes; k++)
		{
			nnode_t *current_subblock = internal_subblocks->internal_nodes[k];
			char *output_search_name = (char*)malloc(sizeof(char)*(strlen(current_subblock->name)+1+4));
			sprintf(output_search_name, "out:%s", current_subblock->name);

			if ((sc_spot = sc_lookup_string(internal_subblocks->nodes_sc, output_search_name)) != -1)
			{
				/* IF - this is an output of the cluster then we need inter cluster info */

				/* now find this node */
				/* find the equivalent blif point */
				if ((sc_spot = sc_lookup_string(LUT_netlist->nodes_sc, current_subblock->name)) == -1)
				{
					error_message(ACTIVATION_ERROR, -1, -1, "Could not find %s in LUT netlist ...\n", current_subblock->name);
				}
				lut_node = (nnode_t *)LUT_netlist->nodes_sc->data[sc_spot];
				act_data = (activation_t*)lut_node->node_data;

				/* Finally, put in th output switching values */
				fprintf (ac2_out, "intercluster_net_probability %s %f\n", current_subblock->name, act_data->static_probability[0]);
				fprintf (ac2_out, "intercluster_net_density %s %f\n", current_subblock->name, act_data->transition_density[0]);
			}

			free(output_search_name);
		}
	}

	/* next we process the subblocks */
	for (i = 0; i < CLUSTER_netlist->num_internal_nodes; i++)
	{
		nnode_t *cluster_node = CLUSTER_netlist->internal_nodes[i];
		netlist_t* internal_subblocks = cluster_node->internal_netlist;

		if (internal_subblocks == NULL)
		{
			/* INPUT/OUTPUT pins and globals (clocks) have no subblocks */
			oassert(FALSE);
			continue;
		}

		/* now we process the internal structure of the node, which represents the subblocks.  This is stored in an internall netlist. */
		for (k = 0; k < internal_subblocks->num_internal_nodes; k++)
		{
			nnode_t *current_subblock = internal_subblocks->internal_nodes[k];
			char probability_string[4096];
			char density_string[4096];
			int input_active_count = 0;
			short is_clock = FALSE;
			int input_index = 0;

			sprintf(probability_string, "subblock_probability %s", current_subblock->name);
			sprintf(density_string, "subblock_density %s", current_subblock->name);

			/* first get all the input values */
			for (l = 0; l < lut_size+1; l++)
			{
				if (current_subblock->input_pins[input_index] != NULL)
				{
					nnode_t *input_node = current_subblock->input_pins[input_index]->net->driver_pin->node;
			
					/* find the equivalent blif point */
					if ((sc_spot = sc_lookup_string(LUT_netlist->nodes_sc, input_node->name)) == -1)
					{
						error_message(ACTIVATION_ERROR, -1, -1, "Could not find %s in LUT netlist ...\n", input_node->name);
					}
					lut_node = (nnode_t *)LUT_netlist->nodes_sc->data[sc_spot];	
					act_data = (activation_t*)lut_node->node_data;
			
					if (lut_node->type == CLOCK_NODE)
					{
						/* IF - the clock spot */
						if (l == lut_size)
						{
							/* only print it out if this is the last spot */
							sprintf(probability_string, "%s %f", probability_string, act_data->static_probability[0]);
							sprintf(density_string, "%s %f", density_string,  act_data->transition_density[0]);
							is_clock = TRUE;
						}
						else
						{
							sprintf(probability_string, "%s 0.0", probability_string);
							sprintf(density_string, "%s 0.0", density_string);
						}
					}
					else
					{
						sprintf(probability_string, "%s %f", probability_string, act_data->static_probability[0]);
						sprintf(density_string, "%s %f", density_string,  act_data->transition_density[0]);

						input_index++;
					}

					input_active_count ++;
				}	
				else
				{
					/* No connection to this input */
					sprintf(probability_string, "%s 0.0", probability_string);
					sprintf(density_string, "%s 0.0", density_string);
				}
			}
			
			if (is_clock == FALSE)
			{
				/* IF - there is no clock meaning this is just a LUT, output the probabilities */
				sprintf(probability_string, "%s 0.0", probability_string);
				sprintf(density_string, "%s 0.0", density_string);
			}

			/* now find this node */
			/* find the equivalent blif point */
			if ((sc_spot = sc_lookup_string(LUT_netlist->nodes_sc, current_subblock->name)) == -1)
			{
				error_message(ACTIVATION_ERROR, -1, -1, "Could not find %s in LUT netlist ...\n", current_subblock->name);
			}
			lut_node = (nnode_t *)LUT_netlist->nodes_sc->data[sc_spot];
			act_data = (activation_t*)lut_node->node_data;

			/* find the values of the switching between LUT and FF */
			if ((lut_node->type == FF_NODE) && (input_active_count > 0))
			{
				/* LUT + FF */
				/* IF - this is a FF node and has more than 1 input, then it's a LUT-FF */
				activation_t *input_node_to_ff_act_data = (activation_t *)lut_node->input_pins[0]->net->driver_pin->node->node_data;
				sprintf(probability_string, "%s %f", probability_string, input_node_to_ff_act_data->static_probability[0]);
				sprintf(density_string, "%s %f", density_string,  input_node_to_ff_act_data->transition_density[0]);

				/* print out the function of this node based on it's inputs function */
				oassert (lut_node->input_pins[0]->net->driver_pin->node->associated_function != NULL)
				fprintf(function_out, "subblock_function %s ", lut_node->name);
		
				for (l = 0; l < pow2(lut_size); l++)
				{
					fprintf(function_out, "%d", lut_node->input_pins[0]->net->driver_pin->node->associated_function[l]);
				}
	
				fprintf(function_out, "\n");
			}
			else if (lut_node->type == FF_NODE)
			{
				/* FF */
				/* ELSE - this is a FF_NODE only */
				sprintf(probability_string, "%s 0.0", probability_string);
				sprintf(density_string, "%s 0.0", density_string);

				/* output function is just a buffer since this is just a FF */
				fprintf(function_out, "subblock_function %s ", lut_node->name);
				for (l = 0; l < pow2(lut_size); l++)
				{
					if (l % 2 == 0)
					{
						fprintf(function_out, "0");
					}
					else
					{
						fprintf(function_out, "1");
					}
				}
				fprintf(function_out, "\n");
			}
			else
			{
				/* LUT */
				/* print out the function of this node based on it's just a LUT */
				oassert (lut_node->associated_function != NULL)
				fprintf(function_out, "subblock_function %s ", lut_node->name);
		
				for (l = 0; l < pow2(lut_size); l++)
				{
					fprintf(function_out, "%d", lut_node->associated_function[l]);
				}
	
				fprintf(function_out, "\n");
			}

			/* Finally, put in th output switching values */
			sprintf(probability_string, "%s %f", probability_string, act_data->static_probability[0]);
			sprintf(density_string, "%s %f", density_string,  act_data->transition_density[0]);

			/* output the values created for the subblock */
			fprintf(ac2_out, "%s\n", probability_string);
			fprintf(ac2_out, "%s\n", density_string);
		}
	}

	fclose(function_out);
	fclose(ac2_out);
}

/*---------------------------------------------------------------------------------------------
 * (function: cleanup_activation)
 *-------------------------------------------------------------------------------------------*/
void cleanup_activation(netlist_t *netlist)
{
	int i, j;

	for (i = 0; i < netlist->num_forward_levels; i++)
	{
		for (j = 0; j < netlist->num_at_forward_level[i]; j++)
		{
			/* initialize the activation data */
			nnode_t *current_node = netlist->forward_levels[i][j];
			activation_t *act_data = (activation_t*)current_node->node_data;
			oassert(act_data != NULL);

			if (act_data->static_probability != NULL)
				free(act_data->static_probability);
			if (act_data->transition_density != NULL)
				free(act_data->transition_density);
			if (act_data->transition_probability != NULL)
				free(act_data->transition_probability);

			free(act_data);
			current_node->unique_node_data_id = RESET;
		}
	}
}
