FROM burningdaylight/mingw-arch:latest AS test
RUN cd /tmp
RUN wget https://raw.githubusercontent.com/vim/vim/master/src/xxd/xxd.c && make xxd && mv xxd /usr/local/bin
RUN wget -O- https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-2.0.5.tar.gz | tar zxv && wget -O- https://github.com/libsdl-org/SDL/releases/download/release-2.0.22/SDL2-2.0.22.tar.gz | tar zxv 
RUN cd SDL2-2.0.22 && \
	./configure --build=x86_64 --host=i686-w64-mingw32 --prefix=/usr/i686-w64-mingw32 && \
	make -j4 && \
	make install && \
	cd ../SDL_image-release-2.0.5 && \
	 ./configure --build=x86_64 --host=i686-w64-mingw32 --prefix=/usr/i686-w64-mingw32 && \
	make -j4 && make install && cd ..
WORKDIR /home/devel

