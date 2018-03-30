#!/bin/sh
#echo "## Got: $*"
CC="$1"
DIR="$2"
shift 2
case "$DIR" in
    "" | ".")
    $CC -MM -MG "$@" | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'
    ;;
    *)
    $CC -MM -MG "$@" | sed -e "s@^\(.*\)\.o:@$DIR/\1.d $DIR/\1.o:@"
    ;;
esac
