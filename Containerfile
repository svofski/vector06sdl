#FROM --platform=linux/386 docker.io/multiarch/alpine:3.18.3 AS test
#FROM --platform=linux/386 docker.io/multiarch/alpine:3.18.3 AS test
FROM docker.io/multiarch/alpine:i386-v3.11 AS test
RUN apk add mingw-w64-gcc g++ py3-pip xxd make bash wine zip
RUN pip3 install pypng
#RUN apk add py3-pypng
RUN adduser -D devel

# boost
RUN mkdir -p /tmp/build-boost
WORKDIR /tmp/build-boost
COPY ./extrafiles/boost_1_79_0.tar.bz2 .
RUN tar jxf ./boost_1_79_0.tar.bz2 
#RUN wget -q -O- https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2 | tar jx
WORKDIR /tmp/build-boost/boost_1_79_0 
# only build filesystem, chrono, program_options, thread, system
RUN echo "using gcc : : i686-w64-mingw32-g++ ;" > user-config.jam
RUN ./bootstrap.sh
RUN ./b2 --user-config=./user-config.jam --prefix=/usr/i686-w64-mingw32/ target-os=windows address-model=32 variant=release  --with-filesystem --with-chrono --with-program_options --with-thread --with-system install
#        ;\
#	exit 0 # boost returns some build error even though it's all fine

# SDL2
WORKDIR /tmp
#RUN wget -q -O- https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-2.0.5.tar.gz | tar zxv && wget -q -O- https://github.com/libsdl-org/SDL/releases/download/release-2.0.22/SDL2-2.0.22.tar.gz | tar zxv 
RUN wget -q -O- https://github.com/libsdl-org/SDL_image/releases/download/release-2.6.3/SDL2_image-2.6.3.tar.gz | tar zxv && wget -q -O- https://github.com/libsdl-org/SDL/releases/download/release-2.28.3/SDL2-2.28.3.tar.gz | tar zxv 

RUN cd SDL2-2.28.3 && \
	./configure --build=x86_64 --host=i686-w64-mingw32 --prefix=/usr/i686-w64-mingw32 && \
	make -j && \
	make install
RUN cd SDL2_image-2.6.3 && \
	 ./configure --build=x86_64 --host=i686-w64-mingw32 --prefix=/usr/i686-w64-mingw32 --with-sdl-prefix=/usr/i686-w64-mingw32 && \
	make -j && make install && cd ..

WORKDIR /home/devel
#ENTRYPOINT /bin/bash

