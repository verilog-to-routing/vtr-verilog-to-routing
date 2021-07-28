##ODIN-II Regression 

`task_list.txt` in this directory points to a set of tasks in the VTR regression suite, `vtr_reg_nightly_test2`. These tasks are referred to in  
[`light_suite`](https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/ODIN_II/regression_test/benchmark/suite/light_suite/task_list.conf) which is part of a series of ODIN-II regression tests. `light_suite` is currently being run by CI under the name`odin_reg_basic`. Not the whole of VTR_flow is tested in ODIN-II regressions tests (only logic synthesis and elaboration) hence why these tasks are put under `vtr_reg_nightly_test2` so that entirety of the flow gets tested by the CI. 
