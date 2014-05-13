import os
import sys

def main():
    task = sys.argv[1]
    const_types = [1]
    vals = [80]
    delays = [1000]
    cuts = [1]
    constants = [1.0]

    for t in const_types:
        print "\nRunning with CONSTANT TYPE = %d\n" % (t)
        for c in constants:
            print "\nRunning with CONSTANT = %f\n" % c
            for cut in cuts:
                print "\n\n Running with num_cuts = %d\n" % (cut)

                for i in xrange(len(delays)):
                    print "\nRunning with delay_increase = %d" % (delays[i])

                    for p in xrange(len(vals)):
                        print "Running with percent_wires_cut = %d" % (vals[p])
                        os.system("../scripts/run_vtr_task.pl %s -p 7 -percent_wires_cut %d -num_cuts %d -delay_increase %d -placer_cost_constant %f -constant_type %d -graph_model star -graph_edge_weight 1/f -ub_factor 5" % (task, vals[p], cut, delays[i], c, t) )
                        os.system("../scripts/parse_vtr_task.pl %s" % (task))
                        print "Finished running with percent_wires_cut = %d" % (vals[p])

                    print "\nFinished running with delay_increase = %d" % (delays[i])

main()
