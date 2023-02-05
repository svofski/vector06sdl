UNAME := $(shell uname -s | tr [:upper:] [:lower:])

ifeq ($(UNAME),linux)
    UNAME := $(UNAME)-$(shell arch)
endif

WINBUILD := build-i686-w64-mingw32
WINDIST := $(WINBUILD)/v06x
PACKAGEDIR := packages

all:	build/v06x

.PHONY:	native-tests wine-tests clean build/v06x stripped macos

native-tests:	build/v06x
	cd test && chmod +x runtests-native.sh && ./runtests-native.sh

wine-tests:	$(WINBUILD)/v06x.exe $(WINBUILD)/tests.exe
	cd test && chmod +x runtests-wine.sh && ./runtests-wine.sh

command-tests:	$(WINBUILD)/v06x.exe
	cd test && chmod +x runtests-command.sh && ./runtests-command.sh

build/v06x:
	make -f Makefile.$(UNAME)

$(WINBUILD):
	mkdir $(WINBUILD)

$(WINBUILD)/v06x.exe:
	make -f Makefile.cross-mingw

clean:
	make -f Makefile.cross-mingw clean
	make -f Makefile.darwin clean
	make -f Makefile.linux-x86_64 clean
	rm -f test/testresult-*.txt test/testlog-*.txt

install:	native-tests
	make -f Makefile.$(UNAME) install

deb:
	dpkg-buildpackage -rfakeroot -b
	mkdir -p $(PACKAGEDIR)
	mv ../*.deb $(PACKAGEDIR)

stripped:
	/usr/bin/strip build/v06x

version=$(shell build/v06x -h | head -1 | cut -f 2 -d '"')
winversion=$(shell wine $(WINBUILD)/v06x.exe -h | head -1 | cut -f 2 -d '"')

macos:	build/v06x stripped native-tests
	# todo decide what goes into the package: scripts, shaders, README etc
	cd build && zip ../../v06x-macos-$(version).zip v06x

unstripped-win:	$(WINDIST).exe

stripped-win:	$(WINDIST).exe
	strip $<

windows:  stripped-win wine-tests
	mkdir -p $(WINDIST)/bin $(WINDIST)/scripts $(WINDIST)/shaders $(WINDIST)/boot
	cp $(WINBUILD)/v06x.exe $(WINDIST)/bin
	cp boot/* $(WINDIST)/boot
	cp shaders/* $(WINDIST)/shaders
	cp winbat/* $(WINDIST)/bin
	cp scripts/*.chai $(WINDIST)/scripts
	cp scripts/README.md $(WINDIST)/scripts
	cd $(WINDIST)/.. && zip -r v06x_$(winversion)-win64.zip v06x

windows-clean:
	rm -rf $(WINBUILD)
