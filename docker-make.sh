docker run --rm -it -v/home/svo/projects/vector06sdl:/home/devel -e BOOST_STATIC=1 -e MT= -e LIBROOT=/usr/i686-w64-mingw32 svo/v06x make -f Makefile.cross-mingw -j4
