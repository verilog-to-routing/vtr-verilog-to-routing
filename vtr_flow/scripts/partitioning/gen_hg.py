import sys
import xml.etree.ElementTree as ET

def get_block_nets(block_element):
    port_elements = (block_element.find('inputs').findall('port')
        + block_element.find('outputs').findall('port')
        + block_element.find('clocks').findall('port'))

    nets = (set(reduce(lambda a,b:a+b,
        map(lambda port_element: port_element.text.strip().split(),
        port_elements))) - set(['open']))

    # subblock names are also nets for this block... kind of weird but vpr says
    # so!
    nets = nets.union(set(map(lambda x: x.get('name'), block_element.findall('block'))))

    return filter(lambda x: "->" not in x, nets)

def add_block_nets_to_hypergraph(block_name, block_nets, hypergraph):
    for net in block_nets:
        if net not in hypergraph:
            hypergraph[net] = []
        hypergraph[net].append(block_name)

def parse_signal_index(signal):
    open_bracket = signal.index('[')
    close_bracket = signal.index(']')
    index = int(signal[open_bracket+1:close_bracket])

    return index

def main():
    input_net_file = sys.argv[1]
    tree = ET.parse(input_net_file)
    root = tree.getroot()

    # dictionary to store the hypergraph. key = net name, value = [block name]
    hypergraph = {}

    for child in root:
        if child.tag == "block":
            block_nets = get_block_nets(child)
            if child.get('instance') == 'clb[74]':
                print "JVD: %s" % block_nets
            add_block_nets_to_hypergraph(child.get('instance'), block_nets,
                hypergraph)

    for net, blocks in hypergraph.items():
        block_numbers = map(lambda x: str(parse_signal_index(x)+1), blocks)
        print "%s //%s" % (" ".join(block_numbers), net)

main()
