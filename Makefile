all:	native-tests

.PHONY:	native-tests wine-tests clean

native-tests:	build/v06x 
	cd test && chmod +x runtests-native.sh && ./runtests-native.sh

wine-tests:	build-i686-w64-mingw32/v06x
	cd test && chmod +x runtests-wine.sh && ./runtests-wine.sh

build/v06x:
	make -f Makefile.linux

build-i686-w64-mingw32/v06x:
	make -f Makefile.cross-mingw

clean:
	make -f Makefile.linux clean
	rm -f test/testresult-*.txt test/testlog-*.txt

install:	build/v06x
	make -f Makefile.linux install

deb:
	dpkg-buildpackage -rfakeroot -b

