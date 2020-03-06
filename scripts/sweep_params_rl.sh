#! /bin/bash

for INNER in 0.25 0.5 1.0 2.0 4.0 
do
    for FANOUT in 10 50 100 
    do
        for EPSILON in 0 0.0001 0.001 0.01  0.1  0.9
        do
            for GAMMA in 0 0.001 0.01 0.1 1
            do
                ../vtr_flow/scripts/run_vtr_task.pl regression_tests/vtr_reg_nightly/vtr_reg_qor_chain_large -j 8 -s --place_move_stats move.stat --place_high_fanout_net $FANOUT --inner_num $INNER --place_agent_epsilon $EPSILON --place_agent_gamma $GAMMA --pack --place
                ../vtr_flow/scripts/parse_vtr_task.pl regression_tests/vtr_reg_nightly/vtr_reg_qor_chain_large
                sed -i "1s/^/$INNER,$FANOUT,$EPSILON,$GAMMA\n/" ../vtr_flow/tasks/regression_tests/vtr_reg_nightly/vtr_reg_qor_chain_large/latest/qor_results.txt 
                sed -i 's/\s\+/,/g' ../vtr_flow/tasks/regression_tests/vtr_reg_nightly/vtr_reg_qor_chain_large/latest/qor_results.txt
            done
        done
    done
done
