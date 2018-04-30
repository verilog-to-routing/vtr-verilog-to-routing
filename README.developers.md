# Commit Procedures

## For external developers
See [Submitting Code to VTR](CONTRIBUTING.md#submitting-code-to-vtr).

## For developers with commit rights
The guiding principle in internal development is to submit your work into the repository without breaking other people's work.
When you commit, make sure that the repository compiles, that the flow runs, and that you did not clobber someone else's work.
In the event that you are responsible for "breaking the build", fix the build at top priority.

We have some guidelines in place to help catch most of these problems:

1.  Before you push code to the central repository, your code MUST pass the check-in regression test. 
    The check-in regression test is a quick way to test if any part of the VTR flow is broken.

    At a minimum you must run:
    ```shell
    #From the VTR root directory
    $ ./run_reg_test.pl vtr_reg_basic
    ```
    You may push if all the tests return `All tests passed`.

    However you are strongly encouraged to run both the *basic* and *strong* regression tests:
    ```shell
    #From the VTR root directory
    $ ./run_reg_test.pl vtr_reg_basic vtr_reg_strong
    ```
    since it performs much more thorough testing.

    It is typically a good idea to run tests regularily as you make changes.
    If you have failures see [how to debugging failed tests](#debugging-failed-tests).

2.  The automated [BuildBot](http://builds.verilogtorouting.org:8080/waterfall) will perform more extensive regressions tests and mark which revisions are stable.

3.  Everyone who is doing development must write regression tests for major feature they create.
    This ensures regression testing will detect if a feature is broken by someone (or yourself).
    See [Adding Tests](#adding-tests) for details.

4.  In the event a regression test is broken, the one responsible for having the test pass is in charge of determining:
    * If there is a bug in the source code, in which case the source code needs to be updated to fix the bug, or
    * If there is a problem with the test (perhaps the quality of the tool did in fact get better or perhaps there is a bug with the test itself), in which case the test needs to be updated to reflect the new changes. 
   
    If the golden results need to be updated and you are sure that the new golden results are better, use the command `../scripts/parse_vtr_task.pl -create_golden your_regression_test_name_here`

5.  Keep in sync with the master branch as regularly as you can (i.e. `git pull` or `git pull --rebase`).
    The longer code deviates from the trunk, the more painful it is to integrate back into the trunk.

Whatever system that we come up with will not be foolproof so be conscientious about how your changes will effect other developers.

# Running Tests

VTR has a variety of tests which are used to check for correctness, performance and Quality of Result (QoR).

There are 4 main regression tests:

* `vtr_reg_basic`: ~3 minutes serial

    **Goal:** Quickly check basic VTR flow correctness

    **Feature Coverage:** Low

    **Benchmarks:** A few small and simple circuits

    **Architectures:** A few simple architectures

    Not suitable for evaluating QoR or performance.

* `vtr_reg_strong`: ~30 minutes serial, ~15 minutes with `-j4`

    **Goal:** Exercise most of VTR's features, moderately fast.

    **Feature Coverage:** High

    **Benchmarks:** A few small circuits, with some special benchmarks to exercise specific features

    **Architectures:** A variety of architectures, including special architectures to exercise specific features

    Not suitable for evaluating QoR or performance.

* `vtr_reg_nightly`: ~15 hours with `-j2`

    **Goal:** Basic QoR and Performance evaluation.

    **Feature Coverage:** Medium

    **Benchmarks:** Small-medium size, diverse. Includes:

    * MCNC20 benchmarks
    * VTR benchmarks
    * Titan 'other' benchmarks (smaller than Titan23)
   
    **Architectures:** A wider variety of architectures
   
* `vtr_reg_weekly`: ~30 hours with `-j2`

    **Goal:** Full QoR and Performance evaluation.

    **Feature Coverage:** Medium

    **Benchmarks:** Medium-Large size, diverse. Includes:

    * VTR benchmarks
    * Titan23 benchmarks
   
    **Architectures:** A wide variety of architectures

These can be run with `run_reg_test.pl`:
```shell
#From the VTR root directory
$ ./run_reg_test.pl vtr_reg_basic
$ ./run_reg_test.pl vtr_reg_strong
```

The *nightly* and *weekly* regressions require the Titan benchmarks which can be integrated into your VTR tree with:
```shell
make get_titan_benchmarks
```
They can then be run using `run_reg_test.pl`:
```shell
$ ./run_reg_test.pl vtr_reg_nightly
$ ./run_reg_test.pl vtr_reg_weekly
```

To speed-up things up, individual sub-tests can be run in parallel using the `-j` option:
```shell
#Run up to 4 tests in parallel
$ ./run_reg_test.pl vtr_reg_strong -j4
```

You can also run multiple regression tests together:
```shell
#Run both the basic and strong regression, with up to 4 tests in parallel
$ ./run_reg_test.pl vtr_reg_basic vtr_reg_strong -j4
```

## Odin Functionality Tests

Odin has its own set of tests to verify the correctness of its synthesis results:

* `odin_reg_micro`: ~2 minutes serial
* `odin_reg_full`: ~6 minutes serial

These can be run with:
```shell
#From the VTR root directory
$ ./run_reg_test.pl odin_reg_micro
$ ./run_reg_test.pl odin_reg_full
```
and should be used when makeing changes to Odin.

## Unit Tests

VTR also has a limited set of unit tests, which can be run with:
```shell
#From the VTR root directory
$ make && make test
```

# Debugging Failed Tests

If a test fails you probably want to look at the log files to determine the cause.

Lets assume we have a failure in `vtr_reg_basic`:

```shell
#In the VTR root directory
$ ./run_reg_test.pl vtr_reg_strong
#Output trimmed...
regression_tests/vtr_reg_basic/basic_no_timing
-----------------------------------------
k4_N10_memSize16384_memData64/ch_intrinsics...failed: vpr
k4_N10_memSize16384_memData64/diffeq1...failed: vpr
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
config  run002  run004
run001  run003  run005
```

There we see there is a `config` directory (which defines the test), and a set of run-directories.
Each time a test is run it creates a new `runXXX` directory (where `XXX` is an incrementing number).
From the above we can tell that our last run was `run005`.
From the output of `run_reg_test.pl` we know that one of the failing architecture/circuit combinations was `k4_N10_memSize16384_memData64/ch_intrinsics`.
Each architecture/circuit combination is run in its own sub-folder.
Lets move to that directory:
```shell
$ cd run005/k4_N10_memSize16384_memData64/ch_intrinsics
$ ls
abc.out                     k4_N10_memSize16384_memData64.xml  qor_results.txt
ch_intrinsics.net           odin.out                           thread_1.out
ch_intrinsics.place         output.log                         vpr.out
ch_intrinsics.pre-vpr.blif  output.txt                         vpr_stdout.log
ch_intrinsics.route         parse_results.txt
```

Here we can see the individual log files produced by each tool (e.g. `vpr.out`), which we can use to guide our debugging.
We could also manually re-run the tools (e.g. with a debugger) using files in this directory.

# Adding Tests

Any time you add a feature to VTR you **must** add a test which exercies the feature.
This ensures that regression tests will detect if the feature breaks in the future.

Consider which regression test suite your test should be added to (see [Running Tests](#running-tests) descriptions).

Typically, test which exercise new features should be added to `vtr_reg_strong`.
These tests should use small benchmarks to ensure they:
 * run quickly (so they get run often!), and
 * are easier to debug.
If your test will take more than ~2 mintues it should probably go in a longer running regression test (but see first if you can create a smaller testcase first).

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
    $ ../scripts/run_vtr_task.pl regression_tests/vtr_reg_strong/strong_mytest

    regression_tests/vtr_reg_strong/strong_mytest
    -----------------------------------------
    Current time: Jan-25 06:51 PM.  Expected runtime of next benchmark: Unknown
    k6_frac_N10_mem32K_40nm/ch_intrinsics...OK
    ```

    Next we can generate the golden reference results using `parse_vtr_task.pl` with the `-create_golden` option:
    ```shell
    $ ../scripts/parse_vtr_task.pl regression_tests/vtr_reg_strong/strong_mytest -create_golden
    ```

    And check that everything matches with `-check_golden`:
    ```shell
    $ ../scripts/parse_vtr_task.pl regression_tests/vtr_reg_strong/strong_mytest -check_golden
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
    $ ./run_reg_test.pl vtr_reg_strong
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

# External Subtrees
VTR includes some code which is developed in external repositories, and is integrated into the VTR source tree using [git subtrees](https://www.atlassian.com/blog/git/alternatives-to-git-submodule-git-subtree).

To simplify the process of working with subtrees we use the `dev/update_external_subtrees.py` script.

For instance, running `./dev/update_external_subtrees.py --list` from the VTR root it shows the subtrees:
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

    You can confirm it works by running: `def/external_subtrees.py --list`:
    ```
    Component: abc             Path: abc                            URL: https://github.com/berkeley-abc/abc.git       URL_Ref: master
    Component: libargparse     Path: libs/EXTERNAL/libargparse      URL: https://github.com/kmurray/libargparse.git    URL_Ref: master
    Component: libblifparse    Path: libs/EXTERNAL/libblifparse     URL: https://github.com/kmurray/libblifparse.git   URL_Ref: master
    Component: libsdcparse     Path: libs/EXTERNAL/libsdcparse      URL: https://github.com/kmurray/libsdcparse.git    URL_Ref: master
    Component: libtatum        Path: libs/EXTERNAL/libtatum         URL: https://github.com/kmurray/tatum.git          URL_Ref: master
    Component: libfoo          Path: libs/EXTERNAL/libfoo           URL: https://github.com/kmurray/libfoo.git         URL_Ref: master
    ```
    which shows libfoo is now recognized.

2. Run `./dev/update_external_subtrees.py $SUBTREE_NAME` to add the subtree.

    For the `libfoo` example above this would be:
    ```shell
    ./dev/update_external_subtrees.py libfoo
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

2. Configure VTR to perform a *debug* build. This ensures that all assertions are enabled, without assertions coverity may report bugs that are gaurded against by assertions. We also set VTR asserts to the highest level.

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

Once the build has been analyzed you can browse the latest results throught the coverity web interface

### No files emitted
If you get the following warning from cov-build:

    [WARNING] No files were emitted.

You may need to configure coverity to 'know' about your compiler. For example:

    ```shell
    cov-configure --compiler `which gcc-7`
    ```
   
On unix-like systems run `scan-build make` from the root VTR directory.
to output the html analysis to a specific folder, run `scan-build make -o /some/folder`

# Debugging with clang static analyser
First make sure you have clang installed.
define clang as the default compiler:
  `export CC=clang`
  `export CXX=clang++`

set the build type to `debug` in makefile
