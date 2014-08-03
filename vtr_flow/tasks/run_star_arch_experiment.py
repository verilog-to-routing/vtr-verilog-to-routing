import os
import sys

def main():
    task = sys.argv[1]
    const_types = [1]
    vals = [80]
    delays = [1000]
    cuts = [1]
    constants = [1.0]


	allow_bidir_interposer_wires = "off"
	allow_chanx_conn = "on"
	
	allow_fanin_transfer = "off"
	allow_additional_fanin = "off"
	pct_interp_to_drive = 0
	
	allow_fanout_transfer = "off"
	allow_additional_fanout = "off"
	pct_interp_to_be_driven_by = 0
	
	if allow_fanout_transfer=="off" and allow_additional_fanout=="on":
		print "Error: allow_fanout_transfer must be ON if you want to use allow_additional_fanout"
		exit(1)
	if allow_fanin_transfer=="off" and allow_additional_fanin=="on":
		print "Error: allow_fanin_transfer must be ON if you want to use allow_additional_fanin"
		exit(1)
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
						os.system("../scripts/run_vtr_task.pl %s -p 7 -percent_wires_cut %d -num_cuts %d -delay_increase %d -placer_cost_constant %f -constant_type %d -allow_bidir_interposer_wires %s -allow_chanx_conn %s -allow_fanin_transfer %s -allow_additional_fanin %s -allow_fanout_transfer %s -allow_additional_fanout %s -pct_interp_to_be_driven_by %d -pct_interp_to_drive %d -graph_model star -graph_edge_weight 1/f -ub_factor 5" % (task, vals[p], cut, delays[i], c, t, allow_bidir_interposer_wires, allow_chanx_conn, allow_fanin_transfer, allow_additional_fanin, allow_fanout_transfer, allow_additional_fanout, pct_interp_to_be_driven_by, pct_interp_to_drive) )
						os.system("../scripts/parse_vtr_task.pl %s" % (task))
						print "Finished running with percent_wires_cut = %d" % (vals[p])
					print "\nFinished running with delay_increase = %d" % (delays[i])

main()
