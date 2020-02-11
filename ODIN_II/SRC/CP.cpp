
void dfs_up(nnode_t *node, int mark, netlist_t *netlist, int value) {

	if (node->traverse_visited != traverse_mark_number) {
        node->critical_path_value_up = MAX_INT;
		node->traverse_visited = mark;
		for (int i = 0; i < node->num_output_pins; i++) {
			if (node->output_pins[i]->net != NULL) {
				nnet_t *next_net = node->output_pins[i]->net;
				next_net->critical_path_value_up = value;
				if(next_net->fanout_pins) {
					for (int j = 0; j < next_net->num_fanout_pins; j++)
					{
						if (next_net->fanout_pins[j]) {
							if (next_net->fanout_pins[j]->node)
								dfs_up(next_net->fanout_pins[j]->node, mark, netlist, value+1);
						}
					}
				}
			}
		}

        for (int i = 0; i < node->num_output_pins; i++) {
			if (node->output_pins[i]->net != NULL) {
				nnet_t *next_net = node->output_pins[i]->net;
                if ( node->critical_path_value_up < next_net->critical_path_value_up )
                    node->critical_path_value_up = next_net->critical_path_value_up;
			}
		}


	}
}

void dfs_down(nnode_t *node, int mark, netlist_t *netlist, int value) {

	if (node->traverse_visited != traverse_mark_number) {
        node->critical_path_value_down = MAX_INT;
		node->traverse_visited = mark;
		for (int i = 0; i < node->num_input_pins; i++) {
			if (node->input_pins[i]->net != NULL) {
				nnet_t *next_net = node->input_pins[i]->net;
				next_net->critical_path_value_down = value;
				if(next_net->driver_pin) {
                    if (next_net->driver_pin->node)
                        dfs_down(next_net-> driver_pin->node, mark, netlist, value+1);
				}
			}
		}

        for (int i = 0; i < node->num_input_pins; i++) {
			if (node->input_pins[i]->net != NULL) {
				nnet_t *next_net = node->output_pins[i]->net;
                if ( node->critical_path_value_down < next_net->critical_path_value_down )
                    node->critical_path_value_down = next_net->critical_path_value_down;
			}
		}


	}
}

void dfs_top(short mk, netlist_t *netlist)
{

	for (int i = 0; i < netlist->num_top_input_nodes; i++) {
		if (netlist->top_input_nodes[i] != NULL)
			dfs_up(netlist->top_input_nodes[i], mk, netlist, 0);
	}

	dfs(netlist->gnd_node, mk, netlist, 0);
	dfs(netlist->vcc_node, mk, netlist, 0);
	dfs(netlist->pad_node, mk, netlist, 0);

    for (int i = 0; i < netlist->num_top_output_nodes; i++) {
		if (netlist->top_output_nodes[i] != NULL)
			dfs_down(netlist->top_output_nodes[i], mk, netlist, 0);
	}

}