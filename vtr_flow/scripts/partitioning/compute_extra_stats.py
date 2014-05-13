import os
import sys
import glob
sys.path.append('/usr/local/google/home/jvds/projects/vpr_net_to_hmetis')
import compute_placement_compliance
import compute_hyperedge_cut

base_dir = os.getcwd()

dirs = map(lambda x: os.path.abspath(x), sys.argv[1:])

for d in dirs:
    os.chdir(d)
    # get the hyperedge cut
    hec = compute_hyperedge_cut.run('hypergraph.txt', 'justgraph.txt.part.2')
    with open('hyperedge_cut.txt', 'w') as f:
        f.write("%d\n" % hec)

    # get the placement compliance
    place_file = glob.glob('*.place')[0]
    count_good, count_bad = compute_placement_compliance.run(place_file, 'packed_partitioned.net')
    with open('placement_compliance.txt', 'w') as f:
        f.write("%f\n" % (float(count_good)/float(count_good+count_bad)))
