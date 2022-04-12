# This file is used to fold VTR's Routing Resource Graph
# Inputs include
#   - folding method
#   - flat_graph name
# author: Ethan Rogers @ethanroj23 (github)

import os
import sys
import re
sys.path.insert(0, os.getcwd()+'/folding_methods')

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

def read_flat_graph(graph_name):
    rr_graph_file = f'flat_graphs/{graph_name}.xml'
    print(f'Reading {graph_name} from file...')
    flat_graph = [[], []] # nodes, edges
    total_nodes = 0
    edge_count = 0
    with open(rr_graph_file, 'r') as file:
        for line in file:
            writeline = line
            if '<segment segment_id=' in line:
                segment_id = int(re.findall('segment_id="([0-9]+)"', line)[0])
                flat_graph[NODE][prev_id][SEGMENT] = segment_id
            if '<timing C=' in line:
                cap = re.findall('C="(.*?)"', line)[0]
                res = re.findall('R="(.*?)"', line)[0]
                flat_graph[NODE][prev_id][C] = cap
                flat_graph[NODE][prev_id][R] = res
            if '<node' in line:
                flat_graph[NODE] += [[]] # add new list for node
                total_nodes += 1
                if total_nodes % 1000000 == 0:
                    print(total_nodes)
                id = int(re.findall('id="([0-9]+)"', line)[0])
                prev_id = id
                xlow = int(re.findall('xlow="([0-9]+)"', line)[0])
                ylow = int(re.findall('ylow="([0-9]+)"', line)[0])
                xhigh = int(re.findall('xhigh="([0-9]+)"', line)[0])
                yhigh = int(re.findall('yhigh="([0-9]+)"', line)[0])
                capacity = int(re.findall('capacity="([0-9]+)"', line)[0])
                ptc = int(re.findall('ptc="([0-9]+)"', line)[0])
                direction = re.findall('direction="(.*?)"', line)
                if direction:
                    direction = direction[0]
                else:
                    direction = None
                side = re.findall('side="(.*?)"', line)
                if side:
                    side = side[0]
                else:
                    side = None
                node_type = re.findall('type="([A-Z]+)"', line)[0]
                # retrieve node
                flat_graph[NODE][id] = [xlow, ylow, xhigh, yhigh, node_type, None,  None, #R and C will be filled in later
                                    capacity, side, ptc, direction, None]

            if '<edge' in line:
                edge_count += 1
                sink_node = int(re.findall('sink_node="([0-9]+)"', line)[0])
                src_node = int(re.findall('src_node="([0-9]+)"', line)[0])
                switch_id = int(re.findall('switch_id="([0-9]+)"', line)[0])
                edge_diff = src_node - len(flat_graph[EDGE])
                if edge_diff >= 0:
                    for _ in range(edge_diff+1):
                        flat_graph[EDGE].append([])
                flat_graph[EDGE][src_node].append([sink_node, switch_id])

    print(f'Total nodes: {total_nodes}')
    print(f'Total edges: {edge_count}')
    flat_graph.append(edge_count)
    return flat_graph


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Please provide the: \n\t- folding method \n\t- graph_name if desired (If no graph_name is provided, all graphs in the flat_graphs/ directory will be folded)")
        print("\tPotential Folding methods:")
        for file in os.listdir('folding_methods'):
            if '.py' in file:
                print(f'\t\t{file[:-3]}')
        exit()
    graphs_to_fold = []
    if len(sys.argv) == 2:
        for file in os.listdir('flat_graphs'):
            graphs_to_fold.append(file[:-4])
    elif len(sys.argv) == 3:
        graphs_to_fold.append(sys.argv[2])
    
    # Dynamically import the folding module based on the input argument
    folding_method = sys.argv[1]
    folding_module = __import__(folding_method)
    print(f'Using folding method {folding_module.name()}')

    # Iterate over all graphs to fold and fold/save them
    for graph_name in graphs_to_fold:
        flat_graph = read_flat_graph(graph_name)
        folding_module.fold_save_metrics(flat_graph, graph_name)

        

# import importlib
# my_module = importlib.import_module("module.submodule")
# MyClass = getattr(my_module, "MyClass")
# instance = MyClass()
