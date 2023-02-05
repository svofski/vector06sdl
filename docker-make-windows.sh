# useful targets:
#   windows-clean unstripped-win stripped-win
TARGET="${1:-windows}"
docker run --rm -it -v$(pwd):/home/devel --user "$(id -u):$(id -g)" -e BOOST_STATIC=1 -e MT= -e LIBROOT=/usr/i686-w64-mingw32 svo/v06x make -j $TARGET
