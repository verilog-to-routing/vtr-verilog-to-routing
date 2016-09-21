Subtree Libraries
==================
Some libraries (e.g. libsdcparse) are developed in other repositories and integrated using git subtrees.

As an example consider libsdcparse:

1. To begin add the sub-project as a remote:

    ` git remote add -f github-libsdcparse git@github.com:kmurray/libsdcparse.git `

2. To initially add the library (first time only):

    ` git subtree add --prefix libsdcparse github-libsdcparse master --squash `

    Note the '--squash' option which prevents the whole up-stream history from being merged into the current repository.

3. To update a library (subsequently):

    ` git subtree pull --prefix libsdcparse github-libsdcparse master --squash `

For more details see [here](https://blogs.atlassian.com/2013/05/alternatives-to-git-submodule-git-subtree/) for a good overview.
