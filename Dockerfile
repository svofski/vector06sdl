FROM ubuntu
ENV DEBIAN_FRONTEND noninteractive
RUN apt update
RUN apt-get install --yes --no-install-recommends libboost-program-options-dev libboost-system-dev libboost-thread-dev libboost-filesystem-dev
RUN apt-get install --yes --no-install-recommends libsdl2-dev libsdl2-image-dev python3-pip
RUN apt-get install --yes --no-install-recommends gcc g++ make
RUN pip3 install pypng
RUN apt-get install --yes --no-install-recommends binutils-mingw-w64-i686 g++-mingw-w64 g++-mingw-w64-i686 gcc-mingw-w64-base gcc-mingw-w64-i686  mingw-w64-common mingw-w64-i686-dev
RUN apt install xxd
RUN apt install --yes --no-install-recommends wget

# add i386 late to avoid installing everything in i386 too
RUN dpkg --add-architecture i386
RUN apt update
RUN apt install --yes --no-install-recommends wine wine32

# build cross tools
RUN mkdir -p /opt/cross/i686-w64-mingw32

# SDL2
WORKDIR /tmp
RUN wget --no-clobber https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
RUN tar zxvf SDL2-2.0.12.tar.gz
WORKDIR SDL2-2.0.12
RUN ./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64
RUN make install

# SDL2_image dependencies

# zlib
WORKDIR /tmp
RUN wget --no-clobber https://zlib.net/zlib-1.2.11.tar.gz
RUN tar zxvf zlib-1.2.11.tar.gz
WORKDIR zlib-1.2.11
RUN make -f win32/Makefile.gcc PREFIX=i686-w64-mingw32- \
    LIBRARY_PATH=/opt/cross/i686-w64-mingw32/lib/ \
    INCLUDE_PATH=/opt/cross/i686-w64-mingw32/include/ install BINARY_PATH=.

# libpng
WORKDIR /tmp
RUN wget --no-clobber http://prdownloads.sourceforge.net/libpng/libpng-1.6.37.tar.gz?download -O libpng-1.6.37.tar.gz
RUN tar zxvf libpng-1.6.37.tar.gz
WORKDIR libpng-1.6.37
RUN ./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 --enable-shared=no --with-sysroot=/opt/cross/i686-w64-mingw32 LDFLAGS="-L/opt/cross/i686-w64-mingw32/lib" CPPFLAGS="-I/opt/cross/i686-w64-mingw32/include"
RUN make install

# SDL2_image
WORKDIR /tmp
RUN wget --no-clobber https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5.tar.gz
RUN tar zxvf SDL2_image-2.0.5.tar.gz
WORKDIR SDL2_image-2.0.5
RUN ./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 --enable-shared=no LDFLAGS="-L/opt/cross/i686-w64-mingw32/lib" CPPFLAGS="-I/opt/cross/i686-w64-mingw32/include" --with-sdl-prefix=/opt/cross/i686-w64-mingw32/
RUN make install

# boost
WORKDIR /tmp
RUN wget --no-clobber https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.gz
RUN tar zxvf boost_1_75_0.tar.gz
WORKDIR boost_1_75_0
RUN ./bootstrap.sh --without-icu --prefix=/opt/cross/i686-w64-mingw
RUN echo "using gcc : : i686-w64-mingw32-g++ ;" >user-config.jam
RUN ./b2 --user-config=user-config.jam toolset=gcc target-os=windows threading=multi release address-model=32 --with-filesystem --with-thread --with-program_options --with-headers --with-system install --prefix=/opt/cross/i686-w64-mingw32

WORKDIR /tmp
RUN rm -rf ./*
RUN apt install --yes zip

# run wine so that it creates ~/.wine stuff, this enables running wineserver -p
RUN wine dir || exit 0
RUN apt install --yes --no-install-recommends dpkg-dev debhelper
WORKDIR /builder
