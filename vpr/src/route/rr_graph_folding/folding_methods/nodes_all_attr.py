import os
import re

'''
------------------------------- NODES_ALL_ATTR folding method --------------------------------------

Every attribute of a node is stored into a node pattern

Data stored for each NODE:
ptn_idx

Data stored for each NODE PATTERN:
xlow
ylow
xhigh
yhigh
type
capacity
R
C
Segment Id
Direction
Side

Example NODE in xml:
<node id="11626" ptn_idx="0"/>

Example NODE PATTERN in xml:
<node_ptn capacity="1" direction="INC_DIR" id="11626" type="CHANX"><loc ptc="0" xhigh="8" xlow="5" yhigh="8" ylow="8"/>
<timing C="1.1619003e-13" R="404"/>
<segment segment_id="0"/>
</node_ptn>


------------------------------- NODES_ALL_ATTR folding method --------------------------------------

'''



# Indices into flat_graph object
NODE = 0
EDGE = 1
EDGE_COUNT = 2

XLOW = 0
YLOW = 1
XHIGH = 2
YHIGH = 3
TYPE = 4
R = 5
C = 6
CAPACITY = 7
SIDE = 8
PTC = 9
DIRECTION = 10
SEGMENT = 11

# Indices into folded_graph object
F_NODE_TO_PATTERN = 0
F_NODE_PATTERNS = 1
F_EDGES = 2
F_SEGMENT = 9 # segment is the only index that is different from flat -> folded

# Size constants
FLAT_NODE_BYTES = 16 # bytes per node
FLAT_EDGE_BYTES = 10 # bytes per edge

def fold_save_metrics(flat_graph, graph_name):
    folded_graph = fold(flat_graph)
    save(folded_graph, graph_name)
    metrics(flat_graph, folded_graph, graph_name)

def fold(graph):
    np = {} # node_patterns
    node_to_pattern = [] # mapping from node_id -> node pattern idx
    p_idx = 0
    for i, node in enumerate(graph[NODE]):
        cur_p = (node[XLOW], node[YLOW], node[XHIGH], node[YHIGH], node[TYPE], node[R],
                node[C], node[CAPACITY], node[SIDE], node[SEGMENT], node[DIRECTION],)
        if cur_p not in np:
            np[cur_p] = p_idx
            p_idx += 1
        node_to_pattern.append(np[cur_p])
    
    folded_graph = [node_to_pattern, np, graph[EDGE]]
    return folded_graph

def save(graph, graph_name):
    '''
    Saves the folded graph into an xml file with nodes and node_patterns
    '''
    save_file = f'{os.getcwd()}/folded_graphs/{name()}_{graph_name}.xml'
    flat_file = f'{os.getcwd()}/flat_graphs/{graph_name}.xml'
    print(f'Saving graph to {save_file}')
    with open(save_file, 'w') as folded_file:
        with open(flat_file, 'r') as file:
            for line in file:
                write_line = line
                if '<node' in line:
                    node = int(re.findall('id="([0-9]+)"', line)[0])
                    ptc = int(re.findall('ptc="([0-9]+)"', line)[0])
                    pattern_idx = graph[F_NODE_TO_PATTERN][node]
                    write_line = f'<node id="{node}" ptn_idx="{pattern_idx}" ptc="{ptc}"/>\n'
                if '</node>' in line:
                    continue
                if '<timing C="' in line or '<segment segment_id="' in line:
                    continue
                if '<rr_nodes>' in line:
                    folded_file.write('<rr_node_patterns>\n')
                    for ptn_idx, ptn in enumerate(graph[F_NODE_PATTERNS]):
                        direction = f'direction="{ptn[DIRECTION]}" ' if ptn[DIRECTION] else ''
                        side = f'side="{ptn[SIDE]}" ' if ptn[SIDE] else ''
                        ptn_line = f'<node_ptn capacity="{ptn[CAPACITY]}" {direction}id="{ptn_idx}" type="{ptn[TYPE]}"><loc {side}xhigh="{ptn[XHIGH]}" xlow="{ptn[XLOW]}" yhigh="{ptn[YHIGH]}" ylow="{ptn[YLOW]}"/>\n'
                        if ptn[R] is not None:
                            ptn_line += f'<timing C="{ptn[C]}" R="{ptn[R]}"/>\n'
                        if ptn[F_SEGMENT] is not None:
                            ptn_line += f'<segment segment_id="{ptn[F_SEGMENT]}"/>\n'
                        ptn_line += '</node_ptn>\n'
                        folded_file.write(ptn_line)
                    folded_file.write('</rr_node_patterns>\n')
                folded_file.write(write_line)



def metrics(flat_graph, folded_graph, graph_name):
    MiB = 1024*1024
    flat_nodes_size = len(flat_graph[NODE])*FLAT_NODE_BYTES/MiB
    flat_size = (len(flat_graph[NODE])*FLAT_NODE_BYTES + flat_graph[EDGE_COUNT]*FLAT_EDGE_BYTES)/MiB
    
    np_count = len(folded_graph[F_NODE_PATTERNS])
    node_count = len(folded_graph[F_NODE_TO_PATTERN])
    edge_count = flat_graph[EDGE_COUNT]
    folded_nodes_size = (node_count*4+np_count*FLAT_NODE_BYTES)/MiB
    folded_size = (np_count*FLAT_NODE_BYTES + node_count*4 + edge_count*FLAT_EDGE_BYTES)/MiB

    print(f'\n{graph_name} metrics [{name()}]:\n\t' \
          f'Node patterns: {np_count} ({np_count*FLAT_NODE_BYTES/MiB:.2f} MiB) [{100*np_count/node_count:.1f}%]\n\t' \
          f'Node to patterns: {node_count} ({node_count*4/MiB:.2f} MiB)\n\t' \
          f'Nodes: {node_count} ({flat_nodes_size:.2f} -> {folded_nodes_size:.2f} MiB) [{100*folded_nodes_size/flat_nodes_size:.1f}%]\n\t' \
          f'Edges: {edge_count} ({edge_count*FLAT_EDGE_BYTES/MiB:.2f} MiB)\n\t'
          f'Total Size: {flat_size:.2f} -> {folded_size:.2f} MiB [{100*folded_size/flat_size:.1f}%]')



def name():
    return 'nodes_all_attr'

if __name__ == '__main__':
    print(f"This folding method is '{name()}'")
    print(f"To fold an rr_graph with this method, run the following command from the directory one level up")
    print(f"\tpython fold_rr_graph.py {name()} <flat_graph_name>")