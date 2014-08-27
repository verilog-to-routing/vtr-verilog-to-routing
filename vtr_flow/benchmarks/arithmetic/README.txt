README for the aritmetic benchmarks.

each benchmark has a bash script called run_${benchmark_name}.bash
each script does four things:
	dup_sources
		duplicates/generates the verilog sources for this benchmark
	task
		run the task for this benchmark
	parse
		run the parse_vtr_task script
	extract
		reparse the output from [parse] and put it in ${benchmark_name}_final_results.tsv

	Running the script with no argumets does all four, and running it with the first argument the
	name of a part will run just that part. Addidionally, when extract is specified, the script
	will also accept another, integer, argument, which will specifiy which run to extract from.