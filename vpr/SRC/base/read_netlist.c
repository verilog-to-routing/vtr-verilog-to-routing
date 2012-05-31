/**
 * Author: Jason Luu
 * Date: May 2009
 * 
 * Read a circuit netlist in XML format and populate the netlist data structures for VPR
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "hash.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "ReadLine.h"
#include "globals.h"
#include "ezxml.h"
#include "read_xml_util.h"
#include "read_netlist.h"
#include "pb_type_graph.h"
#include "cluster_legality.h"
#include "token.h"
#include "rr_graph.h"

static void processPorts(INOUTP ezxml_t Parent, INOUTP t_pb* pb,
		INOUTP t_rr_node *rr_graph, INOUTP int *ncount,
		INOUTP struct s_hash **nhash);

static void processPb(INOUTP ezxml_t Parent, INOUTP t_pb* pb,
		INOUTP t_rr_node *rr_graph, INOUTP int *ncount,
		INOUTP struct s_hash **nhash, INOUTP int *num_primitives);
static void processComplexBlock(INOUTP ezxml_t Parent, INOUTP t_block *cb,
		INP int index, INOUTP int *ncount, INOUTP struct s_hash **nhash,
		INOUTP int *num_primitives, INP const t_arch *arch);
static struct s_net *alloc_and_init_netlist_from_hash(INP int ncount,
		INOUTP struct s_hash **nhash);

static int add_net_to_hash(INOUTP struct s_hash **nhash, INP char *net_name,
		INOUTP int *ncount);

static void load_external_nets_and_cb(INP int num_blocks,
		INP struct s_block block_list[], INP int ncount,
		INP struct s_net nlist[], OUTP int *ext_ncount,
		OUTP struct s_net **ext_nets, INP char **circuit_globals);

static void load_internal_cb_nets(INOUTP t_pb *top_level,
		INP t_pb_graph_node *pb_graph_node, INOUTP t_rr_node *rr_graph,
		INOUTP int * curr_net);

static void alloc_internal_cb_nets(INOUTP t_pb *top_level,
		INP t_pb_graph_node *pb_graph_node, INOUTP t_rr_node *rr_graph,
		INP int pass);

static void load_internal_cb_rr_graph_net_nums(INP t_rr_node * cur_rr_node,
		INP t_rr_node * rr_graph, INOUTP struct s_net * nets,
		INOUTP int * curr_net, INOUTP int * curr_sink);

static void mark_constant_generators(INP int num_blocks,
		INP struct s_block block_list[], INP int ncount,
		INOUTP struct s_net nlist[]);

static void mark_constant_generators_rec(INP t_pb *pb, INP t_rr_node *rr_graph,
		INOUTP struct s_net nlist[]);

static void restore_logical_block_from_saved_block(INP int iblk, INP t_pb *pb);

/**
 * Initializes the block_list with info from a netlist 
 * net_file - Name of the netlist file to read
 * num_blocks - number of CLBs in netlist 
 * block_list - array of blocks in netlist [0..num_blocks - 1]
 * num_nets - number of nets in netlist
 * net_list - nets in netlist [0..num_nets - 1]
 */
void read_netlist(INP const char *net_file, INP const t_arch *arch,
		OUTP int *L_num_blocks, OUTP struct s_block *block_list[],
		OUTP int *L_num_nets, OUTP struct s_net *net_list[]) {
	ezxml_t Cur, Prev, Top;
	int i, j;
	const char *Prop;
	int bcount;
	struct s_block *blist;
	int ncount, ext_ncount;
	struct s_net *nlist, *ext_nlist;
	struct s_hash **nhash;
	char **circuit_inputs, **circuit_outputs, **circuit_globals;
	int Count, Len;

	int num_primitives = 0;

	/* Parse the file */
	printf("Begin parsing packed FPGA netlist file\n");
	Top = ezxml_parse_file(net_file);
	if (NULL == Top) {
		printf(ERRTAG "Unable to load netlist file '%s'.\n", net_file);
		exit(1);
	}
	printf("Finished parsing packed FPGA netlist file\n");

	/* Root node should be block */
	CheckElement(Top, "block");

	/* Check top-level netlist attributes */
	Prop = FindProperty(Top, "name", TRUE);
	printf("Netlist generated from file %s\n", Prop);
	ezxml_set_attr(Top, "name", NULL);

	Prop = FindProperty(Top, "instance", TRUE);
	if (strcmp(Prop, "FPGA_packed_netlist[0]") != 0) {
		printf(
				ERRTAG "[Line %d] Expected instance to be \"FPGA_packed_netlist[0]\", found %s",
				Top->line, Prop);
		exit(1);
	}
	ezxml_set_attr(Top, "instance", NULL);

	/* Parse top-level netlist I/Os */
	Cur = FindElement(Top, "inputs", TRUE);
	circuit_inputs = GetNodeTokens(Cur);
	FreeNode(Cur);
	Cur = FindElement(Top, "outputs", TRUE);
	circuit_outputs = GetNodeTokens(Cur);
	FreeNode(Cur);

	Cur = FindElement(Top, "globals", TRUE);
	CountTokensInString(Cur->txt, &Count, &Len);
	if (Count > 0) {
		circuit_globals = GetNodeTokens(Cur);
	} else {
		circuit_globals = NULL;
	}
	FreeNode(Cur);

	/* Parse all CLB blocks and all nets*/
	bcount = CountChildren(Top, "block", 1);
	blist = (struct s_block *) my_calloc(bcount, sizeof(t_block));
	nhash = alloc_hash_table();
	ncount = 0;

	logical_block = (struct s_logical_block*) my_calloc(num_saved_logical_blocks,
			sizeof(t_logical_block));
	num_logical_blocks = num_saved_logical_blocks;

	Cur = Top->child;
	i = 0;
	while (Cur) {
		if (0 == strcmp(Cur->name, "block")) {
			CheckElement(Cur, "block");
			processComplexBlock(Cur, blist, i, &ncount, nhash, &num_primitives,
					arch);
			Prev = Cur;
			Cur = Cur->next;
			FreeNode(Prev);
			i++;
		} else {
			Cur = Cur->next;
		}
	}
	assert(i == bcount);
	if (saved_logical_blocks != NULL) {
		assert(num_primitives == num_saved_logical_blocks);
	}

	nlist = alloc_and_init_netlist_from_hash(ncount, nhash);
	mark_constant_generators(bcount, blist, ncount, nlist);
	load_external_nets_and_cb(bcount, blist, ncount, nlist, &ext_ncount,
			&ext_nlist, circuit_globals);

	/* TODO: create this function later
	 check_top_IO_matches_IO_blocks(circuit_inputs, circuit_outputs, circuit_globals, blist, bcount);
	 */

	free_hash_table(nhash);
	FreeTokens(&circuit_inputs);
	FreeTokens(&circuit_outputs);
	if (circuit_globals)
		FreeTokens(&circuit_globals);
	FreeNode(Top);

	/* load mapping between external nets and all nets */
	/* jluu TODO: Should use local variables here then assign to globals later, clean up later */
	vpack_net = nlist;
	num_logical_nets = ncount;
	clb_to_vpack_net_mapping = (int *) my_malloc(ext_ncount * sizeof(int));
	vpack_to_clb_net_mapping = (int *) my_malloc(ncount * sizeof(int));
	for (i = 0; i < ncount; i++) {
		vpack_to_clb_net_mapping[i] = OPEN;
	}

	for (i = 0; i < ext_ncount; i++) {
		for (j = 0; j < ncount; j++) {
			if (strcmp(ext_nlist[i].name, nlist[j].name) == 0) {
				clb_to_vpack_net_mapping[i] = j;
				vpack_to_clb_net_mapping[j] = i;
				break;
			}
		}
		assert(j != ncount);
	}

	if (saved_logical_blocks != NULL) {
		free(saved_logical_blocks);
		saved_logical_blocks = NULL;

		for (i = 0; i < num_logical_nets; i++) {
			free(saved_logical_nets[i].name);
			free(saved_logical_nets[i].node_block);
			free(saved_logical_nets[i].node_block_port);
			free(saved_logical_nets[i].node_block_pin);
		}
		free(saved_logical_nets);
		saved_logical_nets = NULL;
	}

	/* Return blocks and nets */
	*L_num_blocks = bcount;
	*block_list = blist;
	*L_num_nets = ext_ncount;
	*net_list = ext_nlist;
}

/**
 * XML parser to populate CLB info and to update nets with the nets of this CLB 
 * Parent - XML tag for this CLB
 * clb - Array of CLBs in the netlist
 * index - index of the CLB to allocate and load information into
 * ncount - number of nets recorded thus far in all CLBs
 * nhash - hashtable of all nets recorded thus far in all CLBs
 */
static void processComplexBlock(INOUTP ezxml_t Parent, INOUTP t_block *cb,
		INP int index, INOUTP int *ncount, INOUTP struct s_hash **nhash,
		INOUTP int *num_primitives, INP const t_arch *arch) {

	const char *Prop;
	boolean found;
	int num_tokens = 0;
	t_token *tokens;
	int i;
	const t_pb_type * pb_type = NULL;

	/* parse cb attributes */
	cb[index].pb = my_calloc(1, sizeof(t_pb));

	Prop = FindProperty(Parent, "name", TRUE);
	cb[index].name = my_strdup(Prop);
	cb[index].pb->name = my_strdup(Prop);
	ezxml_set_attr(Parent, "name", NULL);

	Prop = FindProperty(Parent, "instance", TRUE);
	tokens = GetTokensFromString(Prop, &num_tokens);
	ezxml_set_attr(Parent, "instance", NULL);
	if (num_tokens != 4 || tokens[0].type != TOKEN_STRING
			|| tokens[1].type != TOKEN_OPEN_SQUARE_BRACKET
			|| tokens[2].type != TOKEN_INT
			|| tokens[3].type != TOKEN_CLOSE_SQUARE_BRACKET) {
		printf(
				ERRTAG "[Line %d] Unknown syntax for instance %s in %s.  Expected pb_type[instance_number]\n",
				Parent->line, Prop, Parent->name);
		exit(1);
	}
	assert(my_atoi(tokens[2].data) == index);
	found = FALSE;
	for (i = 0; i < num_types; i++) {
		if (strcmp(type_descriptors[i].name, tokens[0].data) == 0) {
			cb[index].type = &type_descriptors[i];
			pb_type = cb[index].type->pb_type;
			found = TRUE;
			break;
		}
	}
	if (!found) {
		printf(ERRTAG "[Line %d] Unknown cb type %s for cb %s #%d\n",
				Parent->line, Prop, cb[index].name, index);
		exit(1);
	}

	/* Parse all pbs and CB internal nets*/
	cb[index].pb->logical_block = OPEN;
	cb[index].pb->pb_graph_node = cb[index].type->pb_graph_head;
	rr_node = my_calloc(cb[index].type->pb_graph_head->total_pb_pins,
			sizeof(t_rr_node));
	alloc_and_load_rr_graph_for_pb_graph_node(cb[index].pb->pb_graph_node, arch,
			0);
	cb[index].pb->rr_graph = rr_node;
	num_rr_nodes = cb[index].pb->pb_graph_node->total_pb_pins;

	Prop = FindProperty(Parent, "mode", TRUE);
	ezxml_set_attr(Parent, "mode", NULL);

	found = FALSE;
	for (i = 0; i < pb_type->num_modes; i++) {
		if (strcmp(Prop, pb_type->modes[i].name) == 0) {
			cb[index].pb->mode = i;
			found = TRUE;
		}
	}
	if (!found) {
		printf(ERRTAG "[Line %d] Unknown mode %s for cb %s #%d\n", Parent->line,
				Prop, cb[index].name, index);
		exit(1);
	}

	processPb(Parent, cb[index].pb, cb[index].pb->rr_graph, ncount, nhash,
			num_primitives);

	cb[index].nets = my_malloc(cb[index].type->num_pins * sizeof(int));
	for (i = 0; i < cb[index].type->num_pins; i++) {
		cb[index].nets[i] = OPEN;
	}
	alloc_internal_cb_nets(cb[index].pb, cb[index].pb->pb_graph_node,
			cb[index].pb->rr_graph, 1);
	alloc_internal_cb_nets(cb[index].pb, cb[index].pb->pb_graph_node,
			cb[index].pb->rr_graph, 2);
	i = 0;
	load_internal_cb_nets(cb[index].pb, cb[index].pb->pb_graph_node,
			cb[index].pb->rr_graph, &i);
	freeTokens(tokens, num_tokens);
#if 0
	/* TODO: may not need local nets anymore, reminder to remove if not used, keeping this around just in case we need to use it in the future */
	for(i = 0; i < cb[index].pb->num_local_nets; i++) {
		printf("local net %s: ", cb[index].pb->name);
		for(j = 0; j <= cb[index].pb->local_nets[i].num_sinks; j++) {
			printf("%d ", cb[index].pb->local_nets[i].node_block[j]);
		}
		printf("\n");
	}
#endif
}

/**
 * XML parser to populate pb info and to update internal nets of the parent CLB
 * Parent - XML tag for this pb_type
 * pb - physical block to use
 * ncount - number of all internal subblock nets recorded thus far in this CLB
 * nhash - hashtable of all internal subblock nets recorded thus far in this CLBs
 */
static void processPb(INOUTP ezxml_t Parent, INOUTP t_pb* pb,
		INOUTP t_rr_node *rr_graph, INOUTP int *ncount,
		INOUTP struct s_hash **nhash, INOUTP int *num_primitives) {
	ezxml_t Cur, Prev, lookahead;
	const char *Prop;
	const char *instance_type;
	int i, j, pb_index;
	boolean found;
	const t_pb_type *pb_type;
	t_token *tokens;
	int num_tokens;

	Cur = FindElement(Parent, "inputs", TRUE);
	processPorts(Cur, pb, rr_graph, ncount, nhash);
	FreeNode(Cur);
	Cur = FindElement(Parent, "outputs", TRUE);
	processPorts(Cur, pb, rr_graph, ncount, nhash);
	FreeNode(Cur);
	Cur = FindElement(Parent, "globals", TRUE);
	processPorts(Cur, pb, rr_graph, ncount, nhash);
	FreeNode(Cur);

	pb_type = pb->pb_graph_node->pb_type;
	if (pb_type->num_modes == 0) {
		pb->logical_block = *num_primitives;
		/* TODO: This info is not yet used.  Intention was to use later for error checking */
		if (saved_logical_blocks != NULL) {
			restore_logical_block_from_saved_block(*num_primitives, pb);
		}
		(*num_primitives)++;
	} else {
		/* process children of child if exists */

		pb->child_pbs = my_calloc(pb_type->modes[pb->mode].num_pb_type_children,
				sizeof(t_pb*));
		for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
			pb->child_pbs[i] = my_calloc(
					pb_type->modes[pb->mode].pb_type_children[i].num_pb,
					sizeof(t_pb));
		}

		/* Populate info for each physical block  */
		Cur = Parent->child;
		while (Cur) {
			if (0 == strcmp(Cur->name, "block")) {
				CheckElement(Cur, "block");

				instance_type = FindProperty(Cur, "instance", TRUE);
				tokens = GetTokensFromString(instance_type, &num_tokens);
				ezxml_set_attr(Cur, "instance", NULL);
				if (num_tokens != 4 || tokens[0].type != TOKEN_STRING
						|| tokens[1].type != TOKEN_OPEN_SQUARE_BRACKET
						|| tokens[2].type != TOKEN_INT
						|| tokens[3].type != TOKEN_CLOSE_SQUARE_BRACKET) {
					printf(
							ERRTAG "[Line %d] Unknown syntax for instance %s in %s.  Expected pb_type[instance_number]\n",
							Cur->line, instance_type, Cur->name);
					exit(1);
				}

				found = FALSE;
				pb_index = OPEN;
				for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children;
						i++) {
					if (strcmp(
							pb_type->modes[pb->mode].pb_type_children[i].name,
							tokens[0].data) == 0) {
						if (my_atoi(tokens[2].data)
								>= pb_type->modes[pb->mode].pb_type_children[i].num_pb) {
							printf(
									ERRTAG "[Line %d] Instance number exceeds # of pb available for instance %s in %s.\n",
									Cur->line, instance_type, Cur->name);
							exit(1);
						}
						pb_index = my_atoi(tokens[2].data);
						if (pb->child_pbs[i][pb_index].pb_graph_node != NULL) {
							printf(
									ERRTAG "[Line %d] node is used by two different blocks %s and %s\n",
									Cur->line, instance_type,
									pb->child_pbs[i][pb_index].name);
							exit(1);
						}
						pb->child_pbs[i][pb_index].pb_graph_node =
								&pb->pb_graph_node->child_pb_graph_nodes[pb->mode][i][pb_index];
						found = TRUE;
						break;
					}
				}
				if (!found) {
					printf(ERRTAG "[Line %d] Unknown pb type %s\n", Cur->line,
							instance_type);
					exit(1);
				}

				Prop = FindProperty(Cur, "name", TRUE);
				ezxml_set_attr(Cur, "name", NULL);
				if (0 != strcmp(Prop, "open")) {
					pb->child_pbs[i][pb_index].name = my_strdup(Prop);

					/* Parse all pbs and CB internal nets*/
					pb->child_pbs[i][pb_index].logical_block = OPEN;

					Prop = FindProperty(Cur, "mode", FALSE);
					if (Prop) {
						ezxml_set_attr(Cur, "mode", NULL);
					}
					pb->child_pbs[i][pb_index].mode = 0;
					found = FALSE;
					for (j = 0;
							j
									< pb->child_pbs[i][pb_index].pb_graph_node->pb_type->num_modes;
							j++) {
						if (strcmp(Prop,
								pb->child_pbs[i][pb_index].pb_graph_node->pb_type->modes[j].name)
								== 0) {
							pb->child_pbs[i][pb_index].mode = j;
							found = TRUE;
						}
					}
					if (!found
							&& pb->child_pbs[i][pb_index].pb_graph_node->pb_type->num_modes
									!= 0) {
						printf(
								ERRTAG "[Line %d] Unknown mode %s for cb %s #%d\n",
								Cur->line, Prop,
								pb->child_pbs[i][pb_index].name, pb_index);
						exit(1);
					}
					pb->child_pbs[i][pb_index].parent_pb = pb;
					pb->child_pbs[i][pb_index].rr_graph = pb->rr_graph;

					processPb(Cur, &pb->child_pbs[i][pb_index], rr_graph,
							ncount, nhash, num_primitives);
				} else {
					/* physical block has no used primitives but it may have used routing */
					pb->child_pbs[i][pb_index].name = NULL;
					pb->child_pbs[i][pb_index].logical_block = OPEN;
					lookahead = FindElement(Cur, "outputs", FALSE);
					if (lookahead != NULL) {
						lookahead = FindElement(lookahead, "port", TRUE);
						Prop = FindProperty(Cur, "mode", FALSE);
						if (Prop) {
							ezxml_set_attr(Cur, "mode", NULL);
						}
						pb->child_pbs[i][pb_index].mode = 0;
						found = FALSE;
						for (j = 0;
								j
										< pb->child_pbs[i][pb_index].pb_graph_node->pb_type->num_modes;
								j++) {
							if (strcmp(Prop,
									pb->child_pbs[i][pb_index].pb_graph_node->pb_type->modes[j].name)
									== 0) {
								pb->child_pbs[i][pb_index].mode = j;
								found = TRUE;
							}
						}
						if (!found
								&& pb->child_pbs[i][pb_index].pb_graph_node->pb_type->num_modes
										!= 0) {
							printf(
									ERRTAG "[Line %d] Unknown mode %s for cb %s #%d\n",
									Cur->line, Prop,
									pb->child_pbs[i][pb_index].name, pb_index);
							exit(1);
						}
						pb->child_pbs[i][pb_index].parent_pb = pb;
						pb->child_pbs[i][pb_index].rr_graph = pb->rr_graph;
						processPb(Cur, &pb->child_pbs[i][pb_index], rr_graph,
								ncount, nhash, num_primitives);
					}
				}
				Prev = Cur;
				Cur = Cur->next;
				FreeNode(Prev);
				freeTokens(tokens, num_tokens);
			} else {
				Cur = Cur->next;
			}
		}
	}
}

/**
 * Allocates memory for nets and loads the name of the net so that it can be identified and loaded with
 * more complete information later
 * ncount - number of nets in the hashtable of nets
 * nhash - hashtable of nets
 * returns array of nets stored in hashtable
 */
static struct s_net *alloc_and_init_netlist_from_hash(INP int ncount,
		INOUTP struct s_hash **nhash) {
	struct s_net *nlist;
	struct s_hash_iterator hash_iter;
	struct s_hash *curr_net;
	int i;

	nlist = my_calloc(ncount, sizeof(struct s_net));

	hash_iter = start_hash_table_iterator();
	curr_net = get_next_hash(nhash, &hash_iter);
	while (curr_net != NULL) {
		assert(nlist[curr_net->index].name == NULL);
		nlist[curr_net->index].name = my_strdup(curr_net->name);
		nlist[curr_net->index].num_sinks = curr_net->count - 1;

		nlist[curr_net->index].node_block = my_malloc(
				curr_net->count * sizeof(int));
		nlist[curr_net->index].node_block_pin = my_malloc(
				curr_net->count * sizeof(int));
		nlist[curr_net->index].is_global = FALSE;
		for (i = 0; i < curr_net->count; i++) {
			nlist[curr_net->index].node_block[i] = OPEN;
			nlist[curr_net->index].node_block_pin[i] = OPEN;
		}
		curr_net = get_next_hash(nhash, &hash_iter);
	}
	return nlist;
}

/**
 * Adds net to hashtable of nets.  If the net is "open", then this is a keyword so do not add it.  
 * If the net already exists, increase the count on that net 
 */
static int add_net_to_hash(INOUTP struct s_hash **nhash, INP char *net_name,
		INOUTP int *ncount) {
	struct s_hash *hash_value;

	if (strcmp(net_name, "open") == 0) {
		return OPEN;
	}

	hash_value = insert_in_hash_table(nhash, net_name, *ncount);
	if (hash_value->count == 1) {
		assert(*ncount == hash_value->index);
		(*ncount)++;
	}
	return hash_value->index;
}

static void processPorts(INOUTP ezxml_t Parent, INOUTP t_pb* pb,
		t_rr_node *rr_graph, INOUTP int *ncount, INOUTP struct s_hash **nhash) {

	int i, j, in_port, out_port, clock_port, num_tokens;
	ezxml_t Cur, Prev;
	const char *Prop;
	char **pins;
	char *port_name, *interconnect_name;
	int rr_node_index;
	t_pb_graph_pin *** pin_node;
	int *num_ptrs, num_sets;
	boolean found;

	Cur = Parent->child;
	while (Cur) {
		if (0 == strcmp(Cur->name, "port")) {
			CheckElement(Cur, "port");

			Prop = FindProperty(Cur, "name", TRUE);
			ezxml_set_attr(Cur, "name", NULL);

			in_port = out_port = clock_port = 0;
			found = FALSE;
			for (i = 0; i < pb->pb_graph_node->pb_type->num_ports; i++) {
				if (0
						== strcmp(pb->pb_graph_node->pb_type->ports[i].name,
								Prop)) {
					found = TRUE;
					break;
				}
				if (pb->pb_graph_node->pb_type->ports[i].is_clock
						&& pb->pb_graph_node->pb_type->ports[i].type
								== IN_PORT) {
					clock_port++;
				} else if (!pb->pb_graph_node->pb_type->ports[i].is_clock
						&& pb->pb_graph_node->pb_type->ports[i].type
								== IN_PORT) {
					in_port++;
				} else {
					assert(
							pb->pb_graph_node->pb_type->ports[i].type == OUT_PORT);
					out_port++;
				}
			}
			if (!found) {
				printf(ERRTAG "[Line %d] Unknown port %s for pb %s[%d]\n",
						Cur->line, Prop, pb->pb_graph_node->pb_type->name,
						pb->pb_graph_node->placement_index);
				exit(1);
			}

			pins = GetNodeTokens(Cur);
			num_tokens = CountTokens(pins);
			if (0 == strcmp(Parent->name, "inputs")) {
				if (num_tokens != pb->pb_graph_node->num_input_pins[in_port]) {
					printf(
							ERRTAG "[Line %d] Incorrect # pins %d found for port %s for pb %s[%d]\n",
							Cur->line, num_tokens, Prop,
							pb->pb_graph_node->pb_type->name,
							pb->pb_graph_node->placement_index);
					exit(1);
				}
			} else if (0 == strcmp(Parent->name, "outputs")) {
				if (num_tokens
						!= pb->pb_graph_node->num_output_pins[out_port]) {
					printf(
							ERRTAG "[Line %d] Incorrect # pins %d found for port %s for pb %s[%d]\n",
							Cur->line, num_tokens, Prop,
							pb->pb_graph_node->pb_type->name,
							pb->pb_graph_node->placement_index);
					exit(1);
				}
			} else {
				if (num_tokens
						!= pb->pb_graph_node->num_clock_pins[clock_port]) {
					printf(
							ERRTAG "[Line %d] Incorrect # pins %d found for port %s for pb %s[%d]\n",
							Cur->line, num_tokens, Prop,
							pb->pb_graph_node->pb_type->name,
							pb->pb_graph_node->placement_index);
					exit(1);
				}
			}
			if (0 == strcmp(Parent->name, "inputs")
					|| 0 == strcmp(Parent->name, "globals")) {
				if (pb->parent_pb == NULL) {
					/* top-level, connections are nets to route */
					for (i = 0; i < num_tokens; i++) {
						if (0 == strcmp(Parent->name, "inputs"))
							rr_node_index =
									pb->pb_graph_node->input_pins[in_port][i].pin_count_in_cluster;
						else
							rr_node_index =
									pb->pb_graph_node->clock_pins[clock_port][i].pin_count_in_cluster;
						rr_graph[rr_node_index].net_num = add_net_to_hash(nhash,
								pins[i], ncount);
					}
				} else {
					for (i = 0; i < num_tokens; i++) {
						if (0 == strcmp(pins[i], "open")) {
							continue;
						}
						interconnect_name = strstr(pins[i], "->");
						*interconnect_name = '\0';
						interconnect_name += 2;
						port_name = pins[i];
						pin_node =
								alloc_and_load_port_pin_ptrs_from_string(
										pb->pb_graph_node->parent_pb_graph_node,
										pb->pb_graph_node->parent_pb_graph_node->child_pb_graph_nodes[pb->parent_pb->mode],
										port_name, &num_ptrs, &num_sets, TRUE,
										TRUE);
						assert(num_sets == 1 && num_ptrs[0] == 1);
						if (0 == strcmp(Parent->name, "inputs"))
							rr_node_index =
									pb->pb_graph_node->input_pins[in_port][i].pin_count_in_cluster;
						else
							rr_node_index =
									pb->pb_graph_node->clock_pins[clock_port][i].pin_count_in_cluster;
						rr_graph[rr_node_index].prev_node =
								pin_node[0][0]->pin_count_in_cluster;
						found = FALSE;
						for (j = 0; j < pin_node[0][0]->num_output_edges; j++) {
							if (0
									== strcmp(interconnect_name,
											pin_node[0][0]->output_edges[j]->interconnect->name)) {
								found = TRUE;
								break;
							}
						}
						for (j = 0; j < num_sets; j++) {
							free(pin_node[j]);
						}
						free(pin_node);
						free(num_ptrs);
						if (!found) {
							printf(
									ERRTAG "[Line %d] Unknown interconnect %s connecting to pin %s\n",
									Cur->line, interconnect_name, port_name);
							exit(1);
						}
					}
				}
			}

			if (0 == strcmp(Parent->name, "outputs")) {
				if (pb->pb_graph_node->pb_type->num_modes == 0) {
					/* primitives are drivers of nets */
					for (i = 0; i < num_tokens; i++) {
						rr_node_index =
								pb->pb_graph_node->output_pins[out_port][i].pin_count_in_cluster;
						rr_graph[rr_node_index].net_num = add_net_to_hash(nhash,
								pins[i], ncount);
					}
				} else {
					for (i = 0; i < num_tokens; i++) {
						if (0 == strcmp(pins[i], "open")) {
							continue;
						}
						interconnect_name = strstr(pins[i], "->");
						*interconnect_name = '\0';
						interconnect_name += 2;
						port_name = pins[i];
						pin_node =
								alloc_and_load_port_pin_ptrs_from_string(
										pb->pb_graph_node,
										pb->pb_graph_node->child_pb_graph_nodes[pb->mode],
										port_name, &num_ptrs, &num_sets, TRUE,
										TRUE);
						assert(num_sets == 1 && num_ptrs[0] == 1);
						rr_node_index =
								pb->pb_graph_node->output_pins[out_port][i].pin_count_in_cluster;
						rr_graph[rr_node_index].prev_node =
								pin_node[0][0]->pin_count_in_cluster;
						found = FALSE;
						for (j = 0; j < pin_node[0][0]->num_output_edges; j++) {
							if (0
									== strcmp(interconnect_name,
											pin_node[0][0]->output_edges[j]->interconnect->name)) {
								found = TRUE;
								rr_graph[rr_node_index].fan_in++;
								rr_graph[pin_node[0][0]->pin_count_in_cluster].edges[j] =
										rr_node_index;
								break;
							}
						}
						for (j = 0; j < num_sets; j++) {
							free(pin_node[j]);
						}
						free(pin_node);
						free(num_ptrs);
						if (!found) {
							printf(
									ERRTAG "[Line %d] Unknown interconnect %s connecting to pin %s\n",
									Cur->line, interconnect_name, port_name);
							exit(1);
						}
						interconnect_name -= 2;
						*interconnect_name = '-';
					}
				}
			}

			FreeTokens(&pins);

			Prev = Cur;
			Cur = Cur->next;
			FreeNode(Prev);
		} else {
			Cur = Cur->next;
		}
	}
}

/**  
 * This function updates the nets list and the connections between that list and the complex block
 */
static void load_external_nets_and_cb(INP int L_num_blocks,
		INP struct s_block block_list[], INP int ncount,
		INP struct s_net nlist[], OUTP int *ext_ncount,
		OUTP struct s_net **ext_nets, INP char **circuit_globals) {
	int i, j, k, ipin;
	struct s_hash **ext_nhash;
	t_rr_node *rr_graph;
	t_pb_graph_pin *pb_graph_pin;
	int *count;
	int netnum, num_tokens;

	*ext_ncount = 0;
	ext_nhash = alloc_hash_table();

	/* Assumes that complex block pins are ordered inputs, outputs, globals */

	/* Determine the external nets of complex block */
	for (i = 0; i < L_num_blocks; i++) {
		ipin = 0;
		if (block_list[i].type->pb_type->num_input_pins
				+ block_list[i].type->pb_type->num_output_pins
				+ block_list[i].type->pb_type->num_clock_pins
				!= block_list[i].type->num_pins
						/ block_list[i].type->capacity) {

			assert(0);
		}

		/* First determine nets external to complex blocks */
		assert(
				block_list[i].type->pb_type->num_input_pins + block_list[i].type->pb_type->num_output_pins + block_list[i].type->pb_type->num_clock_pins == block_list[i].type->num_pins / block_list[i].type->capacity);

		rr_graph = block_list[i].pb->rr_graph;
		for (j = 0; j < block_list[i].pb->pb_graph_node->num_input_ports; j++) {
			for (k = 0; k < block_list[i].pb->pb_graph_node->num_input_pins[j];
					k++) {
				pb_graph_pin =
						&block_list[i].pb->pb_graph_node->input_pins[j][k];
				assert(pb_graph_pin->pin_count_in_cluster == ipin);
				if (rr_graph[pb_graph_pin->pin_count_in_cluster].net_num
						!= OPEN) {
					block_list[i].nets[ipin] =
							add_net_to_hash(ext_nhash,
									nlist[rr_graph[pb_graph_pin->pin_count_in_cluster].net_num].name,
									ext_ncount);
				} else {
					block_list[i].nets[ipin] = OPEN;
				}
				ipin++;
			}
		}
		for (j = 0; j < block_list[i].pb->pb_graph_node->num_output_ports;
				j++) {
			for (k = 0; k < block_list[i].pb->pb_graph_node->num_output_pins[j];
					k++) {
				pb_graph_pin =
						&block_list[i].pb->pb_graph_node->output_pins[j][k];
				assert(pb_graph_pin->pin_count_in_cluster == ipin);
				if (rr_graph[pb_graph_pin->pin_count_in_cluster].net_num
						!= OPEN) {
					block_list[i].nets[ipin] =
							add_net_to_hash(ext_nhash,
									nlist[rr_graph[pb_graph_pin->pin_count_in_cluster].net_num].name,
									ext_ncount);
				} else {
					block_list[i].nets[ipin] = OPEN;
				}
				ipin++;
			}
		}
		for (j = 0; j < block_list[i].pb->pb_graph_node->num_clock_ports; j++) {
			for (k = 0; k < block_list[i].pb->pb_graph_node->num_clock_pins[j];
					k++) {
				pb_graph_pin =
						&block_list[i].pb->pb_graph_node->clock_pins[j][k];
				assert(pb_graph_pin->pin_count_in_cluster == ipin);
				if (rr_graph[pb_graph_pin->pin_count_in_cluster].net_num
						!= OPEN) {
					block_list[i].nets[ipin] =
							add_net_to_hash(ext_nhash,
									nlist[rr_graph[pb_graph_pin->pin_count_in_cluster].net_num].name,
									ext_ncount);
				} else {
					block_list[i].nets[ipin] = OPEN;
				}
				ipin++;
			}
		}
		for (j = ipin; j < block_list[i].type->num_pins; j++) {
			block_list[i].nets[ipin] = OPEN;
		}
	}

	/* alloc and partially load the list of external nets */
	(*ext_nets) = alloc_and_init_netlist_from_hash(*ext_ncount, ext_nhash);
	/* Load global nets */
	num_tokens = CountTokens(circuit_globals);
	for (i = 0; i < *ext_ncount; i++) {
		for (j = 0; j < num_tokens; j++) {
			if (strcmp(circuit_globals[j], (*ext_nets)[i].name) == 0) {
				(*ext_nets)[i].is_global = TRUE;
			}
		}
	}
	count = my_calloc(*ext_ncount, sizeof(int));

	/* complete load of external nets so that each net points back to the blocks */
	for (i = 0; i < L_num_blocks; i++) {
		ipin = 0;
		rr_graph = block_list[i].pb->rr_graph;
		for (j = 0; j < block_list[i].type->num_pins; j++) {
			netnum = block_list[i].nets[j];
			if (netnum != OPEN) {
				if (RECEIVER
						== block_list[i].type->class_inf[block_list[i].type->pin_class[j]].type) {
					count[netnum]++;
					assert(count[netnum] <= (*ext_nets)[netnum].num_sinks);
					(*ext_nets)[netnum].node_block[count[netnum]] = i;
					(*ext_nets)[netnum].node_block_pin[count[netnum]] = j;

					if ((*ext_nets)[netnum].is_global
							!= block_list[i].type->is_global_pin[j]) {
						if ((*ext_nets)[netnum].is_global) {
							printf(
									ERRTAG "Netlist attempts to connect global net %s to non global pin %d of block %s #%d\n",
									(*ext_nets)[netnum].name, j,
									block_list[i].name, j);
						} else {
							printf(
									ERRTAG "Netlist attempts to connect non-global net %s to global pin %d of block %s #%d\n",
									(*ext_nets)[netnum].name, j,
									block_list[i].name, j);
						}
						exit(1);
					}
				} else {
					assert(
							DRIVER == block_list[i].type->class_inf[block_list[i].type->pin_class[j]].type);
					assert((*ext_nets)[netnum].node_block[0] == OPEN);
					(*ext_nets)[netnum].node_block[0] = i;
					(*ext_nets)[netnum].node_block_pin[0] = j;
				}
			}
		}
	}
	free(count);
	free_hash_table(ext_nhash);
}

/* Recursive function that fills rr_graph of cb with net numbers starting at the given rr_node */
static int count_sinks_internal_cb_rr_graph_net_nums(
		INP t_rr_node * cur_rr_node, INP t_rr_node * rr_graph) {
	int i;
	int count = 0;

	for (i = 0; i < cur_rr_node->num_edges; i++) {
		if (&rr_graph[rr_graph[cur_rr_node->edges[i]].prev_node]
				== cur_rr_node) {
			assert(
					rr_graph[cur_rr_node->edges[i]].net_num == OPEN || rr_graph[cur_rr_node->edges[i]].net_num == cur_rr_node->net_num);
			count += count_sinks_internal_cb_rr_graph_net_nums(
					&rr_graph[cur_rr_node->edges[i]], rr_graph);
		}
	}
	if (count == 0) {
		return 1; /* terminal node */
	} else {
		return count;
	}
}

/* Recursive function that fills rr_graph of cb with net numbers starting at the given rr_node */
static void load_internal_cb_rr_graph_net_nums(INP t_rr_node * cur_rr_node,
		INP t_rr_node * rr_graph, INOUTP struct s_net * nets,
		INOUTP int * curr_net, INOUTP int * curr_sink) {
	int i;

	boolean terminal;
	terminal = TRUE;

	for (i = 0; i < cur_rr_node->num_edges; i++) {
		if (&rr_graph[rr_graph[cur_rr_node->edges[i]].prev_node]
				== cur_rr_node) {
			/* TODO: If multiple edges to same node (should not happen in reasonable design) this always
			 selects the last edge, need to be smart about it in future (ie. select fastest edge */
			assert(
					rr_graph[cur_rr_node->edges[i]].net_num == OPEN || rr_graph[cur_rr_node->edges[i]].net_num == cur_rr_node->net_num);
			rr_graph[cur_rr_node->edges[i]].net_num = cur_rr_node->net_num;
			rr_graph[cur_rr_node->edges[i]].prev_edge = i;
			load_internal_cb_rr_graph_net_nums(&rr_graph[cur_rr_node->edges[i]],
					rr_graph, nets, curr_net, curr_sink);
			terminal = FALSE;
		}
	}
	if (terminal == TRUE) {
		/* Since the routing node index is known, assign that instead of the more obscure node block */
		nets[*curr_net].node_block[*curr_sink] =
				cur_rr_node->pb_graph_pin->pin_count_in_cluster;
		nets[*curr_net].node_block_pin[*curr_sink] = OPEN;
		nets[*curr_net].node_block_port[*curr_sink] = OPEN;
		(*curr_sink)++;
	}
}

/* Load internal cb nets and fill rr_graph of cb with net numbers */
static void load_internal_cb_nets(INOUTP t_pb *top_level,
		INP t_pb_graph_node *pb_graph_node, INOUTP t_rr_node *rr_graph,
		INOUTP int * curr_net) {
	int i, j, k;
	const t_pb_type *pb_type;
	int temp, size;
	struct s_net * nets;

	pb_type = pb_graph_node->pb_type;

	nets = top_level->local_nets;

	temp = 0;

	if (pb_graph_node->parent_pb_graph_node == NULL) { /* determine nets driven from inputs at top level */
		*curr_net = 0;
		for (i = 0; i < pb_graph_node->num_input_ports; i++) {
			for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
				if (rr_graph[pb_graph_node->input_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					load_internal_cb_rr_graph_net_nums(
							&rr_graph[pb_graph_node->input_pins[i][j].pin_count_in_cluster],
							rr_graph, nets, curr_net, &temp);
					assert(temp == nets[*curr_net].num_sinks);
					temp = 0;
					size =
							strlen(pb_graph_node->pb_type->name)
									+ pb_graph_node->placement_index / 10
									+ i / 10 + j / 10
									+ pb_graph_node->input_pins[i][j].pin_count_in_cluster
											/ 10 + 26;
					nets[*curr_net].name = my_calloc(size, sizeof(char));
					sprintf(nets[*curr_net].name,
							"%s[%d].input[%d][%d].pin[%d]",
							pb_graph_node->pb_type->name,
							pb_graph_node->placement_index, i, j,
							pb_graph_node->input_pins[i][j].pin_count_in_cluster);
					(*curr_net)++;
				}
			}
		}
		for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
			for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
				if (rr_graph[pb_graph_node->clock_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					load_internal_cb_rr_graph_net_nums(
							&rr_graph[pb_graph_node->clock_pins[i][j].pin_count_in_cluster],
							rr_graph, nets, curr_net, &temp);
					assert(temp == nets[*curr_net].num_sinks);
					temp = 0;
					nets[*curr_net].is_global = TRUE;
					size =
							strlen(pb_graph_node->pb_type->name)
									+ pb_graph_node->placement_index / 10
									+ i / 10 + j / 10
									+ pb_graph_node->clock_pins[i][j].pin_count_in_cluster
											/ 10 + 26;
					nets[*curr_net].name = my_calloc(size, sizeof(char));
					sprintf(nets[*curr_net].name,
							"%s[%d].clock[%d][%d].pin[%d]",
							pb_graph_node->pb_type->name,
							pb_graph_node->placement_index, i, j,
							pb_graph_node->clock_pins[i][j].pin_count_in_cluster);
					(*curr_net)++;
				}
			}
		}
	}

	if (pb_type->blif_model != NULL) {
		/* This is a terminal node so it might drive nets, find and map the rr_graph path for those nets */
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
				if (rr_graph[pb_graph_node->output_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					load_internal_cb_rr_graph_net_nums(
							&rr_graph[pb_graph_node->output_pins[i][j].pin_count_in_cluster],
							rr_graph, nets, curr_net, &temp);
					assert(temp == nets[*curr_net].num_sinks);
					temp = 0;
					size =
							strlen(pb_graph_node->pb_type->name)
									+ pb_graph_node->placement_index / 10
									+ i / 10 + j / 10
									+ pb_graph_node->output_pins[i][j].pin_count_in_cluster
											/ 10 + 26;
					nets[*curr_net].name = my_calloc(size, sizeof(char));
					sprintf(nets[*curr_net].name,
							"%s[%d].output[%d][%d].pin[%d]",
							pb_graph_node->pb_type->name,
							pb_graph_node->placement_index, i, j,
							pb_graph_node->output_pins[i][j].pin_count_in_cluster);
					(*curr_net)++;
				}
			}
		}
	} else {
		/* Recurse down to primitives */
		for (i = 0; i < pb_type->num_modes; i++) {
			for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					load_internal_cb_nets(top_level,
							&pb_graph_node->child_pb_graph_nodes[i][j][k],
							rr_graph, curr_net);
				}
			}
		}
	}

	if (pb_graph_node->parent_pb_graph_node == NULL) { /* at top level */
		assert(*curr_net == top_level->num_local_nets);
	}
}

/* allocate space to store nets internal to cb 
 two pass algorithm, pass 1 count and allocate # nets, pass 2 determine # sinks
 */
static void alloc_internal_cb_nets(INOUTP t_pb *top_level,
		INP t_pb_graph_node *pb_graph_node, INOUTP t_rr_node *rr_graph,
		INP int pass) {
	int i, j, k;
	const t_pb_type *pb_type;
	int num_sinks;

	pb_type = pb_graph_node->pb_type;

	if (pb_graph_node->parent_pb_graph_node == NULL) { /* determine nets driven from inputs at top level */
		top_level->num_local_nets = 0;
		if (pass == 1)
			top_level->local_nets = NULL;
		for (i = 0; i < pb_graph_node->num_input_ports; i++) {
			for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
				if (rr_graph[pb_graph_node->input_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					if (pass == 2) {
						num_sinks =
								count_sinks_internal_cb_rr_graph_net_nums(
										&rr_graph[pb_graph_node->input_pins[i][j].pin_count_in_cluster],
										rr_graph);
						top_level->local_nets[top_level->num_local_nets].num_sinks =
								num_sinks;
						top_level->local_nets[top_level->num_local_nets].node_block =
								my_calloc(num_sinks, sizeof(int));
						top_level->local_nets[top_level->num_local_nets].node_block_port =
								my_calloc(num_sinks, sizeof(int));
						top_level->local_nets[top_level->num_local_nets].node_block_pin =
								my_calloc(num_sinks, sizeof(int));
					}
					top_level->num_local_nets++;
				}
			}
		}
		for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
			for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
				if (rr_graph[pb_graph_node->clock_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					if (pass == 2) {
						num_sinks =
								count_sinks_internal_cb_rr_graph_net_nums(
										&rr_graph[pb_graph_node->clock_pins[i][j].pin_count_in_cluster],
										rr_graph);
						top_level->local_nets[top_level->num_local_nets].num_sinks =
								num_sinks;
						top_level->local_nets[top_level->num_local_nets].node_block =
								my_calloc(num_sinks, sizeof(int));
						top_level->local_nets[top_level->num_local_nets].node_block_port =
								my_calloc(num_sinks, sizeof(int));
						top_level->local_nets[top_level->num_local_nets].node_block_pin =
								my_calloc(num_sinks, sizeof(int));
					}
					top_level->num_local_nets++;
				}
			}
		}
	}

	if (pb_type->blif_model != NULL) {
		/* This is a terminal node so it might drive nets, find and map the rr_graph path for those nets */
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
				if (rr_graph[pb_graph_node->output_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					if (pass == 2) {
						num_sinks =
								count_sinks_internal_cb_rr_graph_net_nums(
										&rr_graph[pb_graph_node->output_pins[i][j].pin_count_in_cluster],
										rr_graph);
						top_level->local_nets[top_level->num_local_nets].num_sinks =
								num_sinks;
						top_level->local_nets[top_level->num_local_nets].node_block =
								my_calloc(num_sinks, sizeof(int));
						top_level->local_nets[top_level->num_local_nets].node_block_port =
								my_calloc(num_sinks, sizeof(int));
						top_level->local_nets[top_level->num_local_nets].node_block_pin =
								my_calloc(num_sinks, sizeof(int));
					}
					top_level->num_local_nets++;
				}
			}
		}
	} else {
		/* Recurse down to primitives */
		for (i = 0; i < pb_type->num_modes; i++) {
			for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					alloc_internal_cb_nets(top_level,
							&pb_graph_node->child_pb_graph_nodes[i][j][k],
							rr_graph, pass);
				}
			}
		}
	}

	if (pb_graph_node->parent_pb_graph_node == NULL) { /* at top level */
		if (pass == 1) {
			top_level->local_nets = my_calloc(top_level->num_local_nets,
					sizeof(struct s_net));
		}
	}
}

static void mark_constant_generators(INP int L_num_blocks,
		INP struct s_block block_list[], INP int ncount,
		INOUTP struct s_net nlist[]) {
	int i;
	for (i = 0; i < L_num_blocks; i++) {
		mark_constant_generators_rec(block_list[i].pb,
				block_list[i].pb->rr_graph, nlist);
	}
}

static void mark_constant_generators_rec(INP t_pb *pb, INP t_rr_node *rr_graph,
		INOUTP struct s_net nlist[]) {
	int i, j;
	t_pb_type *pb_type;
	boolean const_gen;
	if (pb->pb_graph_node->pb_type->blif_model == NULL) {
		for (i = 0;
				i
						< pb->pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children;
				i++) {
			pb_type =
					&(pb->pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i]);
			for (j = 0; j < pb_type->num_pb; j++) {
				if (pb->child_pbs[i][j].name != NULL) {
					mark_constant_generators_rec(&(pb->child_pbs[i][j]),
							rr_graph, nlist);
				}
			}
		}
	} else if (strcmp(pb->pb_graph_node->pb_type->name, "inpad") != 0) {
		const_gen = TRUE;
		for (i = 0; i < pb->pb_graph_node->num_input_ports && const_gen == TRUE;
				i++) {
			for (j = 0;
					j < pb->pb_graph_node->num_input_pins[i]
							&& const_gen == TRUE; j++) {
				if (rr_graph[pb->pb_graph_node->input_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					const_gen = FALSE;
				}
			}
		}
		for (i = 0; i < pb->pb_graph_node->num_clock_ports && const_gen == TRUE;
				i++) {
			for (j = 0;
					j < pb->pb_graph_node->num_clock_pins[i]
							&& const_gen == TRUE; j++) {
				if (rr_graph[pb->pb_graph_node->clock_pins[i][j].pin_count_in_cluster].net_num
						!= OPEN) {
					const_gen = FALSE;
				}
			}
		}
		if (const_gen == TRUE) {
			printf("%s is a constant generator \n", pb->name);
			for (i = 0; i < pb->pb_graph_node->num_output_ports; i++) {
				for (j = 0; j < pb->pb_graph_node->num_output_pins[i]; j++) {
					if (rr_graph[pb->pb_graph_node->output_pins[i][j].pin_count_in_cluster].net_num
							!= OPEN) {
						nlist[rr_graph[pb->pb_graph_node->output_pins[i][j].pin_count_in_cluster].net_num].is_const_gen =
								TRUE;
					}
				}
			}
		}
	}
}

/* create logical block properties from saved block */
static void restore_logical_block_from_saved_block(INP int iblk, INP t_pb *pb) {
	int i;
	for (i = 0; i < num_saved_logical_blocks; i++) {
		if (pb->pb_graph_node->pb_type->model
				== saved_logical_blocks[i].model) {
			if (saved_logical_blocks[i].name != NULL
					&& strcmp(pb->name, saved_logical_blocks[i].name) == 0) {
				break;
			}
		}
	}
	assert(i != num_saved_logical_blocks);
	logical_block[iblk].name = saved_logical_blocks[i].name;
	logical_block[iblk].clb_index = UNDEFINED;
	logical_block[iblk].clock_net = saved_logical_blocks[i].clock_net;
	logical_block[iblk].clock_net_tnode = NULL;
	logical_block[iblk].index = iblk;
	logical_block[iblk].input_net_tnodes =
			saved_logical_blocks[i].input_net_tnodes;
	logical_block[iblk].input_nets = saved_logical_blocks[i].input_nets;
	logical_block[iblk].model = saved_logical_blocks[i].model;
	logical_block[iblk].output_net_tnodes =
			saved_logical_blocks[i].output_net_tnodes;
	logical_block[iblk].output_nets = saved_logical_blocks[i].output_nets;
	logical_block[iblk].pb = pb;
	logical_block[iblk].truth_table = saved_logical_blocks[i].truth_table;
	logical_block[iblk].type = saved_logical_blocks[i].type;
	logical_block[iblk].used_input_pins =
			saved_logical_blocks[i].used_input_pins;
	logical_block[iblk].packed_molecules = saved_logical_blocks[i].packed_molecules;

	saved_logical_blocks[i].name = NULL;
	saved_logical_blocks[i].input_net_tnodes = NULL;
	saved_logical_blocks[i].input_nets = NULL;
	saved_logical_blocks[i].output_net_tnodes = NULL;
	saved_logical_blocks[i].output_nets = NULL;
	saved_logical_blocks[i].truth_table = NULL;
	saved_logical_blocks[i].packed_molecules = NULL;
}

/* Free logical blocks of netlist */
void free_logical_blocks(void) {
	int iblk, i;
	t_model_ports *port;
	struct s_linked_vptr *tvptr, *next;

	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		port = logical_block[iblk].model->inputs;
		i = 0;
		while (port) {
			if (!port->is_clock) {
				free(logical_block[iblk].input_nets[i]);
				if (logical_block[iblk].input_net_tnodes) {
					if (logical_block[iblk].input_net_tnodes[i])
						free(logical_block[iblk].input_net_tnodes[i]);
				}
				i++;
			}
			port = port->next;
		}
		if (logical_block[iblk].input_net_tnodes) 
			free(logical_block[iblk].input_net_tnodes);
		
		if (logical_block[iblk].packed_molecules)
			free(logical_block[iblk].packed_molecules);

		free(logical_block[iblk].input_nets);
		port = logical_block[iblk].model->outputs;
		i = 0;
		while (port) {
			free(logical_block[iblk].output_nets[i]);
			if (logical_block[iblk].output_net_tnodes) {
				if (logical_block[iblk].output_net_tnodes[i])
					free(logical_block[iblk].output_net_tnodes[i]);
			}
			i++;
			port = port->next;
		}
		if (logical_block[iblk].output_net_tnodes) {
			free(logical_block[iblk].output_net_tnodes);
		}
		free(logical_block[iblk].output_nets);
		free(logical_block[iblk].name);
		tvptr = logical_block[iblk].truth_table;
		while (tvptr != NULL) {
			if (tvptr->data_vptr)
				free(tvptr->data_vptr);
			next = tvptr->next;
			free(tvptr);
			tvptr = next;
		}
	}
	free(logical_block);
}

/* Free  logical blocks of netlist */
void free_logical_nets(void) {
	int inet;

	for (inet = 0; inet < num_logical_nets; inet++) {
		free(vpack_net[inet].name);
		free(vpack_net[inet].node_block);
		free(vpack_net[inet].node_block_port);
		free(vpack_net[inet].node_block_pin);
	}
	free(vpack_net);
}

