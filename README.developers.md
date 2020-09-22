# Commit Procedures

For general guidance on contributing to VTR see [Submitting Code to VTR](CONTRIBUTING.md#submitting-code-to-vtr).

The actual machanics of submitting code are outlined below.

However they differ slightly depending on whether you are:
 * an **internal developer** (i.e. you have commit access to the main VTR repository at `github.com/verilog-to-routing/vtr-verilog-to-routing`) or, 
 * an (**external developer**) (i.e. no commit access).

The overall approach is similar, but we call out the differences below.

1. Setup a local repository on your development machine.

    a. **External Developers**

    * Create a 'fork' of the VTR repository.

        Usually this is done on GitHub, giving you a copy of the VTR repository (i.e. `github.com/<username>/vtr-verilog-to-routing`, where `<username>` is your GitHub username) to which you have commit rights.
        See [About forks](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/about-forks) in the GitHub documentation.

    * Clone your 'fork' onto your local machine.
    
        For example, `git clone git@github.com:<username>/vtr-verilog-to-routing.git`, where `<username>` is your GitHub username.

    b. **Internal Developers**

    * Clone the main VTR repository onto your local machine.
    
        For example, `git clone git@github.com:verilog-to-routing/vtr-verilog-to-routing.git`.

2. Move into the cloned repository.

    For example, `cd vtr-verilog-to-routing`.

3. Create a *branch*, based off of `master` to work on.

    For example, `git checkout -b my_awesome_branch master`, where `my_awesome_branch` is some helpful (and descriptive) name you give you're branch.
    *Please try to pick descriptive branch names!*

4. Make your changes to the VTR code base.

5. Test your changes to ensure they work as intended and have not broken other features.

    At the bare minimum it is recommended to run:
    ```
    make                                                #Rebuild the code
    ./run_reg_test.py vtr_reg_basic vtr_reg_strong      #Run tests
    ```

    See [Running Tests](#running-tests) for more details.

    Also note that additional [code formatting](#code-formatting) checks, and tests will be run when you open a Pull Request.

6. Commit your changes (i.e. `git add` followed by `git commit`).

    *Please try to use good commit messages!*

    See [Commit Messages and Structure](#commit-messages-and-structure) for details.

7. Push the changes to GitHub.

    For example, `git push origin my_awesome_branch`.

    a. **External Developers**

    Your code changes will now exist in your branch (e.g. `my_awesome_branch`) within your fork (e.g. `github.com/<username>/vtr-verilog-to-routing/tree/my_awesome_branch`, where `<username>` is your GitHub username)

    b. **Internal Developers**

    Your code changes will now exist in your branch (e.g. `my_awesome_branch`) within the main VTR repository (i.e. `github.com/verilog-to-routing/vtr-verilog-to-routing/tree/my_awesome_branch`)

8. Create a Pull Request (PR) to request your changes be merged into VTR.

    * Navitage to your branch on GitHub

        a. **External Developers**

        Navigate to your branch within your fork on GitHub (e.g. `https://github.com/<username/vtr-verilog-to-routing/tree/my_awesome_branch`, where `<username>` is your GitHub username, and `my_awesome_branch` is your branch name).

        b. **Internal Developers**

        Navigate to your branch on GitHub (e.g. `https://github.com/verilog-to-routing/vtr-verilog-to-routing/tree/my_awesome_branch`, where `my_awesome_branch` is your branch name).

    * Select the `New pull request` button.

        a. **External Developers**

        If prompted, select `verilog-to-routing/vtr-verilog-to-routing` as the base repository.

# Commit Messages and Structure

## Commit Messages

Commit messagaes are an important part of understanding the code base and it's history.
It is therefore *extremely* important to provide the following information in the commit message:

* What is being changed?
* Why is this change occurring?

The diff of changes included with the commit provides the details of what is actually changed, so only a high-level description of what is being done is needed.
However a code diff provides *no* insight into **why** the change is being made, so this extremely helpful context can only be encoded in the commit message.

The preferred convention in VTR is to structure commit messages as follows:
```
Header line: explain the commit in one line (use the imperative)

More detailed explanatory text. Explain the problem that this commit
is solving. Focus on why you are making this change as opposed to how
(the code explains that). Are there side effects or other unintuitive
consequences of this change? Here's the place to explain them.

If necessary. Wrap lines at some reasonable point (e.g. 74 characters,
or so) In some contexts, the header line is treated as the subject
of the commit and the rest of the text as the body. The blank line
separating the summary from the body is critical (unless you omit
the body entirely); various tools like `log`, `shortlog` and `rebase`
can get confused if you run the two together.

Further paragraphs come after blank lines.

 - Bullet points are okay, too

 - Typically a hyphen or asterisk is used for the bullet, preceded
   by a single space, with blank lines in between, but conventions
   vary here

You can also put issue tracker references at the bottom like this:

Fixes: #123
See also: #456, #789
```
(based off of [here](https://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html), and [here](https://github.com/torvalds/subsurface-for-dirk/blob/master/README.md#contributing)).

Commit messages do not always need to be long, so use your judgement.
More complex or involved changes with wider ranging implications likely deserve longer commit messages than fixing a simple typo.

It is often helpful to phrase the first line of a commit as an imperative/command written as if to tell the repository what to do (e.g. `Update netlist data structure comments`, `Add tests for feature XYZ`, `Fix bug which ...`).

To provide quick context, some VTR developers also tag the first line with the main part of the code base effected, some common ones include:
* `vpr:` for the VPR place and route tool (`vpr/`)
* `flow:` VTR flow architectures, scripts, tests, ... (`vtr_flow/`)
* `archfpga:` for FPGA architecture library (`libs/libarchfpga`)
* `vtrutil:` for common VTR utilities (`libs/libvtrutil`)
* `doc:` Documentation (`doc/`, `*.md`, ...)
* `infra:` Infrastructure (CI, `.github/`, ...)


## Commit Structure
Generally, you should strive to keep commits atomic (i.e. they do one logical change to the code).
This often means keeping commits small and focused in what they change.
Of course, a large number of miniscule commits is also unhelpful (overwhelming and difficult to see the structure), and sometimes things can only be done in large changes -- so use your judgement.
A reasonable rule of thumb is to try and ensure VTR will still compile after each commit.

For those familiar with history re-writing features in git (e.g. rebase) you can sometimes use these to clean-up your commit history after the fact.
However these should only be done on private branches, and never directly on `master`.

# Code Formatting

Some parts of the VTR code base (e.g. VPR, libarchfpga, libvtrutil) have C/C++ code formatting requirements which are checked automatically by regression tests.
If your code changes are not compliant with the formatting, you can run:
```shell
make format
```
from the root of the VTR source tree.
This will automatically reformat your code to be compliant with formatting requirements (this requires the `clang-format` tool to be available on your system).

Python code must also be compliant with the formatting.  To format Python code, you can run:
```shell
make format-py
```
from the root of the VTR source tree (this requires the `black` tool to be available on your system).

## Large Scale Reformatting

For large scale reformatting (should only be performed by VTR maintainers) the script `dev/autoformat.py` can be used to reformat the C/C++ code and commit it as 'VTR Robot', which  keeps the revision history clearer and records metadata about reformatting commits (which allows `git hyper-blame` to skip such commits).  The `--python` option can be used for large scale formatting of Python code.

## Python Linting

Python files are automatically checked using `pylint` to ensure they follow established Python conventions.  You can check an individual Python file by running `pylint <your_python_file>`, or check the entire repository by running `./dev/pylint_check.py`.

# Running Tests

VTR has a variety of tests which are used to check for correctness, performance and Quality of Result (QoR).

There are 4 main regression tests:

* `vtr_reg_basic`: ~1 minute serial

    **Goal:** Fast functionality check

    **Feature Coverage:** Low

    **Benchmarks:** A few small and simple circuits

    **Architectures:** A few simple architectures

    This regression test is *not* suitable for evaluating QoR or performance.
    It's primary purpose is to make sure the various tools do not crash/fail in the basic VTR flow.

    QoR checks in this regression test are primarily 'canary' checks to catch gross degradations in QoR.
    Occasionally, code changes can cause QoR failures (e.g. due to CAD noise -- particularly on small benchmarks); usually such failures are not a concern if the QoR differences are small.

* `vtr_reg_strong`: ~20 minutes serial, ~15 minutes with `-j4`

    **Goal:** Broad functionality check

    **Feature Coverage:** High

    **Benchmarks:** A few small circuits, with some special benchmarks to exercise specific features

    **Architectures:** A variety of architectures, including special architectures to exercise specific features

    This regression test is *not* suitable for evaluating QoR or performance.
    It's primary purpose is try and achieve high functionality coverage.

    QoR checks in this regression test are primarily 'canary' checks to catch gross degradations in QoR.
    Occasionally, changes can cause QoR failures (e.g. due to CAD noise -- particularly on small benchmarks); usually such failures are not a concern if the QoR differences are small.

* `vtr_reg_nightly`: ~6 hours with `-j3`

    **Goal:** Basic QoR and Performance evaluation.

    **Feature Coverage:** Medium

    **Benchmarks:** Small-medium size, diverse. Includes:

    * MCNC20 benchmarks
    * VTR benchmarks
    * Titan 'other' benchmarks (smaller than Titan23)

    **Architectures:** A wider variety of architectures

   QoR checks in this regression are aimed at evaluating quality and run-time of the VTR flow.
   As a result any QoR failures are a concern and should be investigated and understood.

* `vtr_reg_weekly`: ~42 hours with `-j4`

    **Goal:** Full QoR and Performance evaluation.

    **Feature Coverage:** Medium

    **Benchmarks:** Medium-Large size, diverse. Includes:

    * VTR benchmarks
    * Titan23 benchmarks

    **Architectures:** A wide variety of architectures

   QoR checks in this regression are aimed at evaluating quality and run-time of the VTR flow.
   As a result any QoR failures are a concern and should be investigated and understood.

These can be run with `run_reg_test.py`:
```shell
#From the VTR root directory
$ ./run_reg_test.py vtr_reg_basic
$ ./run_reg_test.py vtr_reg_strong
```

The *nightly* and *weekly* regressions require the Titan and ISPD benchmarks
which can be integrated into your VTR tree with:
```shell
make get_titan_benchmarks
make get_ispd_benchmarks
```
They can then be run using `run_reg_test.py`:
```shell
$ ./run_reg_test.py vtr_reg_nightly
$ ./run_reg_test.py vtr_reg_weekly
```

To speed-up things up, individual sub-tests can be run in parallel using the `-j` option:
```shell
#Run up to 4 tests in parallel
$ ./run_reg_test.py vtr_reg_strong -j4
```

You can also run multiple regression tests together:
```shell
#Run both the basic and strong regression, with up to 4 tests in parallel
$ ./run_reg_test.py vtr_reg_basic vtr_reg_strong -j4
```

## Odin Functionality Tests

Odin has its own set of tests to verify the correctness of its synthesis results:

* `odin_reg_micro`: ~2 minutes serial
* `odin_reg_full`: ~6 minutes serial

These can be run with:
```shell
#From the VTR root directory
$ ./run_reg_test.py odin_reg_micro
$ ./run_reg_test.py odin_reg_full
```
and should be used when making changes to Odin.

## Unit Tests

VTR also has a limited set of unit tests, which can be run with:
```shell
#From the VTR root directory
$ make && make test
```

## Running tests on Pull Requests (PRs) via Kokoro

Because of the long runtime for nightly and weekly tests, a Kokoro job can be
used to run these tests once a Pull Request (PR) has been made at
https://github.com/verilog-to-routing/vtr-verilog-to-routing.

Any pull request made by a contributor of the verilog-to-routing GitHub project
on https://github.com/verilog-to-routing/ will get a set of jobs immediately.
Non-contributors can request a contributor on the project add a label
"kokoro:force-run" to the PR. Kokoro will then detect the tag, remove the tag,
and then and issue jobs for that PR.  If the tag remains after being added,
there may not be an available Kokoro runner, so wait.

### Re-running tests on Kokoro

If a job fails due to an intermittent failure or a re-run is desired, a
contributor can add the label "kokoro:force-run" to re-issue jobs for that PR.

### Checking results from Kokoro tests

Currently there is not a way for an in-flight job to be monitored.

Once a job has been completed, you can follow the "Details" link that appears on the PR status. 
The Kokoro page will show the job's stdout in the 'Target Log' tab (once the job has completed).
The full log can be downloading by clicking the 'Download Full Log' button, or from the 'Artifacts' tab.

### Downloading logs from Google Cloud Storage (GCS)

After a Kokoro run is complete a number of useful log files (e.g. for each VPR invocation) are stored to Google Cloud Storage (GCS).

The top level directory containing all VTR Kokoro runs is:

    https://console.cloud.google.com/storage/browser/vtr-verilog-to-routing/artifacts/prod/foss-fpga-tools/verilog-to-routing/upstream/

PR jobs are under the `presubmit` directory, and continuous jobs (which run on the master branch) are under the `continuous` directory.

Each Kokoro run has a unique build number, which can be found in the log file (available via the Kokoro run webpage).
For example, if the log file contains:
```
export KOKORO_BUILD_NUMBER="450"
```
then the Kokoro build number is `450`.

If build 450 corresponded to a PR (`presubmit`) build of the `nightly` regression tests, the resulting output files would be available at:

    https://console.cloud.google.com/storage/browser/vtr-verilog-to-routing/artifacts/prod/foss-fpga-tools/verilog-to-routing/upstream/presubmit/nightly/450/

where `presubmit/nightly/450/` (the type, test name and build number) have been appended to the base URL.
Navigating to that URL will allow you to browse and download the collected log files.

To download all the files from that Kokoro run, replace `https://console.cloud.google.com/storage/browser/` in the URL with `gs://` and invoke the [gsutil](https://cloud.google.com/storage/docs/gsutil) command (and it's `cp -R` sub-command), like so:

```
gsutil -m cp -R gs://vtr-verilog-to-routing/artifacts/prod/foss-fpga-tools/verilog-to-routing/upstream/presubmit/nightly/450 .
```

This will download all of the logs to the current directory for inspection.

#### Kokoro runner details

Kokoro runners are a standard
[`n1-highmem-16`](https://cloud.google.com/compute/docs/machine-types#n1_high-memory_machine_types)
VM with a 4 TB `pd-standard` disk used to perform the build of VPR and run the
tests.

#### What to do if Kokoro jobs are not starting?

There are several reasons Kokoro jobs might not be starting.
Try adding the "kokoro:force-run" label if it is not already added, or remove
and add it if it already was added.

If adding the label has no affect, check GCS status, as a GCS disruption will
also disrupt Kokoro.

Another reason jobs may not start is if there is a large backlog of jobs
running, there may be no runners left to start.  In this case, someone with
Kokoro management rights may need to terminate stale jobs, or wait for job
timeouts.

# Debugging Failed Tests

If a test fails you probably want to look at the log files to determine the cause.

Lets assume we have a failure in `vtr_reg_basic`:

```shell
#In the VTR root directory
$ ./run_reg_test.py vtr_reg_strong
#Output trimmed...
regression_tests/vtr_reg_basic/basic_no_timing
-----------------------------------------
k4_N10_memSize16384_memData64/ch_intrinsics/common   failed: vpr
k4_N10_memSize16384_memData64/diffeq1/common         failed: vpr
#Output trimmed...
regression_tests/vtr_reg_basic/basic_no_timing...[Fail]
 k4_N10_memSize16384_memData64.xml/ch_intrinsics.v vpr_status: golden = success result = exited
#Output trimmed...
Error: 10 tests failed!
```

Here we can see that `vpr` failed, which caused subsequent QoR failures (`[Fail]`), and resulted in 10 total errors.

To see the log files we need to find the run directory.
We can see from the output that  the specific test which failed was `regression_tests/vtr_reg_basic/basic_no_timing`.
All the regression tests take place under `vtr_flow/tasks`, so the test directory is `vtr_flow/tasks/regression_tests/vtr_reg_basic/basic_no_timing`.
Lets move to that directory:
```shell
#From the VTR root directory
$ cd vtr_flow/tasks/regression_tests/vtr_reg_basic/basic_no_timing
$ ls
config  run001  run003
latest  run002  run004  run005
```

There we see there is a `config` directory (which defines the test), and a set of run-directories.
Each time a test is run it creates a new `runXXX` directory (where `XXX` is an incrementing number).
From the above we can tell that our last run was `run005` (the symbolic link `latest` also points to the most recent run directory).
From the output of `run_reg_test.py` we know that one of the failing architecture/circuit/parameters combinations was `k4_N10_memSize16384_memData64/ch_intrinsics/common`.
Each architecture/circuit/parameter combination is run in its own sub-folder.
Lets move to that directory:
```shell
$ cd run005/k4_N10_memSize16384_memData64/ch_intrinsics/common
$ ls
abc.out                     k4_N10_memSize16384_memData64.xml  qor_results.txt
ch_intrinsics.net           odin.out                           thread_1.out
ch_intrinsics.place         output.log                         vpr.out
ch_intrinsics.pre-vpr.blif  output.txt                         vpr_stdout.log
ch_intrinsics.route         parse_results.txt
```

Here we can see the individual log files produced by each tool (e.g. `vpr.out`), which we can use to guide our debugging.
We could also manually re-run the tools (e.g. with a debugger) using files in this directory.

# Evaluating Quality of Result (QoR) Changes
VTR uses highly tuned and optimized algorithms and data structures.
Changes which effect these can have significant impacts on the quality of VTR's design implementations (timing, area etc.) and VTR's run-time/memory usage.
Such changes need to be evaluated carefully before they are pushed/merged to ensure no quality degradation occurs.

If you are unsure of what level of QoR evaluation is necessary for your changes, please ask a VTR developer for guidance.

## General QoR Evaluation Principles
The goal of performing a QoR evaluation is to measure precisely the impact of a set of code/architecture/benchmark changes on both the quality of VTR's design implementation (i.e. the result of VTR's optimizations), and on tool run-time and memory usage.

This process is made more challenging by the fact that many of VTR's optimization algorithms are based on heuristics (some of which depend on randomization).
This means that VTR's implementation results are dependent upon:
 * The initial conditions (e.g. input architecture & netlist, random number generator seed), and
 * The precise optimization algorithms used.

The result is that a minor change to either of these can can make the measured QoR change.
This effect can be viewed as an intrinsic 'noise' or 'variance' to any QoR measurement for a particular architecture/benchmark/algorithm combination.

There are typically two key methods used to measure the 'true' QoR:

1. Averaging metrics across multiple architectures and benchmark circuits.

2. Averaging metrics multiple runs of the same architecture and benchmark, but using different random number generator seeds

    This is a further variance reduction technique, although it can be very CPU-time intensive.
    A typical example would be to sweep an entire benchmark set across 3 or 5 different seeds.

In practice any algorithm changes will likely cause improvements on some architecture/benchmark combinations, and degradations on others.
As a result we primarily focus on the *average* behaviour of a change to evaluate its impact.
However extreme outlier behaviour on particular circuits is also important, since it may indicate bugs or other unexpected behaviour.

### Key QoR Metrics

The following are key QoR metrics which should be used to evaluate the impact of changes in VTR.

Implementation Quality Metrics:

| Metric                      | Meaning                                                                  | Sensitivity |
|-----------------------------|--------------------------------------------------------------------------|-------------|
| num_pre_packed_blocks       | Number of primitive netlist blocks (after tech. mapping, before packing) | Low         |
| num_post_packed_blocks      | Number of Clustered Blocks (after packing)                               | Medium      |
| device_grid_tiles           | FPGA size in grid tiles                                                  | Low-Medium  |
| min_chan_width              | The minimum routable channel width                                       | Medium\*    |
| crit_path_routed_wirelength | The routed wirelength at the relaxed channel width                       | Medium      |
| critical_path_delay         | The critical path delay at the relaxed channel width                     | Medium-High |

\* By default, VPR attempts to find the minimum routable channel width; it then performs routing at a relaxed (e.g. 1.3x minimum) channel width. At minimum channel width routing congestion can distort the true timing/wirelength characteristics. Combined with the fact that most FPGA architectures are built with an abundance of routing, post-routing metrics are usually only evaluated at the relaxed channel width.

Run-time/Memory Usage Metrics:

| Metric                      | Meaning                                                                        | Sensitivity |
|-----------------------------|--------------------------------------------------------------------------------|-------------|
| vtr_flow_elapsed_time       | Wall-clock time to complete the VTR flow                                       | Low         |
| pack_time                   | Wall-clock time VPR spent during packing                                       | Low         |
| place_time                  | Wall-clock time VPR spent during placement                                     | Low         |
| min_chan_width_route_time   | Wall-clock time VPR spent during routing at the minimum routable channel width | High\*      |
| crit_path_route_time        | Wall-clock time VPR spent during routing at the relaxed channel width          | Low         |
| max_vpr_mem                 | Maximum memory used by VPR (in kilobytes)                                      | Low         |

\*  Note that the minimum channel width route time is chaotic and can be highly variable (e.g. 10x variation is not unusual). Minimum channel width routing performs a binary search to find the minimum channel width. Since route time is highly dependent on congestion, run-time is highly dependent on the precise channel widths searched (which may change due to perturbations).

In practice you will likely want to consider additional and more detailed metrics, particularly those directly related to the changes you are making.
For example, if your change related to hold-time optimization you would want to include hold-time related metrics such as `hold_TNS` (hold total negative slack) and `hold_WNS` (hold worst negative slack).
If your change related to packing, you would want to report additional packing-related metrics, such as the number of clusters formed by each block type (e.g. numbers of CLBs, RAMs, DSPs, IOs).

### Benchmark Selection

An important factor in performing any QoR evaluation is the benchmark set selected.
In order to draw reasonably general conclusions about the impact of a change we desire two characteristics of the benchmark set:

1. It includes a large number of benchmarks which are representative of the application domains of interest.

    This ensures we don't over-tune to a specific benchmark or application domain.

2. It should include benchmarks of large sizes.

    This ensures we can optimize and scale to large problem spaces.

In practice (for various reasons) satisfying both of these goals simultaneously is challenging.
The key goal here is to ensure the benchmark set is not unreasonably biased in some manner (e.g. benchmarks which are too small, benchmarks too skewed to a particular application domain).

### Fairly measuring tool run-time
Accurately and fairly measuring the run-time of computer programs is challenging in practice.
A variety of factors effect run-time including:

* Operating System
* System load (e.g. other programs running)
* Variance in hardware performance (e.g. different CPUs on different machines, CPU frequency scaling)

To make reasonably 'fair' run-time comparisons it is important to isolate the change as much as possible from other factors.
This involves keeping as much of the experimental environment identical as possible including:

1. Target benchmarks
2. Target architecture
3. Code base (e.g. VTR revision)
4. CAD parameters
5. Computer system (e.g. CPU model, CPU frequency/power scaling, OS version)
6. Compiler version

## Collecting QoR Measurements
The first step is to collect QoR metrics on your selected benchmark set.

You need at least two sets of QoR measurements:
1. The baseline QoR (i.e. unmodified VTR).
2. The modified QoR (i.e. VTR with your changes).

Note that it is important to generate both sets of QoR measurements on the same computing infrastructure to ensure a fair run-time comparison.

The following examples show how a single set of QoR measurements can be produced using the VTR flow infrastructure.

### Example: VTR Benchmarks QoR Measurement

The VTR benchmarks are a group of benchmark circuits distributed with the VTR project.
The are provided as synthesizable verilog and can be re-mapped to VTR supported architectures.
They consist mostly of small to medium sized circuits from a mix of application domains.
They are used primarily to evaluate the VTR's optimization quality in an architecture exploration/evaluation setting (e.g. determining minimum channel widths).

A typical approach to evaluating an algorithm change would be to run `vtr_reg_qor_chain` task from the nightly regression test:

```shell
#From the VTR root
$ cd vtr_flow/tasks

#Run the VTR benchmarks
$ ../scripts/run_vtr_task.py regression_tests/vtr_reg_nightly/vtr_reg_qor_chain

#Several hours later... they complete

#Parse the results
$ ../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_nightly/vtr_reg_qor_chain

#The run directory should now contain a summary parse_results.txt file
$ head -5 vtr_reg_nightly/vtr_reg_qor_chain/latest/parse_results.txt
arch                                  	circuit           	script_params	vpr_revision 	vpr_status	error	num_pre_packed_nets	num_pre_packed_blocks	num_post_packed_nets	num_post_packed_blocks	device_width	device_height	num_clb	num_io	num_outputs	num_memoriesnum_mult	placed_wirelength_est	placed_CPD_est	placed_setup_TNS_est	placed_setup_WNS_est	min_chan_width	routed_wirelength	min_chan_width_route_success_iteration	crit_path_routed_wirelength	crit_path_route_success_iteration	critical_path_delay	setup_TNS	setup_WNS	hold_TNS	hold_WNS	logic_block_area_total	logic_block_area_used	min_chan_width_routing_area_total	min_chan_width_routing_area_per_tile	crit_path_routing_area_total	crit_path_routing_area_per_tile	odin_synth_time	abc_synth_time	abc_cec_time	abc_sec_time	ace_time	pack_time	place_time	min_chan_width_route_time	crit_path_route_time	vtr_flow_elapsed_time	max_vpr_mem	max_odin_mem	max_abc_mem
k6_frac_N10_frac_chain_mem32K_40nm.xml	bgm.v             	common       	9f591f6-dirty	success   	     	26431              	24575                	14738               	2258                  	53          	53           	1958   	257   	32         	0           11      	871090               	18.5121       	-13652.6            	-18.5121            	84            	328781           	32                                    	297718                     	18                               	20.4406            	-15027.8 	-20.4406 	0       	0       	1.70873e+08           	1.09883e+08          	1.63166e+07                      	5595.54                             	2.07456e+07                 	7114.41                        	11.16          	1.03          	-1          	-1          	-1      	141.53   	108.26    	142.42                   	15.63               	652.17               	1329712    	528868      	146796
k6_frac_N10_frac_chain_mem32K_40nm.xml	blob_merge.v      	common       	9f591f6-dirty	success   	     	14163              	11407                	3445                	700                   	30          	30           	564    	36    	100        	0           0       	113369               	13.4111       	-2338.12            	-13.4111            	64            	80075            	18                                    	75615                      	23                               	15.3479            	-2659.17 	-15.3479 	0       	0       	4.8774e+07            	3.03962e+07          	3.87092e+06                      	4301.02                             	4.83441e+06                 	5371.56                        	0.46           	0.17          	-1          	-1          	-1      	67.89    	11.30     	47.60                    	3.48                	198.58               	307756     	48148       	58104
k6_frac_N10_frac_chain_mem32K_40nm.xml	boundtop.v        	common       	9f591f6-dirty	success   	     	1071               	1141                 	595                 	389                   	13          	13           	55     	142   	192        	0           0       	5360                 	3.2524        	-466.039            	-3.2524             	34            	4534             	15                                    	3767                       	12                               	3.96224            	-559.389 	-3.96224 	0       	0       	6.63067e+06           	2.96417e+06          	353000.                          	2088.76                             	434699.                     	2572.18                        	0.29           	0.11          	-1          	-1          	-1      	2.55     	0.82      	2.10                     	0.15                	7.24                 	87552      	38484       	37384
k6_frac_N10_frac_chain_mem32K_40nm.xml	ch_intrinsics.v   	common       	9f591f6-dirty	success   	     	363                	493                  	270                 	247                   	10          	10           	17     	99    	130        	1           0       	1792                 	1.86527       	-194.602            	-1.86527            	46            	1562             	13                                    	1438                       	20                               	2.4542             	-226.033 	-2.4542  	0       	0       	3.92691e+06           	1.4642e+06           	259806.                          	2598.06                             	333135.                     	3331.35                        	0.03           	0.01          	-1          	-1          	-1      	0.46     	0.31      	0.94                     	0.09                	2.59                 	62684      	8672        	32940
```

### Example: Titan Benchmarks QoR Measurements

The Titan benchmarks are a group of large benchmark circuits from a wide range of applications, which are compatible with the VTR project.
The are typically used as post-technology mapped netlists which have been pre-synthesized with Quartus.
They are substantially larger and more realistic than the VTR benchmarks, but can only target specifically compatible architectures.
They are used primarily to evaluate the optimization quality and scalability of VTR's CAD algorithms while targeting a fixed architecture (e.g. at a fixed channel width).

A typical approach to evaluating an algorithm change would be to run `vtr_reg_titan` task from the weekly regression test:

```shell
#From the VTR root

#Download and integrate the Titan benchmarks into the VTR source tree
$ make get_titan_benchmarks

#Move to the task directory
$ cd vtr_flow/tasks

#Run the VTR benchmarks
$ ../scripts/run_vtr_task.py regression_tests/vtr_reg_weekly/vtr_reg_titan

#Several days later... they complete

#Parse the results
$ ../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_weekly/vtr_reg_titan

#The run directory should now contain a summary parse_results.txt file
$ head -5 vtr_reg_nightly/vtr_reg_qor_chain/latest/parse_results.txt
arch                     	circuit                                 	vpr_revision	vpr_status	error	num_pre_packed_nets	num_pre_packed_blocks	num_post_packed_nets	num_post_packed_blocks	device_width	device_height	num_clb	num_io	num_outputs	num_memoriesnum_mult	placed_wirelength_est	placed_CPD_est	placed_setup_TNS_est	placed_setup_WNS_est	routed_wirelength	crit_path_route_success_iteration	logic_block_area_total	logic_block_area_used	routing_area_total	routing_area_per_tile	critical_path_delay	setup_TNS   setup_WNS	hold_TNS	hold_WNS	pack_time	place_time	crit_path_route_time	max_vpr_mem	max_odin_mem	max_abc_mem
stratixiv_arch.timing.xml	neuron_stratixiv_arch_timing.blif       	0208312     	success   	     	119888             	86875                	51408               	3370                  	128         	95           	-1     	42    	35         	-1          -1      	3985635              	8.70971       	-234032             	-8.70971            	1086419          	20                               	0                     	0                    	2.66512e+08       	21917.1              	9.64877            	-262034     -9.64877 	0       	0       	127.92   	218.48    	259.96              	5133800    	-1          	-1
stratixiv_arch.timing.xml	sparcT1_core_stratixiv_arch_timing.blif 	0208312     	success   	     	92813              	91974                	54564               	4170                  	77          	57           	-1     	173   	137        	-1          -1      	3213593              	7.87734       	-534295             	-7.87734            	1527941          	43                               	0                     	0                    	9.64428e+07       	21973.8              	9.06977            	-625483     -9.06977 	0       	0       	327.38   	338.65    	364.46              	3690032    	-1          	-1
stratixiv_arch.timing.xml	stereo_vision_stratixiv_arch_timing.blif	0208312     	success   	     	127088             	94088                	62912               	3776                  	128         	95           	-1     	326   	681        	-1          -1      	4875541              	8.77339       	-166097             	-8.77339            	998408           	16                               	0                     	0                    	2.66512e+08       	21917.1              	9.36528            	-187552     -9.36528 	0       	0       	110.03   	214.16    	189.83              	5048580    	-1          	-1
stratixiv_arch.timing.xml	cholesky_mc_stratixiv_arch_timing.blif  	0208312     	success   	     	140214             	108592               	67410               	5444                  	121         	90           	-1     	111   	151        	-1          -1      	5221059              	8.16972       	-454610             	-8.16972            	1518597          	15                               	0                     	0                    	2.38657e+08       	21915.3              	9.34704            	-531231     -9.34704 	0       	0       	211.12   	364.32    	490.24              	6356252    	-1          	-1
```

## Comparing QoR Measurements
Once you have two (or more) sets of QoR measurements they now need to be compared.

A general method is as follows:
1. Normalize all metrics to the values in the baseline measurements (this makes the relative changes easy to evaluate)
2. Produce tables for each set of QoR measurements showing the per-benchmark relative values for each metric
3. Calculate the GEOMEAN over all benchmarks for each normalized metric
4. Produce a summary table showing the Metric Geomeans for each set of QoR measurements

### QoR Comparison Gotchas
There are a variety of 'gotchas' you need to avoid to ensure fair comparisons:
* GEOMEAN's must be over the same set of benchmarks .
    A common issue is that a benchmark failed to complete for some reason, and it's metric values are missing

* Run-times need to be collected on the same compute infrastructure at the same system load (ideally unloaded).

### Example QoR Comparison
Suppose we've make a change to VTR, and we now want to evaluate the change.
As described above we produce QoR measurements for both the VTR baseline, and our modified version.

We then have the following (hypothetical) QoR Metrics.

**Baseline QoR Metrics:**

| arch                                   | circuit            | num_pre_packed_blocks | num_post_packed_blocks | device_grid_tiles | min_chan_width | crit_path_routed_wirelength | critical_path_delay | vtr_flow_elapsed_time | pack_time | place_time | min_chan_width_route_time | crit_path_route_time | max_vpr_mem |
|----------------------------------------|--------------------|-----------------------|------------------------|-------------------|----------------|-----------------------------|---------------------|-----------------------|-----------|------------|---------------------------|----------------------|-------------|
| k6_frac_N10_frac_chain_mem32K_40nm.xml | bgm.v              | 24575                 | 2258                   | 2809              | 84             | 297718                      | 20.4406             | 652.17                | 141.53    | 108.26     | 142.42                    | 15.63                | 1329712     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | blob_merge.v       | 11407                 | 700                    | 900               | 64             | 75615                       | 15.3479             | 198.58                | 67.89     | 11.3       | 47.6                      | 3.48                 | 307756      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | boundtop.v         | 1141                  | 389                    | 169               | 34             | 3767                        | 3.96224             | 7.24                  | 2.55      | 0.82       | 2.1                       | 0.15                 | 87552       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | ch_intrinsics.v    | 493                   | 247                    | 100               | 46             | 1438                        | 2.4542              | 2.59                  | 0.46      | 0.31       | 0.94                      | 0.09                 | 62684       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | diffeq1.v          | 886                   | 313                    | 256               | 60             | 9624                        | 17.9648             | 15.59                 | 2.45      | 1.36       | 9.93                      | 0.93                 | 86524       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | diffeq2.v          | 599                   | 201                    | 256               | 52             | 8928                        | 13.7083             | 13.14                 | 1.41      | 0.87       | 9.14                      | 0.94                 | 85760       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | LU8PEEng.v         | 31396                 | 2286                   | 2916              | 100            | 348085                      | 79.4512             | 1514.51               | 175.67    | 153.01     | 1009.08                   | 45.47                | 1410872     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | LU32PEEng.v        | 101542                | 7251                   | 9216              | 158            | 1554942                     | 80.062              | 28051.68              | 625.03    | 930.58     | 25050.73                  | 251.87               | 4647936     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mcml.v             | 165809                | 6767                   | 8649              | 128            | 1311825                     | 51.1905             | 9088.1                | 524.8     | 742.85     | 4001.03                   | 127.42               | 4999124     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkDelayWorker32B.v | 4145                  | 1327                   | 2500              | 38             | 30086                       | 8.39902             | 65.54                 | 7.73      | 15.39      | 26.19                     | 3.23                 | 804720      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkPktMerge.v       | 1160                  | 516                    | 784               | 44             | 13370                       | 4.4408              | 21.75                 | 2.45      | 2.14       | 13.95                     | 1.96                 | 122872      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkSMAdapter4B.v    | 2852                  | 548                    | 400               | 48             | 19274                       | 5.26765             | 47.64                 | 16.22     | 4.16       | 19.95                     | 1.14                 | 116012      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | or1200.v           | 4530                  | 1321                   | 729               | 62             | 51633                       | 9.67406             | 105.62                | 33.37     | 12.93      | 44.95                     | 3.33                 | 219376      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | raygentop.v        | 2934                  | 710                    | 361               | 58             | 22045                       | 5.14713             | 39.72                 | 9.54      | 4.06       | 19.8                      | 2.34                 | 126056      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | sha.v              | 3024                  | 236                    | 289               | 62             | 16653                       | 10.0144             | 390.89                | 11.47     | 2.7        | 6.18                      | 0.75                 | 117612      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision0.v    | 21801                 | 1122                   | 1156              | 58             | 64935                       | 3.63177             | 82.74                 | 20.45     | 15.49      | 24.5                      | 2.6                  | 411884      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision1.v    | 19538                 | 1096                   | 1600              | 100            | 143517                      | 5.61925             | 272.41                | 26.99     | 18.15      | 149.46                    | 15.49                | 676844      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision2.v    | 42078                 | 2534                   | 7396              | 134            | 650583                      | 15.3151             | 3664.98               | 66.72     | 119.26     | 3388.7                    | 62.6                 | 3114880     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision3.v    | 324                   | 55                     | 49                | 30             | 768                         | 2.66429             | 2.25                  | 0.75      | 0.2        | 0.57                      | 0.05                 | 61148       |

**Modified QoR Metrics:**

| arch                                   | circuit            | num_pre_packed_blocks | num_post_packed_blocks | device_grid_tiles | min_chan_width | crit_path_routed_wirelength | critical_path_delay | vtr_flow_elapsed_time | pack_time | place_time | min_chan_width_route_time | crit_path_route_time | max_vpr_mem |
|----------------------------------------|--------------------|-----------------------|------------------------|-------------------|----------------|-----------------------------|---------------------|-----------------------|-----------|------------|---------------------------|----------------------|-------------|
| k6_frac_N10_frac_chain_mem32K_40nm.xml | bgm.v              | 24575                 | 2193                   | 2809              | 82             | 303891                      | 20.414              | 642.01                | 70.09     | 113.58     | 198.09                    | 16.27                | 1222072     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | blob_merge.v       | 11407                 | 684                    | 900               | 72             | 77261                       | 14.6676             | 178.16                | 34.31     | 13.38      | 57.89                     | 3.35                 | 281468      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | boundtop.v         | 1141                  | 369                    | 169               | 40             | 3465                        | 3.5255              | 4.48                  | 1.13      | 0.7        | 0.9                       | 0.17                 | 82912       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | ch_intrinsics.v    | 493                   | 241                    | 100               | 54             | 1424                        | 2.50601             | 1.75                  | 0.19      | 0.27       | 0.43                      | 0.09                 | 60796       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | diffeq1.v          | 886                   | 293                    | 256               | 50             | 9972                        | 17.3124             | 15.24                 | 0.69      | 0.97       | 11.27                     | 1.44                 | 72204       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | diffeq2.v          | 599                   | 187                    | 256               | 50             | 7621                        | 13.1714             | 14.14                 | 0.63      | 1.04       | 10.93                     | 0.78                 | 68900       |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | LU8PEEng.v         | 31396                 | 2236                   | 2916              | 98             | 349074                      | 77.8611             | 1269.26               | 88.44     | 153.25     | 843.31                    | 49.13                | 1319276     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | LU32PEEng.v        | 101542                | 6933                   | 9216              | 176            | 1700697                     | 80.1368             | 28290.01              | 306.21    | 897.95     | 25668.4                   | 278.74               | 4224048     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mcml.v             | 165809                | 6435                   | 8649              | 124            | 1240060                     | 45.6693             | 9384.4                | 296.99    | 686.27     | 4782.43                   | 99.4                 | 4370788     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkDelayWorker32B.v | 4145                  | 1207                   | 2500              | 36             | 33354                       | 8.3986              | 53.94                 | 3.85      | 14.75      | 19.53                     | 2.95                 | 785316      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkPktMerge.v       | 1160                  | 494                    | 784               | 36             | 13881                       | 4.57189             | 20.75                 | 0.82      | 1.97       | 15.01                     | 1.88                 | 117636      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkSMAdapter4B.v    | 2852                  | 529                    | 400               | 56             | 19817                       | 5.21349             | 27.58                 | 5.05      | 2.66       | 14.65                     | 1.11                 | 103060      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | or1200.v           | 4530                  | 1008                   | 729               | 76             | 48034                       | 8.70797             | 202.25                | 10.1      | 8.31       | 171.96                    | 2.86                 | 178712      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | raygentop.v        | 2934                  | 634                    | 361               | 58             | 20799                       | 5.04571             | 22.58                 | 2.75      | 2.42       | 12.86                     | 1.64                 | 108116      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | sha.v              | 3024                  | 236                    | 289               | 62             | 16052                       | 10.5007             | 337.19                | 5.32      | 2.25       | 4.52                      | 0.69                 | 105948      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision0.v    | 21801                 | 1121                   | 1156              | 58             | 70046                       | 3.61684             | 86.5                  | 9.5       | 15.02      | 41.81                     | 2.59                 | 376100      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision1.v    | 19538                 | 1080                   | 1600              | 92             | 142805                      | 6.02319             | 343.83                | 10.68     | 16.21      | 247.99                    | 11.66                | 480352      |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision2.v    | 42078                 | 2416                   | 7396              | 124            | 646793                      | 14.6606             | 5614.79               | 34.81     | 107.66     | 5383.58                   | 62.27                | 2682976     |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision3.v    | 324                   | 54                     | 49                | 34             | 920                         | 2.5281              | 1.55                  | 0.31      | 0.14       | 0.43                      | 0.05                 | 63444       |

Based on these metrics we then calculate the following ratios and summary.

**QoR Metric Ratio** (Modified QoR / Baseline QoR):

| arch                                   | circuit            | num_pre_packed_blocks | num_post_packed_blocks | device_grid_tiles | min_chan_width | crit_path_routed_wirelength | critical_path_delay | vtr_flow_elapsed_time | pack_time | place_time | min_chan_width_route_time | crit_path_route_time | max_vpr_mem |
|----------------------------------------|--------------------|-----------------------|------------------------|-------------------|----------------|-----------------------------|---------------------|-----------------------|-----------|------------|---------------------------|----------------------|-------------|
| k6_frac_N10_frac_chain_mem32K_40nm.xml | bgm.v              | 1.00                  | 0.97                   | 1.00              | 0.98           | 1.02                        | 1.00                | 0.98                  | 0.50      | 1.05       | 1.39                      | 1.04                 | 0.92        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | blob_merge.v       | 1.00                  | 0.98                   | 1.00              | 1.13           | 1.02                        | 0.96                | 0.90                  | 0.51      | 1.18       | 1.22                      | 0.96                 | 0.91        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | boundtop.v         | 1.00                  | 0.95                   | 1.00              | 1.18           | 0.92                        | 0.89                | 0.62                  | 0.44      | 0.85       | 0.43                      | 1.13                 | 0.95        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | ch_intrinsics.v    | 1.00                  | 0.98                   | 1.00              | 1.17           | 0.99                        | 1.02                | 0.68                  | 0.41      | 0.87       | 0.46                      | 1.00                 | 0.97        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | diffeq1.v          | 1.00                  | 0.94                   | 1.00              | 0.83           | 1.04                        | 0.96                | 0.98                  | 0.28      | 0.71       | 1.13                      | 1.55                 | 0.83        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | diffeq2.v          | 1.00                  | 0.93                   | 1.00              | 0.96           | 0.85                        | 0.96                | 1.08                  | 0.45      | 1.20       | 1.20                      | 0.83                 | 0.80        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | LU8PEEng.v         | 1.00                  | 0.98                   | 1.00              | 0.98           | 1.00                        | 0.98                | 0.84                  | 0.50      | 1.00       | 0.84                      | 1.08                 | 0.94        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | LU32PEEng.v        | 1.00                  | 0.96                   | 1.00              | 1.11           | 1.09                        | 1.00                | 1.01                  | 0.49      | 0.96       | 1.02                      | 1.11                 | 0.91        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mcml.v             | 1.00                  | 0.95                   | 1.00              | 0.97           | 0.95                        | 0.89                | 1.03                  | 0.57      | 0.92       | 1.20                      | 0.78                 | 0.87        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkDelayWorker32B.v | 1.00                  | 0.91                   | 1.00              | 0.95           | 1.11                        | 1.00                | 0.82                  | 0.50      | 0.96       | 0.75                      | 0.91                 | 0.98        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkPktMerge.v       | 1.00                  | 0.96                   | 1.00              | 0.82           | 1.04                        | 1.03                | 0.95                  | 0.33      | 0.92       | 1.08                      | 0.96                 | 0.96        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | mkSMAdapter4B.v    | 1.00                  | 0.97                   | 1.00              | 1.17           | 1.03                        | 0.99                | 0.58                  | 0.31      | 0.64       | 0.73                      | 0.97                 | 0.89        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | or1200.v           | 1.00                  | 0.76                   | 1.00              | 1.23           | 0.93                        | 0.90                | 1.91                  | 0.30      | 0.64       | 3.83                      | 0.86                 | 0.81        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | raygentop.v        | 1.00                  | 0.89                   | 1.00              | 1.00           | 0.94                        | 0.98                | 0.57                  | 0.29      | 0.60       | 0.65                      | 0.70                 | 0.86        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | sha.v              | 1.00                  | 1.00                   | 1.00              | 1.00           | 0.96                        | 1.05                | 0.86                  | 0.46      | 0.83       | 0.73                      | 0.92                 | 0.90        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision0.v    | 1.00                  | 1.00                   | 1.00              | 1.00           | 1.08                        | 1.00                | 1.05                  | 0.46      | 0.97       | 1.71                      | 1.00                 | 0.91        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision1.v    | 1.00                  | 0.99                   | 1.00              | 0.92           | 1.00                        | 1.07                | 1.26                  | 0.40      | 0.89       | 1.66                      | 0.75                 | 0.71        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision2.v    | 1.00                  | 0.95                   | 1.00              | 0.93           | 0.99                        | 0.96                | 1.53                  | 0.52      | 0.90       | 1.59                      | 0.99                 | 0.86        |
| k6_frac_N10_frac_chain_mem32K_40nm.xml | stereovision3.v    | 1.00                  | 0.98                   | 1.00              | 1.13           | 1.20                        | 0.95                | 0.69                  | 0.41      | 0.70       | 0.75                      | 1.00                 | 1.04        |
|                                        | GEOMEAN            | 1.00                  | 0.95                   | 1.00              | 1.02           | 1.01                        | 0.98                | 0.92                  | 0.42      | 0.87       | 1.03                      | 0.96                 | 0.89        |

**QoR Summary:**

|                             | baseline | modified |
|-----------------------------|----------|----------|
| num_pre_packed_blocks       | 1.00     | 1.00     |
| num_post_packed_blocks      | 1.00     | 0.95     |
| device_grid_tiles           | 1.00     | 1.00     |
| min_chan_width              | 1.00     | 1.02     |
| crit_path_routed_wirelength | 1.00     | 1.01     |
| critical_path_delay         | 1.00     | 0.98     |
| vtr_flow_elapsed_time       | 1.00     | 0.92     |
| pack_time                   | 1.00     | 0.42     |
| place_time                  | 1.00     | 0.87     |
| min_chan_width_route_time   | 1.00     | 1.03     |
| crit_path_route_time        | 1.00     | 0.96     |
| max_vpr_mem                 | 1.00     | 0.89     |

From the results we can see that our change, on average, achieved a small reduction in the number of logic blocks (0.95) in return for a 2% increase in minimum channel width and 1% increase in routed wirelength. From a run-time perspective the packer is substantially faster (0.42).

### Automated QoR Comparison Script
To automate some of the QoR comparison VTR includes a script to compare `parse_results.txt` files and generate a spreadsheet including the ratio and summary tables.

For example:
```shell
#From the VTR Root
$ ./vtr_flow/scripts/qor_compare.py parse_results1.txt parse_results2.txt parse_results3.txt -o comparison.xlsx
```
will produce ratio tables and a summary table for the files parse_results1.txt, parse_results2.txt and parse_results3.txt, where the first file (parse_results1.txt) is assumed to be the baseline used to produce normalized ratios.

### Generating New QoR Golden Result
There may be times when a regression test fails its QoR test because its golden_result needs to be changed due to known changes in code behaviour. In this case, a new golden result needs to be generated so that the test can be passed. To generate a new golden result, follow the steps outlined below.

1. Move to the `vtr_flow/tasks` directory from the VTR root, and run the failing test. For example, if a test called `vtr_ex_test` in `vtr_reg_nightly` was failing:

	```shell
    #From the VTR root
    $ cd vtr_flow/tasks
    $ ../scripts/run_vtr_task.pl regression_tests/vtr_reg_nightly/vtr_ex_test
	```
2. Next, generate new golden reference results using `parse_vtr_task.pl` and the `-create_golden` option.

    ```shell
    $ ../scripts/parse_vtr_task.pl regression_tests/vtr_reg_nightly/vtr_ex_test -create_golden
    ```
3. Lastly, check that the results match with the `-check_golden` option

    ```shell
    $ ../scripts/parse_vtr_task.pl regression_tests/vtr_reg_nightly/vtr_ex_test -check_golden
    ```
Once the `-check_golden` command passes, the changes to the golden result can be committed so that the reg test will pass in future runs of vtr_reg_nightly.

# Adding Tests

Any time you add a feature to VTR you **must** add a test which exercises the feature.
This ensures that regression tests will detect if the feature breaks in the future.

Consider which regression test suite your test should be added to (see [Running Tests](#running-tests) descriptions).

Typically, test which exercise new features should be added to `vtr_reg_strong`.
These tests should use small benchmarks to ensure they:
 * run quickly (so they get run often!), and
 * are easier to debug.
If your test will take more than ~1 minute it should probably go in a longer running regression test (but see first if you can create a smaller testcase first).

## Adding a test to vtr_reg_strong
This describes adding a test to `vtr_reg_strong`, but the process is similar for the other regression tests.

1. Create a configuration file

    First move to the vtr_reg_strong directory:
    ```shell
    #From the VTR root directory
    $ cd vtr_flow/tasks/regression_tests/vtr_reg_strong
    $ ls
    qor_geomean.txt             strong_flyover_wires        strong_pack_and_place
    strong_analysis_only        strong_fpu_hard_block_arch  strong_power
    strong_bounding_box         strong_fracturable_luts     strong_route_only
    strong_breadth_first        strong_func_formal_flow     strong_scale_delay_budgets
    strong_constant_outputs     strong_func_formal_vpr      strong_sweep_constant_outputs
    strong_custom_grid          strong_global_routing       strong_timing
    strong_custom_pin_locs      strong_manual_annealing     strong_titan
    strong_custom_switch_block  strong_mcnc                 strong_valgrind
    strong_echo_files           strong_minimax_budgets      strong_verify_rr_graph
    strong_fc_abs               strong_multiclock           task_list.txt
    strong_fix_pins_pad_file    strong_no_timing            task_summary
    strong_fix_pins_random      strong_pack
    ```
    Each folder (prefixed with `strong_` in this case) defines a task (sub-test).

    Let's make a new task named `strong_mytest`.
    An easy way is to copy an existing configuration file such as `strong_timing/config/config.txt`
    ```shell
    $ mkdir -p strong_mytest/config
    $ cp strong_timing/config/config.txt strong_mytest/config/.
    ```
    You can now edit `strong_mytest/config/config.txt` to customize your test.

2. Generate golden reference results

    Now we need to test our new test and generate 'golden' reference results.
    These will be used to compare future runs of our test to detect any changes in behaviour (e.g. bugs).

    From the VTR root, we move to the `vtr_flow/tasks` directory, and then run our new test:
    ```shell
    #From the VTR root
    $ cd vtr_flow/tasks
    $ ../scripts/run_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest

    regression_tests/vtr_reg_strong/strong_mytest
    -----------------------------------------
    Current time: Jan-25 06:51 PM.  Expected runtime of next benchmark: Unknown
    k6_frac_N10_mem32K_40nm/ch_intrinsics...OK
    ```

    Next we can generate the golden reference results using `parse_vtr_task.py` with the `-create_golden` option:
    ```shell
    $ ../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest -create_golden
    ```

    And check that everything matches with `-check_golden`:
    ```shell
    $ ../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest -check_golden
    regression_tests/vtr_reg_strong/strong_mytest...[Pass]
    ```

3. Add it to the task list

    We now need to add our new `strong_mytest` task to the task list, so it is run whenever `vtr_reg_strong` is run.
    We do this by adding the line `regression_tests/vtr_reg_strong/strong_mytest` to the end of `vtr_reg_strong`'s `task_list.txt`:
    ```shell
    #From the VTR root directory
    $ vim vtr_flow/tasks/regression_tests/vtr_reg_strong/task_list.txt
    # Add a new line 'regression_tests/vtr_reg_strong/strong_mytest' to the end of the file
    ```

    Now, when we run `vtr_reg_strong`:
    ```shell
    #From the VTR root directory
    $ ./run_reg_test.py vtr_reg_strong
    #Output trimmed...
    regression_tests/vtr_reg_strong/strong_mytest
    -----------------------------------------
    #Output trimmed...
    ```
    we see our test is run.

4. Commit the new test

    Finally you need to commit your test:
    ```shell
    #Add the config.txt and golden_results.txt for the test
    $ git add vtr_flow/tasks/regression_tests/vtr_reg_strong/strong_mytest/
    #Add the change to the task_list.txt
    $ git add vtr_flow/tasks/regression_tests/vtr_reg_strong/task_list.txt
    #Commit the changes, when pushed the test will automatically be picked up by BuildBot
    $ git commit
    ```

# Debugging Aids
VTR has support for several additional tools/features to aid debugging.

## Sanitizers
VTR can be compiled using *sanitizers* which will detect invalid memory accesses, memory leaks and undefined behaviour (supported by both GCC and LLVM):
```shell
#From the VTR root directory
$ cmake -D VTR_ENABLE_SANITIZE=ON build
$ make
```

## Assertion Levels
VTR supports configurable assertion levels.

The default level (`2`) which turns on most assertions which don't cause significant run-time penalties.

This level can be increased:
```shell
#From the VTR root directory
$ cmake -D VTR_ASSERT_LEVEL=3 build
$ make
```
this turns on more extensive assertion checking and re-builds VTR.

## GDB Pretty Printers
To make it easier to debug some of VTR's data structures with [GDB](www.gnu.org/gdb).

### STL Pretty Printers

It is helpful to enable [STL pretty printers](https://sourceware.org/gdb/wiki/STLSupport), which make it much easier to debug data structures using STL.

For example printing a `std::vector<int>` by default prints:

    (gdb) p/r x_locs
    $2 = {<std::_Vector_base<int, std::allocator<int> >> = {
        _M_impl = {<std::allocator<int>> = {<__gnu_cxx::new_allocator<int>> = {<No data fields>}, <No data fields>}, _M_start = 0x555556f063b0, 
          _M_finish = 0x555556f063dc, _M_end_of_storage = 0x555556f064b0}}, <No data fields>}

which is not very helpful.

But with STL pretty printers it prints:

    (gdb) p x_locs
    $2 = std::vector of length 11, capacity 64 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

which is much more helpful for debugging!

If STL pretty printers aren't already enabled on your system, add the following to your [.gdbinit file](https://sourceware.org/gdb/current/onlinedocs/gdb/gdbinit-man.html):

    python
    import sys
    sys.path.insert(0, '$STL_PRINTER_ROOT')
    from libstdcxx.v6.printers import register_libstdcxx_printers
    register_libstdcxx_printers(None)

    end

where `$STL_PRINTER_ROOT` should be replaced with the appropriate path to the STL pretty printers.
For example recent versions of GCC include these under `/usr/share/gcc-*/python` (e.g. `/usr/share/gcc-9/python`)


### VTR Pretty Printers

VTR includes some pretty printers for some VPR/VTR specific types.

For example, without the pretty printers you would see the following when printing a VPR `AtomBlockId`:

    (gdb) p blk_id
    $1 = {
      id_ = 71
    }

But with the VTR pretty printers enabled you would see:

    (gdb) p blk_id
    $1 = AtomBlockId(71)

To enable the VTR pretty printers in GDB add the following to your [.gdbinit file](https://sourceware.org/gdb/current/onlinedocs/gdb/gdbinit-man.html):

    python
    import sys

    sys.path.insert(0, "$VTR_ROOT/dev")
    import vtr_gdb_pretty_printers
    gdb.pretty_printers.append(vtr_gdb_pretty_printers.vtr_type_lookup)

    end

where ``$VTR_ROOT`` should be replaced with the root of the VTR source tree on your system.

## RR (Record Replay) Debugger

[RR](https://rr-project.org/) extends GDB with the ability to to record a run of a tool and then re-run it to reproduce any observed issues.
RR also enables efficient reverse execution (!) which can be *extremely helpful* when tracking down the source of a bug.

# Speeding up the edit-compile-test cycle
Rapid iteration through the edit-compile-test/debug cycle is very helpful when making code changes to VTR.

The following is some guidance on techniques to reduce the time required.

# Speeding Compilation

1. Parallel compilation

    For instance when [building VTR](BUILDING.md) using make, you can specify the `-j N` option to compile the code base with N parallel jobs:
    ```
    $ make -j N
    ```

    A reasonable value for `N` is equal to the number of threads you system can run. For instance, if your system has 4 cores with HyperThreading (i.e. 2-way SMT) you could run:
    ```
    $ make -j8
    ```

2. Building only a subset of VTR

    If you know your changes only effect a specific tool in VTR, you can request that only that tool is rebuilt.
    For instance, if you only wanted to re-compile VPR you could run:
    ```
    $ make vpr
    ```
    which would avoid re-building other tools (e.g. ODIN, ABC).

3. Use ccache

    [ccache](https://ccache.dev/) is a program which caches previous compilation results.
    This can save significant time, for instance, when switching back and forth between release and debug builds.

    VTR's cmake configuration should automatically detect and make use of ccache once it is installed.

    For instance on Ubuntu/Debian systems you can install ccache with:
    ```
    $ sudo apt install ccache
    ```
    This only needs to be done once on your development system.

4. Disable Interprocedural Optimizatiaons (IPO)

    IPO re-optimizes an entire executable at link time, and is automatically enabled by VTR if a supporting compiler is found.
    This can notably improve performance (e.g. ~10-20% faster), but can significantly increase compilation time (e.g. >2x in some cases).
    When frequently re-compiling and debugging the extra execution speed may not be worth the longer compilation times.
    In such cases you can manually disable IPO by setting the cmake parameter `VTR_IPO_BUILD=off`.

    For instance using the wrapper Makefile:
    ```
    $ make CMAKE_PARAMS="-DVTR_IPO_BUILD=off"
    ```
    Note that this option is sticky, so subsequent calls to make don't need to keep specifying VTR_IPO_BUILD, until you want to re-enable it.

    This setting can also be changed with the ccmake tool (i.e. `ccmake build`).

All of these option can be used in combination.
For example, the following will re-build only VPR using 8 parallel jobs with IPO disabled:
```
make CMAKE_PARAMS="-DVTR_IPO_BUILD=off" -j8 vpr
```

# Profiling VTR

1. Install `gprof`, `gprof2dot`, and `xdot`. Specifically, the previous two packages require python3, and you should install the last one with `sudo apt install` for all the dependencies you will need for visualizing your profile results.
    ```
    pip3 install gprof
    pip3 install gprof2dot
    sudo apt install xdot
    ```

    Contact your administrator if you do not have the `sudo` rights.

2. Use the CMake option below to enable VPR profiler build.
    ```
    make CMAKE_PARAMS="-DVTR_ENABLE_PROFILING=ON" vpr
    ```

3. With the profiler build, each time you run the VTR flow script, it will produce an extra file `gmon.out` that contains the raw profile information. 
    Run `gprof` to parse this file. You will need to specify the path to the VPR executable.
    ```
    gprof $VTR_ROOT/vpr/vpr gmon.out > gprof.txt
    ```

4. Next, use `gprof2dot` to transform the parsed results to a `.dot` file, which describes the graph of your final profile results. If you encounter long function names, specify the `-s` option for a cleaner graph.
    ```
    gprof2dot -s gprof.txt > vpr.dot
    ```

5. You can chain the above commands to directly produce the `.dot` file:
    ```
    gprof $VTR_ROOT/vpr/vpr gmon.out | gprof2dot -s > vpr.dot
    ```

6. Use `xdot` to view your results:
    ```
    xdot vpr.dot
    ```

7. To save your results as a `png` file:
    ```
    dot -Tpng -Gdpi=300 vpr.dot > vpr.png
    ```
    
    Note that you can use the `-Gdpi` option to make your picture clearer if you find the default dpi settings not clear enough.

# External Subtrees
VTR includes some code which is developed in external repositories, and is integrated into the VTR source tree using [git subtrees](https://www.atlassian.com/blog/git/alternatives-to-git-submodule-git-subtree).

To simplify the process of working with subtrees we use the [`dev/external_subtrees.py`](./dev/external_subtrees.py) script.

For instance, running `./dev/external_subtrees.py --list` from the VTR root it shows the subtrees:
```
Component: abc             Path: abc                            URL: https://github.com/berkeley-abc/abc.git       URL_Ref: master
Component: libargparse     Path: libs/EXTERNAL/libargparse      URL: https://github.com/kmurray/libargparse.git    URL_Ref: master
Component: libblifparse    Path: libs/EXTERNAL/libblifparse     URL: https://github.com/kmurray/libblifparse.git   URL_Ref: master
Component: libsdcparse     Path: libs/EXTERNAL/libsdcparse      URL: https://github.com/kmurray/libsdcparse.git    URL_Ref: master
Component: libtatum        Path: libs/EXTERNAL/libtatum         URL: https://github.com/kmurray/tatum.git          URL_Ref: master
```

Code included in VTR by subtrees should *not be modified within the VTR source tree*.
Instead changes should be made in the relevant up-stream repository, and then synced into the VTR tree.

### Updating an existing Subtree
1. From the VTR root run: `./dev/external_subtrees.py $SUBTREE_NAME`, where `$SUBTREE_NAME` is the name of an existing subtree.

    For example to update the `libtatum` subtree:
    ```shell
    ./dev/external_subtrees.py --update libtatum
    ```

### Adding a new Subtree

To add a new external subtree to VTR do the following:

1. Add the subtree specification to `dev/subtree_config.xml`.

    For example to add a subtree name `libfoo` from the `master` branch of `https://github.com/kmurray/libfoo.git` to `libs/EXTERNAL/libfoo` you would add:
    ```xml
    <subtree
        name="libfoo"
        internal_path="libs/EXTERNAL/libfoo"
        external_url="https://github.com/kmurray/libfoo.git"
        default_external_ref="master"/>
    ```
    within the existing `<subtrees>` tag.

    Note that the internal_path directory should not already exist.

    You can confirm it works by running: `dev/external_subtrees.py --list`:
    ```
    Component: abc             Path: abc                            URL: https://github.com/berkeley-abc/abc.git       URL_Ref: master
    Component: libargparse     Path: libs/EXTERNAL/libargparse      URL: https://github.com/kmurray/libargparse.git    URL_Ref: master
    Component: libblifparse    Path: libs/EXTERNAL/libblifparse     URL: https://github.com/kmurray/libblifparse.git   URL_Ref: master
    Component: libsdcparse     Path: libs/EXTERNAL/libsdcparse      URL: https://github.com/kmurray/libsdcparse.git    URL_Ref: master
    Component: libtatum        Path: libs/EXTERNAL/libtatum         URL: https://github.com/kmurray/tatum.git          URL_Ref: master
    Component: libfoo          Path: libs/EXTERNAL/libfoo           URL: https://github.com/kmurray/libfoo.git         URL_Ref: master
    ```
    which shows libfoo is now recognized.

2. Run `./dev/external_subtrees.py --update $SUBTREE_NAME` to add the subtree.

    For the `libfoo` example above this would be:
    ```shell
    ./dev/external_subtrees.py --update libfoo
    ```

    This will create two commits to the repository.
    The first will squash all the upstream changes, the second will merge those changes into the current branch.


### Subtree Rational

VTR uses subtrees to allow easy tracking of upstream dependencies.

Their main advantages included:
 * Works out-of-the-box: no actions needed post checkout to pull in dependencies (e.g. no `git submodule update --init --recursive`)
 * Simplified upstream version tracking
 * Potential for local changes (although in VTR we do not use this to make keeping in sync easier)

See [here](https://blogs.atlassian.com/2013/05/alternatives-to-git-submodule-git-subtree/) for a more detailed discussion.

# Finding Bugs with Coverity
[Coverity Scan](https://scan.coverity.com) is a static code analysis service which can be used to detect bugs.

### Browsing Defects
To view defects detected do the following:

1. Get a coverity scan account

    Contact a project maintainer for an invitation.

2. Browse the existing defects through the coverity web interface


### Submitting a build
To submit a build to coverity do the following:

1. [Download](https://scan.coverity.com/download) the coverity build tool

2. Configure VTR to perform a *debug* build. This ensures that all assertions are enabled, without assertions coverity may report bugs that are guarded against by assertions. We also set VTR asserts to the highest level.

    ```shell
    #From the VTR root
    mkdir -p build
    cd build
    CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=debug -DVTR_ASSERT_LEVEL=3 ..
    ```

Note that we explicitly asked for gcc and g++, the coverity build tool defaults to these compilers, and may not like the default 'cc' or 'c++' (even if they are linked to gcc/g++).

3. Run the coverity build tool

    ```shell
    #From the build directory where we ran cmake
    cov-build --dir cov-int make -j8
    ```

4. Archive the output directory

    ```shell
    tar -czvf vtr_coverity.tar.gz cov-int
    ```

5. Submit the archive through the coverity web interface

Once the build has been analyzed you can browse the latest results through the coverity web interface

### No files emitted
If you get the following warning from cov-build:

    [WARNING] No files were emitted.

You may need to configure coverity to 'know' about your compiler. For example:

    ```shell
    cov-configure --compiler `which gcc-7`
    ```

On unix-like systems run `scan-build make` from the root VTR directory.
to output the html analysis to a specific folder, run `scan-build make -o /some/folder`

# Release Procedures

## General Principles

We periodically make 'official' VTR releases.
While we aim to keep the VTR master branch stable through-out development some users prefer to work of off an official release.
Historically this has coincided with the publishing of a paper detailing and carefully evaluating the changes from the previous VTR release.
This is particularly helpful for giving academics a named baseline version of VTR to which they can compare which has a known quality.

In preparation for a release it may make sense to produce 'release candidates' which when fully tested and evaluated (and after any bug fixes) become the official release.

## Checklist

The following outlines the procedure to following when making an official VTR release:

 * Check the code compiles on the list of supported compilers
 * Check that all regression tests pass functionality
 * Update regression test golden results to match the released version
 * Check that all regression tests pass QoR
 * Create a new entry in the CHANGELOG.md for the release, summarizing at a high-level user-facing changes
 * Increment the version number (set in root CMakeLists.txt)
 * Create a git annotated tag (e.g. `v8.0.0`) and push it to github
 * GitHub will automatically create a release based on the tag
 * Add the new change log entry to the [GitHub release description](https://github.com/verilog-to-routing/vtr-verilog-to-routing/releases)
 * Update the [ReadTheDocs configuration](https://readthedocs.org/projects/vtr/versions/) to build and serve documentation for the relevant tag (e.g. `v8.0.0`)
 * Send a release announcement email to the [vtr-announce](vtr-announce@googlegroups.com) mailing list (make sure to thank all contributors!)

