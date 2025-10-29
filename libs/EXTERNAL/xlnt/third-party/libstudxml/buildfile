# file      : buildfile
# license   : MIT; see accompanying LICENSE file

./: {*/ -build/} doc{INSTALL NEWS README} legal{LICENSE} manifest

# Don't install examples, tests or the INSTALL file.
#
examples/:       install = false
tests/:          install = false
doc{INSTALL}@./: install = false
