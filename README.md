# Cross-platform emulator of Вектор-06ц in C++
## Кросс-платформенный эмулятор Вектора-06ц на C++

Verified platforms:
 * Raspberry Pi 3
 * Linux amd64
 * Windows 10
 * macOS 10.13.1 High Sierra
  

## Build instructions

## Raspberry Pi 3 and Linux in general

Assuming you already have a proper build environment, you may also need need to install additionally
```cmake```, ```libboost1.62-dev```.

I run vector06sdl in fullscreen and I compiled SDL2 on RPi3 from source like so:
```
../configure --host=armv7l-raspberry-linux-gnueabihf --disable-pulseaudio --disable-esd --disable-video-mir --disable-video-wayland --disable-video-x11 --disable-video-opengl
make install
```
It is possible that the default SDL2 provided with Raspbian is perfectly fine too.

```libsdl2_image``` is recommended but is only used to save PNG images of frames in the tests.

### macOS

You need xcode command-line tools and ports tree or Homebrew installed. I prefer Homebrew. Set up instructions are at https://brew.sh/

You'll need ```cmake```, ```sdl2```. The tests also use ```sdl2_image```, Python 2.7, ```pypng```.

### Windows

I like nuwen.net MinGW distro: https://nuwen.net/mingw.html It already has libboost and SDL2 pre-packaged, which is a huge time saver. Alternatively, it should not be a problem to build using the toolchain supplied with Code::Blocks. 

SDL2_Image is not a part of this distro. You can get one pre-built from https://www.libsdl.org/projects/SDL_image 
Specify the path to SDL_image in SDL2IMAGEDIR environment variable before running cmake.

Open the environment using C:\MinGW\open_distro_window.bat. Now it's almost as on a real system, but you need to specify a magic parameter to cmake:
```
git clone --depth=1 https://github.com/svofski/vector06sdl
cd vector06sdl
mkdir build && cd build
set SDL2IMAGEDIR=<fully-qualified path to SDL2_image-2.0.3\x86_64-w64-mingw32>
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j4
```


## Building (all platforms)
```
git clone --depth=1 https://github.com/svofski/vector06sdl
cd vector06sdl
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

## Running tests (all platforms)
``` 
cd ../test
./t-v06c.sh
```

# Raspberry Pi 3 specific

Raspberry Pi supports PAL 50Hz 288p 'fake progressive' screen mode, which works fantastic in conjunction 
with a CRT TV or monitor. To use it, plug a CRT TV to the composite output, obviously. Then edit /boot/config.txt and set ```sdtv_mode=18```
(that's mode 2 + 16 for progessive scan).
Your text console will probably look grumpy after doing so, you can make the text readable by issuing 
```fbset -yres 288``` in the console prompt. This has no effect on the emulator itself.

This is the closest to the real thing you can ever get.

# Profiling

Example 1-minute run:
```
LD_PRELOAD=/usr/lib/libprofiler.so CPUPROFILE=v06x.prof ./v06x --rom testtp.rom --nosound --vsync  & sleep 60 && kill %1
```

For some reason, Linux distros known to me insist on packaging some prehistoric version of pprof which is not fantastic. To examine profile data, install golang (```apt-get install golang```), set up ```$GOPATH``` and install proper up to date pprof:
```go get github.com/google/pprof```

Now to interactively examine profiler data:
```
$GOPATH/bin/pprof -http=0.0.0.0:9999 ./v06x ./v06x.prof
```
