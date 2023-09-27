#!/bin/bash
# useful targets:
#   windows-clean unstripped-win stripped-win

TARGET="${1:-windows}"
podman run --rm -it -v$(pwd):/home/devel -e BOOST_STATIC=1 -e MT= -e LIBROOT=/usr/i686-w64-mingw32 v06x make $TARGET
