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

## LEVEL THREE  - Nightly VTR Regression - `vtr_reg_nightly`

 * To be run by automated build system every night.
 * Estimated Runtime: ~15-20 hours

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
```
./run_reg_test.py vtr_reg_nightly
./run_reg_test.py vtr_reg_valgrind
```

To create golden results, use:
```
./run_reg_test.py -create_golden vtr_reg_nightly
```

Execute with:
```
<scripts_path>/run_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt
```

Parse results with:
```
<scripts_path>/parse_vtr_task.py -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt
```

Check results with:
```
<scripts_path>/parse_vtr_task.py -check_golden -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt
```

Create golden results with:
```
<scripts_path>/parse_vtr_task.py -create_golden -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt
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
