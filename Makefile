UNAME := $(shell uname -s | tr [:upper:] [:lower:])

all:	build/v06x

.PHONY:	native-tests wine-tests clean build/v06x stripped macos

native-tests:	build/v06x 
	cd test && chmod +x runtests-native.sh && ./runtests-native.sh

wine-tests:	build-i686-w64-mingw32/v06x.exe
	cd test && chmod +x runtests-wine.sh && ./runtests-wine.sh

build/v06x:
	make -f Makefile.$(UNAME)

build-i686-w64-mingw32/v06x:
	make -f Makefile.cross-mingw

clean:
	make -f Makefile.$(UNAME) clean
	rm -f test/testresult-*.txt test/testlog-*.txt

install:	native-tests
	make -f Makefile.$(UNAME) install

deb:
	dpkg-buildpackage -rfakeroot -b

stripped:
	/usr/bin/strip build/v06x

version=$(shell build/v06x -h | head -1 | cut -f 2 -d '"')

macos:	build/v06x stripped native-tests
	# todo decide what goes into the package: scripts, shaders, README etc
	cd build && zip ../../v06x-macos-$(version).zip v06x

stripped-win:	build-i686-w64-mingw32/v06x.exe
	strip $<

windows:  stripped-win wine-tests
	cd build-i686-w64-mingw32 && zip ../../v06x-win64-$(version).zip v06x.exe
