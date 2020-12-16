FROM ubuntu:focal-20201106
ENV DEBIAN_FRONTEND noninteractive
RUN dpkg --add-architecture i386
RUN apt update
RUN apt-get install --yes --no-install-recommends libboost-program-options-dev \
    libboost-system-dev libboost-thread-dev libboost-filesystem-dev \
    libsdl2-dev libsdl2-image-dev python3-pip gcc g++ make \
    binutils-mingw-w64-i686 g++-mingw-w64 g++-mingw-w64-i686 \
    gcc-mingw-w64-base gcc-mingw-w64-i686  mingw-w64-common mingw-w64-i686-dev \
    xxd wget ca-certificates zip dpkg-dev debhelper
RUN pip3 install pypng
RUN apt install --yes --no-install-recommends wine wine32

# build cross libraries for win32
RUN mkdir -p /opt/cross/i686-w64-mingw32

# SDL2
WORKDIR /tmp
RUN wget -q -O- https://www.libsdl.org/release/SDL2-2.0.12.tar.gz | tar zxv
WORKDIR SDL2-2.0.12
RUN ./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 && make install

# SDL2_image dependencies

# zlib
WORKDIR /tmp
RUN wget -q -O- https://zlib.net/zlib-1.2.11.tar.gz | tar zxv
WORKDIR zlib-1.2.11
RUN make -f win32/Makefile.gcc PREFIX=i686-w64-mingw32- \
    LIBRARY_PATH=/opt/cross/i686-w64-mingw32/lib/ \
    INCLUDE_PATH=/opt/cross/i686-w64-mingw32/include/ install BINARY_PATH=.

# libpng
WORKDIR /tmp
RUN wget -q -O- http://prdownloads.sourceforge.net/libpng/libpng-1.6.37.tar.gz?download | tar zxv
WORKDIR libpng-1.6.37
RUN ./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 --enable-shared=no --with-sysroot=/opt/cross/i686-w64-mingw32 LDFLAGS="-L/opt/cross/i686-w64-mingw32/lib" CPPFLAGS="-I/opt/cross/i686-w64-mingw32/include" && make install

# SDL2_image
WORKDIR /tmp
RUN wget -q -O- https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5.tar.gz | tar zxv
WORKDIR SDL2_image-2.0.5
RUN ./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 --enable-shared=no LDFLAGS="-L/opt/cross/i686-w64-mingw32/lib" CPPFLAGS="-I/opt/cross/i686-w64-mingw32/include" --with-sdl-prefix=/opt/cross/i686-w64-mingw32/ && make install

# boost
WORKDIR /tmp
RUN wget -q -O- https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.gz | tar zxv
WORKDIR boost_1_75_0
RUN ./bootstrap.sh --without-icu --prefix=/opt/cross/i686-w64-mingw
RUN echo "using gcc : : i686-w64-mingw32-g++ ;" >user-config.jam
RUN ./b2 --user-config=user-config.jam toolset=gcc target-os=windows threading=multi release address-model=32 --with-filesystem --with-thread --with-program_options --with-headers --with-system install --prefix=/opt/cross/i686-w64-mingw32

# run wine so that it creates ~/.wine stuff, this enables running wineserver -p
RUN wine dir || exit 0
WORKDIR /builder
RUN rm -rf /tmp/*
