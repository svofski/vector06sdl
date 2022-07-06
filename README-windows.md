SDL2: 

./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64
make install

libz/zlib:
https://zlib.net/zlib-1.2.11.tar.gz
tar zxvf zlib-1.2.11.tar.gz && cd zlib-1.2.11
make -f win32/Makefile.gcc PREFIX=i686-w64-mingw32- LIBRARY_PATH=/opt/cross/i686-w64-mingw32/lib/ INCLUDE_PATH=/opt/cross/i686-w64-mingw32/include/ install BINARY_PATH=.

libpng:
wget http://prdownloads.sourceforge.net/libpng/libpng-1.6.37.tar.gz?download -O libpng-1.6.37.tar.gz
cd libpng-1.6.37
./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 --enable-shared=no --with-sysroot=/opt/cross/i686-w64-mingw32 LDFLAGS="-L/opt/cross/i686-w64-mingw32/lib" CPPFLAGS="-I/opt/cross/i686-w64-mingw32/include"
make install

SDL2_image:
wget https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5.tar.gz
cd SDL2_image-2.0.5
./configure --prefix=/opt/cross/i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64 --enable-shared=no LDFLAGS="-L/opt/cross/i686-w64-mingw32/lib" CPPFLAGS="-I/opt/cross/i686-w64-mingw32/include" --with-sdl-prefix=/opt/cross/i686-w64-mingw32/
make install

boost:
echo "using gcc : : i686-w64-mingw32-g++ ;" >user-config.jam
./b2 --user-config=user-config.jam toolset=gcc target-os=windows threading=multi release address-model=32 --with-filesystem --with-thread --with-program_options --with-headers --with-system install --prefix=/opt/cross/i686-w64-mingw32
