#!/bin/sh
# Run this to generate all the initial makefiles, etc.
aclocal --install || exit 1
autoreconf --force --install -Wno-portability || exit 1
