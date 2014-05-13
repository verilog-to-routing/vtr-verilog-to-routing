import sys

def edge_weight_1overf(vertices):
    return 1.0/len(vertices)

def edge_weight_1overf2(vertices):
    return 1.0/(len(vertices) * len(vertices))

def edge_weight_1(vertices):
    return 1.0

def generate_edges_from_hedge_clique(hedge_parts, edges, edge_weight_fn):
    # for each pair of vertices in this hedge, add an edge
    parts = sorted(hedge_parts)
    for i in range(len(parts)-1):
        for j in range(i+1,len(parts)):
            # need the -1s because the file is 1-indexed but we're 0-indexed
            edge = (parts[i]-1,parts[j]-1)
            weight = edge_weight_fn(hedge_parts)
            if edge in edges:
                edges[edge] += weight
            else:
                edges[edge] = weight

def generate_edges_from_hedge_star(hedge_parts, edges, edge_weight_fn):
    # add an edge from source to each dest
    for part in hedge_parts[1:]:
        edge = (hedge_parts[0]-1, part-1)
        weight = edge_weight_fn(hedge_parts)
        if edge in edges:
            edges[edge] += weight
        else:
            edges[edge] = weight

def read_and_transform_hypergraph(btypes, block_to_type, infp, outfp, graph_model_fn):
    # map from edge to edge weight
    # purpose of the first part is to populate this edges map
    edges = {}
    extra_lines = []
    with open(infp) as f:
        first_line = True
        num_hedges_read = 0
        for line in f:
            comment_idx = line.find("//")
            if comment_idx > -1:
                line = line[0:comment_idx]
            line = line.strip()
            if len(line) == 0:
                continue
            parts = map(lambda x: int(x), line.strip().split())
            if first_line:
                num_vertices = parts[1]
                num_hedges = parts[0]
                first_line = False
                continue
            # the hyperedge is a list of vertex indices
            num_hedges_read += 1
            graph_model_fn(hedge_parts=parts, edges=edges)

        print num_hedges
        print num_hedges_read
        assert(num_hedges == num_hedges_read)

    # edges map is done. now dump it!
    smallest_edge_weight = None
    # make edge weights integer
    for edge, weight in edges.items():
        if smallest_edge_weight:
            smallest_edge_weight = min(smallest_edge_weight, weight)
        else:
            smallest_edge_weight = weight

    for edge in edges:
        edges[edge] = edges[edge] / smallest_edge_weight
    # transform edge data into vertex data
    vertex_data = {}
    for edge, weight in edges.items():
        vertex_a = edge[0]
        vertex_b = edge[1]
        if vertex_a in vertex_data:
            vertex_data[vertex_a].append((vertex_b, weight))
        else:
            vertex_data[vertex_a] = [(vertex_b, weight)]
        if vertex_b in vertex_data:
            vertex_data[vertex_b].append((vertex_a, weight))
        else:
            vertex_data[vertex_b] = [(vertex_a, weight)]

    with open(outfp,"w") as f:
        f.write("%d %d 011 %d\n" % (num_vertices, len(edges), len(btypes)))
        for vertex_index in range(num_vertices):
            # the vertex weight - what type is it?
            for i in range(len(btypes)):
                if i == block_to_type[vertex_index]:
                    f.write("1 ")
                else:
                    f.write("0 ")
            for edge in vertex_data[vertex_index]:
                # need the +1 to go back to 1-indexed
                f.write("%d %d " % (edge[0]+1, edge[1]))
            f.write("\n")

def read_blocks(filepath):
    btypes = []
    block_to_type = []
    with open(filepath) as f:
        for line in f:
            btype = line.strip().split()[1]
            if btype not in btypes:
                btypes.append(btype)
            idx = btypes.index(btype)
            block_to_type.append(idx)

    return btypes, block_to_type

def run(blocks_file, input_hypergraph, output_graph, graph_model, graph_edge_weight):
    btypes, block_to_type = read_blocks(blocks_file)

    def graph_model_fn(hedge_parts, edges):
        if graph_model == "star":
            gen_edge_fn = generate_edges_from_hedge_star
        elif graph_model == "clique":
            gen_edge_fn = generate_edges_from_hedge_clique
        else:
            assert(False)

        if graph_edge_weight == "1/f":
            edge_weight_fn = edge_weight_1overf
        elif graph_edge_weight == "1/f2":
            edge_weight_fn = edge_weight_1overf2
        elif graph_edge_weight == "1":
            edge_weight_fn = edge_weight_1

        gen_edge_fn(hedge_parts = hedge_parts, edges=edges, edge_weight_fn = edge_weight_fn)

    read_and_transform_hypergraph(btypes, block_to_type, input_hypergraph, output_graph, graph_model_fn)

def main():
    blocks_file = sys.argv[1]
    input_hypergraph = sys.argv[2]
    output_graph = sys.argv[3]

    run(blocks_file, input_hypergraph, output_graph)

if __name__=="__main__":
    main()
