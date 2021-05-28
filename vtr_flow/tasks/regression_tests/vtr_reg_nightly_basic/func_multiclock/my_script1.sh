#!/bin/bash

RUN_SCRIPT='/home/ahmadi55/vtr-verilog-to-routing/vtr_flow/scripts/run_vtr_task.py'
PARSE_SCRIPT='/home/ahmadi55/vtr-verilog-to-routing/vtr_flow/scripts/python_libs/vtr/parse_vtr_task.py'
check='0'
for top_dir in *;do 
	if [ -d "$top_dir" ]; then 
		echo "outer dir: $top_dir"
		for inner_dir in $top_dir/*;do
			if grep -q "config" <<< "$inner_dir";then
				check=1
				break 						
			fi
			if grep -q "table_X" <<< "$inner_dir";then
				check=1
				continue  						
			fi
			echo "$inner_dir" >>~/Desktop/timing_results1.txt
			(time $RUN_SCRIPT $inner_dir) >>~/Desktop/run-results_test.txt  2>>~/Desktop/timing_results1_test.txt
			 	
		done
		echo "$outer_dir"
		if [[ "$check" == "1" ]]; then
			echo "$top_dir" >>~/Desktop/timing_results1_test.txt
			(time $RUN_SCRIPT $top_dir) >>~/Desktop/run-results_test.txt  2>>~/Desktop/timing_results1_test.txt
			
		fi 
	fi 
done 
