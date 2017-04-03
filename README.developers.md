Subtrees
========
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

Coverity
========
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

    #From the VTR root
    mkdir -p build
    cd build
    CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=debug ..

Note that we explicitly asked for gcc and g++, the coverity build tool defaults to these compilers, and may not like the default 'cc' or 'c++' (even if they are linked to gcc/g++).

3. Run the coverity build tool
    
    #From the build directory where we ran cmake
    cov-build --dir cov-int make -j8

4. Archive the output directory

    tar -czvf cov-int vtr_coverity.tar.gz

5. Submit the archive through the coverity web interface

Once the build has been analyzed you can browse the latest results throught the coverity web interface
