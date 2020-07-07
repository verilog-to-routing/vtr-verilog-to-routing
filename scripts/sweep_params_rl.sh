#! /bin/bash

for CRIT in 0.5 0.6 0.7 0.8 0.9 
do
    ../vtr_flow/scripts/run_vtr_task.pl regression_tests/vtr_reg_nightly/vtr_reg_qor_chain_large -j 8 -s --place_move_stats move.stat --place_agent_gamma 0.01 --pack --place --simpleRL_agent_placement on --place_agent_algorithm e_greedy --place_reward_num 3 --place_dm_rlim 3 --place_crit_limit $CRIT --place_timing_cost_func 0
done
