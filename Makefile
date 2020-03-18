UNAME := $(shell uname -s | tr [:upper:] [:lower:])

all:	build/v06x

.PHONY:	native-tests wine-tests clean build/v06x

native-tests:	build/v06x 
	cd test && chmod +x runtests-native.sh && ./runtests-native.sh

wine-tests:	build-i686-w64-mingw32/v06x
	cd test && chmod +x runtests-wine.sh && ./runtests-wine.sh

build/v06x:
	make -f Makefile.$(UNAME)

build-i686-w64-mingw32/v06x:
	make -f Makefile.cross-mingw

clean:
	make -f Makefile.$(UNAME) clean
	rm -f test/testresult-*.txt test/testlog-*.txt

install:	build/v06x
	make -f Makefile.$(UNAME) install

deb:	native-tests
	dpkg-buildpackage -rfakeroot -b

