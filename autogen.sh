#!/bin/sh
# Run this to generate all the initial makefiles, etc.
mkdir -p m4
aclocal --install -I m4 || exit 1
autoreconf --force --install -Wno-portability || exit 1
