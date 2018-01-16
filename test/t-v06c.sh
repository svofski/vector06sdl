#!/usr/bin/env bash

emu=../build/v06x

rompath=../testroms

mkdir -p out

function announce {
    echo -e "\e[0;35m $1 \e[0m" |tee -a testlog.txt
}

function test_boot_cas {
    cmd="$emu --max-frame 42 --save-frame 42 --nofdc --novideo --nosound --bootpalette"
    announce "$cmd"
    $cmd >> testlog.txt
    ./pngdiff.py expected/$2 out/$3 | tee -a testresult.txt
}

function test_boot_fdc {
    cmd="$emu --max-frame 43 --save-frame 43 --novideo --nosound --bootpalette"
    announce "$cmd"
    $cmd >> testlog.txt
    ./pngdiff.py expected/$2 out/$3 | tee -a testresult.txt
}

function test60 {
    cmd="$emu --rom $rompath/$1 --max-frame 60 --save-frame 60 --nofdc --novideo --nosound --bootpalette"
    announce "$cmd"
    $cmd >> testlog.txt
    ./pngdiff.py expected/$2 out/$3 | tee -a testresult.txt
}

function test1600 {
    cmd="$emu --rom $rompath/$1 --max-frame 1600 --save-frame 1600 --novideo --nosound --bootpalette"
    announce "$cmd"
    $cmd >> testlog.txt
    ./pngdiff.py expected/$2 out/$3 | tee -a testresult.txt
}

function test1800 {
    cmd="$emu --rom $rompath/$1 --max-frame 1800 --save-frame 1800 --novideo --nosound --bootpalette"
    announce "$cmd"
    $cmd >> testlog.txt
    ./pngdiff.py expected/$2 out/$3 | tee -a testresult.txt
}


function scrltest {
    cmd="$emu --rom $rompath/scrltst2.rom --max-frame 120 \
        --save-frame 40 --save-frame 98 --save-frame 101 --save-frame 110 \
        --save-frame 113 --novideo --nosound --bootpalette"
    announce "$cmd"
    $cmd >> testlog.txt
    (
    ./pngdiff.py expected/scrltest_40.png  out/scrltest_40.png
    ./pngdiff.py expected/scrltest_98.png  out/scrltest_98.png  
    ./pngdiff.py expected/scrltest_101.png out/scrltest_101.png 
    ./pngdiff.py expected/scrltest_110.png out/scrltest_110.png 
    ./pngdiff.py expected/scrltest_113.png out/scrltest_113.png 
    ) | tee -a testresult.txt
}

function testfdd {
    cmd="$emu --autostart --fdd $rompath/$1 --max-frame $2 --save-frame $2 \
        --novideo --nosound"
    announce "$cmd"
    $cmd >> testlog.txt
    ./pngdiff.py expected/$3 out/$4 | tee -a testresult.txt
}

echo>testlog.txt
echo>testresult.txt

make -j4 -C ../build | tee testlog.txt 2>&1

../build/tests | tee -a testresult.txt

test_boot_cas xoxoxo boot-cas.png boots_42.png 
test_boot_fdc xoxoxo boot-fdd.png boots_43.png 
test60 bord2.rom        bord2.rom.png       bord2_60.png
test60 brdtestx.rom     brdtestx.rom.png     brdtestx_60.png
test60 chkvi53.rom      chkvi53.rom.png     chkvi53_60.png
test60 i8253.rom        i8253.rom.png       i8253_60.png
test60 i82531.rom       i82531.rom.png      i82531_60.png
test60 i82532.rom       i82532.rom.png      i82532_60.png
test60 i8253_bcd.rom    i8253_bcd.rom.png   i8253_bcd_60.png
test60 tst8253.rom      tst8253.rom.png     tst8253_60.png
test1600 testtp.rom     testtp.rom.png      testtp_1600.png
testfdd test.fdd 318 test_318.png test_318.png
test1800 vst.rom        vst_1800.png        vst_1800.png


