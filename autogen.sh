#!/bin/sh

AUTORECONF=`which autoreconf`
if test -z $AUTORECONF; then
    echo "*** No autoreconf found, please install it ***"
    exit 1
else
    autoreconf --force --install --verbose || exit $?
    intltoolize --copy --force --automake || exit $?
fi
