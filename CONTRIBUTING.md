# Contribution Guidelines

Thanks for considering contributing to VTR!
Here are some helpful guidelines to follow.

## Common Scenarios

### I have a question
If you have questions about VTR take a look at our [Support Resources](SUPPORT.md).

If the answer to your question wasn't in the documentation (and you think it should have been), consider [enhancing the documentation](#enhancing-documentation).
That way someone (perhaps your future self!) will be able to quickly find the answer in the future.


### I found a bug!
While we strive to make VTR reliable and robust, bugs are inevitable in large-scale software projects.

Please file a [detailed bug report](#filling-bug-reports).
This ensures we know about the problem and can work towards fixing it.


### It would be great if VTR supported ...
VTR has many features and is highly flexible.
Make sure you've checkout out all our [Support Resources](SUPPORT.md) to see if VTR already supports what you want.

If VTR does not support your use case, consider [filling an enhancement](#filling-enhancement-requests).

### I have a bug-fix/feature I'd like to include in VTR
Great! Submitting bug-fixes and features is a great way to improve VTR.
See the guidlines for [submitting code](#submitting-code-to-vtr).

## The Details

### Enhancing Documentation
Enhancing documentation is a great way to start contributing to VTR.

You can edit the [documentation](https://docs.verilogtorouting.org) directly by clicking the `Edit on GitHub` link of the relevant page, or by editing the re-structured text (`.rst`) files under `doc/src`.

Generally it is best to make small incremental changes.
If you are considering larger changes its best to discuss them first (e.g. file a [bug](#filling-bug-reports) or [enhancement](#filling-enhancement-requests)).

Once you've made your enhancements [open a pull request](#making-pull-requests) to get your changes considered for inclusion in the documentation.


### Filling Bug Reports
First, search for [existing issues](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues?&=) to see if the bug has already been reported.

If no bug exists you will need to collect key pieces of information.
This information helps us to quickly reproduce (and hopefully fix) the issue:

* What behaviour you expect

    How you think VTR should be working.

* What behaviour you are seeing

    What VTR actually does on your system.

* Detailed steps to re-produce the bug

    *This is key to getting your bug fixed.*

    Provided *detailed steps* to reproduce the bug, including the exact commands to reproduce the bug.
    Attach all relevant files (e.g. FPGA architecture files, benchmark circuits, log files).

    If we can't re-produce the issue it is very difficult to fix.

* Context about what you are trying to achieve

    Sometimes VTR does things in a different way than you expect.
    Telling us what you are trying to accomplish helps us to come up with better real-world solutions.

* Details about your environment

    Tell us what version of VTR you are using (e.g. the output of `vpr --version`), which Operating System and compiler you are using, or any other relevant information about where or how you are building/running VTR.

Once you've gathered all the information [open an Issue](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new?template=bug_report.md) on our issue tracker.

If you know how to fix the issue, or already have it coded-up, please also consider [submitting the fix](#submitting-code-to-vtr).
This is likely the fastest way to get bugs fixed!

### Filling Enhancement Requests
First, search [existing issues](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues) to see if your enhancement request overlaps with an existing Issue.

If not feature request exists you will need to describe your enhancement:

* New behaviour

    How your proposed enhancement will work (from a user's perspective).

* Contrast with current behaviour

    How will your enhancement differ from the current behaviour (from a user's perspective).

* Potential Implementation

    Describe (if you have some idea) how the proposed enhancement would be implemented.

* Context

    What is the broader goal you are trying to accomplish? How does this enhancement help?
    This allows us to understand why this enhancement is beneficial, and come up with the best real-world solution.

**VTR developers have limited time and resources, and will not be able to address all feature requests.**
Typically, simple enhancements, and those which are broadly useful to a wide group of users get higher priority.

Features which are not generally useful, or useful to only a small group of users will tend to get lower priority.
(Of course [coding the enhancement yourself](#submitting-code-to-vtr) is an easy way to bypass this challenge).

Once you've gathered all the information [open an Issue](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new?template=feature_request.md) on our issue tracker.

### Submitting Code to VTR
VTR welcomes external contributions.

In general changes that are narrowly focused (e.g. small bug fixes) are easier to review and include in the code base.

Large changes, such as substantial new features or significant code-refactoring are more challenging to review.
It is probably best to file an [enhancement](#filling-enhancement-requests) first to discuss your approach.

Additionally, new features which are generally useful are much easier to justify adding to the code base, whereas features useful in only a few specialized cases are more difficult to justify.

Once your fix/enahcement is ready to go, [start a pull request](#making-pull-requests).

### Making Pull Requests
It is assumed that by opening a pull request to VTR you have permission to do so, and the changes are under the relevant [License](LICENSE.md).
VTR does not require a Contributor License Agreement (CLA) or formal Developer Certificate of Origin (DCO) for contributions.

Each pull request should describe it's motivation and context (linking to a relevant Issue for non-trivial changes).

Code-changes should also describe:

* The type of change (e.g. bug-fix, feature)

* How it has been tested

* What tests have been added

    All new features must have tests added which exercise the new features.
    This ensures any future changes which break your feature will be detected.
    It is also best to add tests when fixing bugs, for the same reason

    See [Adding Tests](README.developers.md#adding-tests) for details on how to create new regression tests.
    If you aren't sure what tests are needed, ask a maintainer.

* How the feature has been documented

    Any new user-facing features should be documented in the public documentation, which is in `.rst` format under `doc/src`, and served at https://docs.verilogtorouting.org

Once everything is ready [create a pull request](https://github.com/verilog-to-routing/vtr-verilog-to-routing/pulls).

**Tips for Pull Requests**
The following are general tips for making your pull requests easy to review (and hence more likely to be merged):

* Keep changes small

    Large change sets are difficult and time-consuming to review.
    If a change set is becoming too large, consider splitting it into smaller pieces; you'll probably want to [file an issue](#filling-feature-requests) to discuss things first.

* Do one thing only

    All the changes and commits in your pull request should be relevant to the bug/feature it addresses.
    There should be no unrelated changes (e.g. adding IDE files, re-formatting unchanged code).

    Unrelated changes make it difficult to accept a pull request, since it does more than what the pull request described.

* Match existing code style
    When modifying existing code, try match the existing coding style.
    This helps to keep the code consistent and reduces noise in the pull request (e.g. by avoiding re-formatting changes), which makes it easier to review and more likely to be merged.
