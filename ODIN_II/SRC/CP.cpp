void dfs(nnode_t *node, int mark, netlist_t *netlist) {

	if (node->traverse_visited != traverse_mark_number) {
        node->critical_path_value = MAX_INT;
		node->traverse_visited = mark;
		for (int i = 0; i < node->num_output_pins; i++) {
			if (node->output_pins[i]->net != NULL) {
				nnet_t *next_net = node->output_pins[i]->net;
				next_net->critical_path_value++;
				if(next_net->fanout_pins) {
					for (int j = 0; j < next_net->num_fanout_pins; j++)
					{
						if (next_net->fanout_pins[j]) {
							if (next_net->fanout_pins[j]->node)
								dfs(next_net->fanout_pins[j]->node, mark, netlist);
						}
					}
				}
			}
		}

        for (int i = 0; i < node->num_output_pins; i++) {
			if (node->output_pins[i]->net != NULL) {
				nnet_t *next_net = node->output_pins[i]->net;
                if ( node->critical_path_value < next_net->critical_path_value )
                    node->critical_path_value = next_net->critical_path_value;
			}
		}


	}
}

void dfs_top(short mk, netlist_t *netlist)
{

	for (int i = 0; i < netlist->num_top_input_nodes; i++) {
		if (netlist->top_input_nodes[i] != NULL)
			dfs(netlist->top_input_nodes[i], mk, netlist);
	}

	dfs(netlist->gnd_node, mk, netlist);
	dfs(netlist->vcc_node, mk, netlist);
	dfs(netlist->pad_node, mk, netlist);
}