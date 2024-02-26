#!/bin/bash
# Build for CI.
# Written by Reuben Thomas 2021-2024.
# This file is in the public domain.

set -e

./bootstrap --skip-po
if [[ "$ASAN" == "yes" ]]; then
    CONFIGURE_ARGS+=(CFLAGS="-g3 -fsanitize=address -fsanitize=undefined" CXXFLAGS="-g3 -fsanitize=address -fsanitize=undefined" LDFLAGS="-fsanitize=address -fsanitize=undefined")
fi
./configure "${CONFIGURE_ARGS[@]}"
make V=1
make distcheck || ( cat ./plptools-*/_build/sub/tests/test-suite.log; exit 1 )
