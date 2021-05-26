# Verify Script

The verify_odin.sh script is designed for generating regression test results. 

./verify_odin.sh [args]

## Arguments

*The tool requires a task to run hence `-t <task directory>` must be passed in

| arg   |  equivalent form          | Following argument                  |      Description                                                                               |
|-------|---------------------------|-------------------------------------|------------------------------------------------------------------------------------------------|
| `-t`  |  path to directory containing a test |The test types are a task.conf,task_list.conf or a predefined test                             |
|  `-j` | `-- nb_of_process`        | # of processors requested           |  This allows the user to choose how many processor are used in the computer, the default is 1  |
|  `-d` | `--output_dir`            | path to directory                   |  This allows the user to change the directory of the run output                                |
|  `-C` | `--config`                | path to configuration file          |  This directs the compiler to a configuration file to append the configuration file of the test|
|  `-t` | `--test`                  | path to directory containing a test |  The test types are a task.conf,task_list.conf or a predefined test                            |
|  `-h` | `--help`                  | N/A                                 |  Prints all possible commands and directives                                                   |
|  `-g` | `--generate_bench`        | N/A                                 |  Generates input and output vectors for test                                                    |
|  `-o` | `--generate_output`       | N/A                                 |  Generates output vector for test given its input vector                                       |
|  `-b` | `--build_config`          | path to directory                   |  Generates a configuration file for a given directory                                          |
|  `-c` | `--clean`                 | N/A                                 |  Clean temporary directory                                                                     |
|  `-f` | `--force_simulate`        | N/A                                 |  Force the simulation to be executed regardless of the configuration file                      |
|  N/A  | `--override`              | N/A                                 |  if a configuration file is passed in, override arguments rather than append                   |
|  N/A  | `--dry_run`               | N/A                                 |  performs a dry run to check the validity of the task and flow                                 |
|  N/A  | `--randomize`             | N/A                                 |  performs a dry run randomly to check the validity of the task and flow                        |
|  N/A  | `--regenerate_expectation`| N/A                                 |  regenerates expectation and overrides the expected value only if there's a mismatch           |
|  N/A  | `--generate_expectation`  | N/A                                 |  generate the expectation and overrides the expectation file                                   |

## Examples

The following examples are being performed in the ODIN_II directory:

### Generating Results for a New Task

To generate new results, `synthesis_parse_file` and `simulation_parse_file` must be specified
in task.conf file.

The following commands will generate the results of a new regression test using N processors:

```bash
make sanitize
```

```bash
./verify_odin.sh --generate_expectation -j N -t <regression_test/benchmark/task/<task_name>
```

A synthesis_result.json and a simulation_result.json will be generated in the task's folder.
The simulation results for each benchmark are only generated if they synthesize correctly (no exit error), thus if none of the benchmarks synthesize there will be no simulation_result.json generated.

### Regenerating Results for a Changed Test

The following commands will only generate the results of the changes.
If there are new benchmarks it will add to the results.
If there are deleted benchmarks or modified benchmarks the results will be updated accordingly.

```bash
make sanitize
```

```bash
./verify_odin.sh --regenerate_expectation -t <regression_test/benchmark/task/<task_name>
```

### Generating Results for a Suite

The following commands generate the results for all the tasks called upon in a suite.

```bash
make sanitize
```

> **NOTE**
>
> If the suite calls upon the large test **DO NOT** run `make sanitize`.
> Instead run `make build`.

```bash
./verify_odin.sh --generate_expectation -t <regression_test/benchmark/suite/<suite_name>
```

### Checking the configuration file

The following commands will check if a configuration file is being read properly.

```bash
make build
```

```bash
./verify_odin.sh --dry_run -t <regression_test/benchmark/<path/to/config_file/difrectory>
```

### Running a subset of tests in a suite

The following commands will run only the tests matching `<test regex>`:

```bash
./verify_odin.sh -t <regression_test/benchmark/suite/<suite_name> <test regex>
```

You may specify as many test regular expressions as desired and the script will run any test that matches at least one regex

> **NOTE**
>
> This uses grep's extended regular expression syntax for matching test names.  
> Test names matched are of the form <suite_name>/<test_name>/<architecture>