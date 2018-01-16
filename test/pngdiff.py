#!/usr/bin/env python

import png
from sys import argv,exit,exc_info

expected,got = argv[1:]

msg = "comparing %s to %s " %(expected, got)

try:
    expdata = png.Reader(expected).read()
    gotdata = png.Reader(got).read()

    exp_list = list(expdata[2])
    got_list = list(gotdata[2])

    errline = -1
    for row in xrange(len(exp_list)):
        if cmp(exp_list[row], got_list[row]) != 0:
            errline = row
            break
except:
    print "When " + msg
    print "Unexpected error:", exc_info()[0]
    exit(1)

if errline == -1:
    print msg + "\033[46;30m PASS \033[0m";
    exit(0)
else:
    print msg + "\033[41;97m ERROR in row " + repr(row) + " \033[0m";
    exit(-errline)
