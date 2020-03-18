# Cross-platform emulator of Вектор-06ц in C++
## Кросс-платформенный эмулятор Вектора-06ц на C++

Verified platforms:
 * Raspberry Pi 3
 * Linux amd64
 * Windows 10
 * macOS 10.13.1 High Sierra
 * Android
  

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

# Scripting
v06x can execute chai scripts (see https://chaiscript.com). The scripts can attach hooks to frame interrupt and use debugger API and probably do other things.
## Example scripts

### rk86
Example scripts that demonstrate scripting ability by implementing a 3-stage loading are provided in [scripts](../master/scripts). They facilitate loading of music files composed in rk86 music system. Normally to listen to a tune one should:
1) load rk86 monitor-emulator (micro_rk.rom)
2) in the emulator, load the rk music system (MSVec.rk)
3) in the rk music system, load a music file 
4) compile and run music file
Doing all of this in v06x is challenging not only because of the number of steps involved, but also because v06x does not support loading rk files natively. The only option would be to convert everything to wav files and issue a lot of keyboard commands right on time.

```rkload.chai``` uses debugger API to intercept rk86 monitor call that loads a byte
```musload.chai``` implements a robot typer that loads the files and types all the keys automatically

The example invokation can be found in [msvec.bat](../master/scripts/msvec.bat).

### BASIC files
A lot of programs written in BASIC 2.5 for Vector-06C are stored in .BAS files. Loading these files is facilitated by loading 3 scripts:
```bas25hook.chai```, ```robotnik.chai```, ```basload.chai```. To make boot times faster, a copy of BASIC 2.5 from TimSoft's 32kB bootrom is used. Here's an example (which can be found in [basic.bat](../master/scripts/basic.bat))
```
v06x --script bas25hook.chai --script robotnik.chai --script basload.chai --bootrom ..\boot\boot.bin --scriptargs DIAMOND.BAS
```

## Available API
The API is ad-hoc and is being added as needed. Current list of available functions (probably outdated):
  * ```scriptargs[]``` arguments passed in ---scriptargs options, array of strings
  * ```add_callback(name, fun(x) {})``` available callbacks: "frame", "wavloaded", "breakpoint". The argument is integer.
  * ```loadwav(filename)``` load a wav file and start playing, useful for multi-staged file loading 
  * ```scancode_from_name(name)``` returns SDL_Scancode by name
  * ```insert_breakpoint(type, addr, kind)``` inserts a breakpoint
    * type 1: hw breakpoint: kind = 1
    * type 2: write watchpoint, kind = number of bytes to watch
    * type 3: read watchpoint, kind = number of bytes to watch
    * type 4: access watchpoint, kind = number of bytes to watch
  * ```debugger_attached()``` attached as debugger, this pauses the execution
  * ```debugger_detached()``` debugger detach
  * ```debugger_break()``` break execution
  * ```debugger_continue()``` continue execution
  * ```read_register(name)``` return integer register value, names are "a","f","b","c","d","e","h","l","sp","pc"
  * ```set_register(name, value)``` set register value, register names as in read_register()
  * ```read_memory(addr, stack)``` return memory byte at addr, stack = 1 if stack access
  * ```write_memory(addr, w8, stack)``` write memory byte at addr, stack = 1 if stack access
  * ```read_file(filename)``` read a binary file and return contents as VectorInt()


