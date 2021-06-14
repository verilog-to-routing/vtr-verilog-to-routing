# Parallesim Startegy for vtr_reg_nightly: 
## Current Sub-suites: 

  * The nightly regression suite is broken up into multiple sub-suites to minimize the wall-clock when ran by CI using Kokoro machines. 
  * The lower bound for the run-time of the nightly regression tests is the longest vtr_flow run in all suites (currently this flow is in vtr_reg_nightly_test2/vtr_reg_qor) 
  * To minimize wall-clock time, tasks which have the three longest flow runs are put in seperate directories and other tasks are added to keep the 
    run-time for the sub-suite under ~4 hours using -j8.
  * The longest tasks are put at the bottom of task_list.txt to get started first (the files are read in backwards in `run_reg_test.py`
  * If tasks that do not have long flow runs are to be added, it is best that they are added under vtr_reg_nightly_test1 as this suite has the smallest run-time 
    of all suites (~2 hours using -j8). 
  * If tasks with long flows that exceed ~1.5-2 hours are to be added, it is best to seperate them from the other suites and put it in a seperate test
    at the bottom of the task list. 

## Adding Sub-suites: 
  * To add additional suites, a config file (.cfg) has to be added to [the config list for Kokoro machines](https://github.com/verilog-to-routing/vtr-verilog-to-routing/tree/master/.github/kokoro/presubmit). The new config should be indentical to the other config file for nightly tests, with the only difference being the value for VTR_TEST (i.e. the value should be changed to the directory name for the new suite). 

  
  
