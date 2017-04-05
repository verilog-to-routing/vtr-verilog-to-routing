Commit Proceedures
==================

For devlopers with commit rights on GitHub
------------------------------------------
The guiding principle in internal development is to submit your work into the repository without breaking other people's work.
When you commit, make sure that the repository compiles, that the flow runs, and that you did not clobber someone else's work.
In the event that you are responsible for "breaking the build", fix the build at top priority.

We have some guidelines in place to help catch most of these problems:

1.  Before you push code to the central repository, your code MUST pass the check-in regression test.  
    The check-in regression test is a quick way to test if any part of the Verilog-to-routing flow is broken.
    To run the check-in regression test run:
```shell
#From the VTR root directory
$ ./run_quick_test.pl
```
You may push if all the tests return `[PASS]`.
It is typically a good idea to run tests regularily as you make changes.

2.  The automated [BuildBot](http://builds.verilogtorouting.org:8080/waterfall) will perform more extensive regressions tests and mark which revisions are stable.

3.  Everyone who is doing development must write regression tests for major feature they create.
    This ensures regression testing will detect if a feature is broken by someone (or yourself).

4.  In the event a regression test is broken, the one responsible for having the test pass is in charge of determining:
    * If there is a bug in the source code, in which case the source code needs to be updated to fix the bug, or 
    * If there is a problem with the test (perhaps the quality of the tool did in fact get better or perhaps there is a bug with the test itself), in which case the test needs to be updated to reflect the new changes.  
    
    If the golden results need to be updated and you are sure that the new golden results are better, use the command `../scripts/parse_vtr_task.pl -create_golden your_regression_test_name_here`

5.  Keep in sync with the GitHub master branch as regularly as you can (i.e. `git pull` or `git pull --rebase`).
    The longer code deviates from the trunk, the more painful it is to integrate back into the trunk.

Whatever system that we come up with will not be foolproof so be conscientious about how your changes will effect other developers.

For external developers
-----------------------
VTR welcomes external contributions. 

The general proceedure is to file an issue on GitHub propossing your changes, and then create an associated pull request.

Here are some general guidelines:

* Keep your pull requests focused and specific.
  Short pull requests are much easier to review (and hence likely to be merged).
  Large scale changes which touch a large amount of code are difficult to review, and should be discussed (e.g. in a GitHub issue) before hand.

* If you are adding user-facing features, make sure the documentation has been updated

* If you are adding a new feature, ensure you have added tests for the feature

* Ensure that all tests (new and existing) pass

External Libraries Using Subtrees
=================================
Some libraries used by VTR are developed in other repositories and integrated using git subtrees.

Currently these includes:
* libsdcparse       [from git@github.com:kmurray/libsdcparse.git]
* libblifparse      [from git@github.com:kmurray/libblifparse.git]
* libtatum          [from git@github.com:kmurray/tatum.git]

As an example consider libsdcparse:

1. To begin add the sub-project as a remote:

    ` git remote add -f github-libsdcparse git@github.com:kmurray/libsdcparse.git `

2. To initially add the library (first time the library is added only):

    ` git subtree add --prefix libsdcparse github-libsdcparse master --squash `

    Note the '--squash' option which prevents the whole up-stream history from being merged into the current repository.

3. To update a library (subsequently):

    ` git subtree pull --prefix libsdcparse github-libsdcparse master --squash `

    Note the '--squash' option which prevents the whole up-stream history from being merged into the current repository.

    If you have made local changes to the subtree and pushed them to the parent repository, you may encounter a merge conflict
    during the above pull (caused by divergent commit IDs between the main and parent repositories).
    You will need to re-split the subtree so that subtree pull finds the correct common ancestor:

    ` git subtree split --rejoin -P libsdcparse/ `

    You can then re-try the subtree pull as described above.

4. [This is unusual, be sure you really mean to do this!] To push local changes to the upstream subtree

    ` git subtree push --prefix libsdcparse github-libsdcparse master `

For more details see [here](https://blogs.atlassian.com/2013/05/alternatives-to-git-submodule-git-subtree/) for a good overview.

Finding Bugs with Coverity
==========================
[Coverity Scan](https://scan.coverity.com) is a static code analysis service which can be used to detect bugs.

Browsing Defects
----------------
To view defects detected do the following:

1. Get a coverity scan account

    Contact a project maintainer for an invitation.

2. Browse the existing defects through the coverity web interface


Submitting a build
------------------
To submit a build to coverity do the following:

1. [Download](https://scan.coverity.com/download) the coverity build tool

2. Configure VTR to perform a *debug* build. This ensures that all assertions are enabled, without assertions coverity may report bugs that are gaurded against by assertions.

    ```shell
    #From the VTR root
    mkdir -p build
    cd build
    CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=debug ..
    ```

Note that we explicitly asked for gcc and g++, the coverity build tool defaults to these compilers, and may not like the default 'cc' or 'c++' (even if they are linked to gcc/g++).

3. Run the coverity build tool
    
    ```shell
    #From the build directory where we ran cmake
    cov-build --dir cov-int make -j8
    ```

4. Archive the output directory

    ```shell
    tar -czvf cov-int vtr_coverity.tar.gz
    ```

5. Submit the archive through the coverity web interface

Once the build has been analyzed you can browse the latest results throught the coverity web interface
