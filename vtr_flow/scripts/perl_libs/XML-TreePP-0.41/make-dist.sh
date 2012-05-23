#!/bin/sh

die () {
    echo "$*" >&2
    exit 1
}
doit () {
    echo "\$ $*" >&2
    $* || die "[ERROR:$?]"
}

rdf=t/example/index.rdf
doit wget -O $rdf~ http://www.kawa.net/rss/index-e.rdf
diff $rdf $rdf~ > /dev/null || doit /bin/mv -f $rdf~ $rdf
/bin/rm -f $rdf~

[ -f Makefile ] && doit make clean
[ -f META.yml ] || doit touch META.yml

egrep -v '^(lib/.*\.pm|t/.*\.t)$' MANIFEST > MANIFEST~
ls Makefile.PL README Changes MANIFEST META.yml COPYING >> MANIFEST~ 2> /dev/null
find lib -type f -name '*.pm' >> MANIFEST~
ls t/*.t >> MANIFEST~
LC_ALL=C sort MANIFEST~ | uniq > MANIFEST~~
/bin/mv -f MANIFEST~~ MANIFEST~
diff MANIFEST MANIFEST~ > /dev/null || doit /bin/mv -f MANIFEST~ MANIFEST
/bin/rm -f MANIFEST~

doit perl Makefile.PL
doit make metafile
newmeta=`ls -t */META.yml | head -1`
diff META.yml $newmeta > /dev/null || doit /bin/cp -f $newmeta META.yml

doit make disttest

name=`grep '^name:' META.yml | sed 's#^.*: *##; s#-#/#g;'`
main=`grep "$name.pm$" < MANIFEST | head -1`
[ "$main" == "" ] && die "main module is not found in MANIFEST"
doit pod2text $main > README~
diff README README~ > /dev/null || doit /bin/mv -f README~ README
/bin/rm -f README~

doit make dist
[ -d blib ] && doit /bin/rm -fr blib
[ -f pm_to_blib ] && doit /bin/rm -f pm_to_blib
[ -f Makefile.old ] && doit /bin/rm -f Makefile.old

ls -lt *.tar.gz | head -1
