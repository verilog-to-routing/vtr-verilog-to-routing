# VTR Testing Guide

**Unit tests** are best for isolated APIs and data structures that can be exercised without a full VPR run (e.g., a new container or algorithm). **Regression tests** cover end-to-end behavior; because VPR's data structures require large input files to construct, most CAD algorithm changes are validated through regression tests rather than unit tests.

## Running the VTR Flow

Activate the Python virtual environment before running any scripts below:

```shell
source .venv/bin/activate
```

```shell
# Run a regression task
cd vtr_flow/tasks
../scripts/run_vtr_task.py regression_tests/vtr_reg_strong/strong_timing

# Parse QoR results
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_timing

# Compare QoR across two runs
./vtr_flow/scripts/qor_compare.py parse_results_baseline.txt parse_results_new.txt -o comparison.xlsx
```

## Unit Tests

Unit tests use Catch2. Run them with:

```shell
# Build and run all unit tests
make && make test

# Run a specific unit test binary directly
cd build/vpr && ./test_vpr --list-tests
cd build/vpr && ./test_vpr connection_router binary_heap
```

Source-to-binary mapping:
- `vpr/test/` → `build/vpr/test_vpr`
- `libs/libarchfpga/test/` → `build/libs/libarchfpga/test_archfpga`
- `libs/libvtrutil/test/` → `build/libs/libvtrutil/test_vtrutil`
- `utils/fasm/test/` → `build/utils/fasm/test_fasm`

New unit tests go in the corresponding `test/` directory and are registered in the module's `CMakeLists.txt`.

## Adding a Regression Test

New features **must** include a regression test in `vtr_reg_strong` (small benchmarks, < 1 min runtime):

```shell
cd vtr_flow/tasks/regression_tests/vtr_reg_strong
mkdir -p strong_mytest/config
cp strong_timing/config/config.txt strong_mytest/config/.
# Edit the config: update the architecture file, circuit list, and VPR command-line flags for your test
# Then from vtr_flow/tasks/:
../scripts/run_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest -create_golden
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest -check_golden
# Add path to vtr_flow/tasks/regression_tests/vtr_reg_strong/task_list.txt
```

## Updating a Golden Result

After an intentional QoR change, regenerate the golden result for the affected test:

```shell
cd vtr_flow/tasks
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/<test_name> -create_golden
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/<test_name> -check_golden
```
