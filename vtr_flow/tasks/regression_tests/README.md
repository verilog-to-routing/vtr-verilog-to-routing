# Verilog of Routing Regression Test Infrastructure

There are currently four levels of regression testing as described below. Refer
to [`regression_tests.ods`](./regression_tests.ods) for details on architecture
+ benchmarks run by each test.

## LEVEL ONE - Basic VTR Regression - `vtr_reg_basic`

 * MANDATORY - Must be run by ALL developers before committing.
 * Estimated Runtime: ~2-5 minutes

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
```
./run_reg_test.py vtr_reg_basic
```
To create golden results, use:
```
./run_reg_test.py -create_golden vtr_reg_basic
```

Execute with:
```
<scripts_path>/run_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt
```

Parse results with:
```
<scripts_path>/parse_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt
```

Check results with:
```
<scripts_path>/parse_vtr_task.py -check_golden -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt
```

Create golden results with:
```
<scripts_path>/parse_vtr_task.py -create_golden -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt
```

## LEVEL TWO - Strong VTR Regression - `vtr_reg_strong`

 * OPTIONAL - Can be run by developers before committing.
 * Estimated Runtime: ~5-10 minutes

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
```
./run_reg_test.py vtr_reg_strong
./run_reg_test.py vtr_reg_valgrind_small
```
To create golden results, use:
```
./run_reg_test.py -create_golden vtr_reg_strong
```

Execute with:
```
<scripts_path>/run_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt
```

Parse results with:
```
<scripts_path>/parse_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt
```

Check results with:
```
<scripts_path>/parse_vtr_task.py -check_golden -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt
```

Create golden results with:
```
<scripts_path>/parse_vtr_task.py -create_golden -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt
```

## LEVEL THREE  - Nightly VTR Regression - `vtr_reg_nightly_test#, #:1-3` 

 * To be run by automated build system every night and on every pull request.
 * To keep the wall-clock time of this suite under ~6 hours using -j8, it is divided into multiple sub-suites, and each of them are submitted as different jobs to different kokoro machines. 
 * Estimated runtime: 30-35 hours
 
DO-IT-ALL COMMAND - This command will execute, parse, and check results.
```
./run_reg_test.py vtr_reg_nightly_test1
./run_reg_test.py vtr_reg_nightly_test2
./run_reg_test.py vtr_reg_nightly_test3
./run_reg_test.py vtr_reg_valgrind
```
**The below commands concern a single sub-suite (# is the sub-suite number). They have to be repeated for all sub-suites to cover all tests under Nightly VTR Regression**

To create golden results, use:
```
./run_reg_test.py -create_golden vtr_reg_nightly_test#
```

Execute  a sub-suite with:
```
<scripts_path>/run_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_nightly_test#/task_list.txt
```

Parse results with:
```
<scripts_path>/parse_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_nightly_test#/task_list.txt
```

Check results with:
```
<scripts_path>/parse_vtr_task.py -check_golden -l <tasks_path>/regression_tests/vtr_reg_nightly_test#/task_list.txt
```

Create golden results with:
```
<scripts_path>/parse_vtr_task.py -create_golden -l <tasks_path>/regression_tests/vtr_reg_nightly_test#/task_list.txt
```


## LEVEL FOUR - Weekly VTR Regression - `vtr_reg_weekly`

 * To be run by automated build system every weekend.
 * Estimated Runtime: 40+ hours

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
```
./run_reg_test.py vtr_reg_weekly
```

To create golden results, use:
```
./run_reg_test.py -create_golden vtr_reg_weekly
```

Execute with:
```
<scripts_path>/run_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt
```

Parse results with:
```
<scripts_path>/parse_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt
```

Check results with:
```
<scripts_path>/parse_vtr_task.py -check_golden -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt
```

Create golden results with:
```
<scripts_path>/parse_vtr_task.py -create_golden -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt
```
## Parallesim Startegy for vtr_reg_nightly:
### Current Sub-suites:

  * The nightly regression suite is broken up into multiple sub-suites to minimize the wall-clock when ran by CI using Kokoro machines.
  * The lower bound for the run-time of the nightly regression tests is the longest vtr_flow run in all suites (currently this flow is in vtr_reg_nightly_test2/vtr_reg_qor)
  * To minimize wall-clock time, tasks which have the three longest flow runs are put in seperate directories and other tasks are added to keep the
    run-time for the sub-suite under ~5 hours using -j8 option on the Kokoro machines.
  * The longest tasks are put at the bottom of task_list.txt to get started first (the files are read in backwards in `run_reg_test.py`
  * If tasks that do not have long flow runs are to be added, it is best that they are added under vtr_reg_nightly_test1 as this suite has the smallest run-time
    of all suites (~2 hours using -j8).

### Adding Sub-suites:

  * If tasks with long flows that exceed ~3 hours are to be added, it is best to seperate them from the other suites and put it in a seperate test
    at the bottom of the task list.
  * Adding additional suites to vtr_reg_nightly comprises three steps:
    - a config file (.cfg) has to be added to the config list for Kokoro machines located at `$VTR_ROOT/.github/kokoro/presubmit`. The new config should be indentical to the other config files for nightly tests (e.g. `VTR_ROOT/.github/kokoro/presubmit/nightly_test1.cfg`) , with the only difference being the value for VTR_TEST (i.e. the value should be changed to the directory name for the new suite say vtr_reg_nightly_testX).  
    - `$VTR_ROOT/.github/kokoro/steps/vtr-test.sh` need to be updated to recongize the new suite and zip up the output files (we don't want the machine to run of disk space ...). e.g. if the suite to be added is `vtr_reg_nightly_testX`, the following line should be added to the script in its appropriate place:
    ```
    find vtr_flow/tasks/regression_tests/vtr_reg_nightly_testX/ -type f -print0 | xargs -0 -P $(nproc) gzip
    ```

    - The previous addition of .cfg file sets up the configs from our side of the repo. The new configs need to be submitted on Google's side as well for the Kokoro machines to run the new CI tests. The best person to contact to do this setup is Tim Ansell (@mithro on Github). 
