# Contributing

The Odin II team welcomes outside help from anyone interested.
To fix issues or add a new feature submit a PR or WIP PR following the provided guidelines.  

## Creating a Pull Request (PR)

**Important** Before creating a Pull Request (PR), if it is a bug you have happened upon and intend to fix make sure you create an issue beforhand.

Pull requests are intended to correct bugs and improve Odin's performance.
To create a pull request, clone the vtr-verilog-rooting repository and branch from the master.
Make changes to the branch that improve Odin II and correct the bug.
**Important** In addition to correcting the bug, it is required that test cases (benchmarks) are created that reproduce the issue and are included in the regression tests.
An example of a good test case could be the benchmark found in the "Issue" being addressed.
The results of these new tests need to be regenerate. See [regression tests](./regression_tests) for further instruction.
Push these changes to the cloned repository and create the pull request.
Add a description of the changes made and reference the "issue" that it corrects. There is a template provided on GitHub.

### Creating a "Work in progress" (WIP) PR

**Important** Before creating a WIP PR, if it is a bug you have happened upon and intend to fix make sure you create an issue beforhand.

A "work in progress" PR is a pull request that isn't complete or ready to be merged.
It is intended to demonstrate that an Issue is being addressed and indicates to other developpers that they don't need to fix it.
Creating a WIP PR is similar to a regular PR with a few adjustements.
First, clone the vtr-verilog-rooting repository and branch from the master.
Make changes to that branch.
Then, create a pull request with that branch and **include WIP in the title.**
This will automatically indicate that this PR is not ready to be merged.
Continue to work on the branch, pushing the commits regularly.
Like a PR, test cases are also needed to be included through the use of benchmarks.
See [regression tests](./regression_tests) for further instruction.

### Formating

Odin II shares the same contributing philosophy as [VPR](https://docs.verilogtorouting.org/en/latest/dev/contributing/contributing/).
Most importantly PRs will be rejected if they do not respect the coding standard: see [VPRs coding standard](https://docs.verilogtorouting.org/en/latest/dev/developing/#code-formatting)
