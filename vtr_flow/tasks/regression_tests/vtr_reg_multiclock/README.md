##ODIN-II Regression 

`task_list.txt` in this directory points to a set of tasks in the same directory. These tasks are referred to in  
[`light_suite`](https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/ODIN_II/regression_test/benchmark/suite/light_suite/task_list.conf) which is part of a series of ODIN-II regression tests. `light_suite` is currently being run by CI under the name`odin_reg_basic`. First, Odin specific tests are run then the task_list is run by `run_vtr_task.py`. Thus, the entirety of the flow is tested for `vtr_reg_multiclock` tasks by CI. 
