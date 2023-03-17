#!/usr/bin/env python3
import png
import sys
import os
from subprocess import call, Popen, PIPE
from math import *
from operator import itemgetter
from utils import *
from base64 import b64encode
import array

VARCHUNK=2
LEFTOFS=4
mode='varblit'

# if cell is empty and all cells in this column above it are empty
#       check cell directly below and if it is not empty consider current one not empty as well
#
# if cell is empty and all cells in this column under it are empty
#       check cell directly above and if it is not empty, consider current one not empty as well
#   
# step 1: shrinkwrap
# step 2: profit



def shrinkwrap(plane, ncolumns):
    rows = list(chunker(plane, ncolumns))
    hull = [[1 for x in range(len(rows[0]))] for y in range(len(rows))]

    for x in range(len(rows[0])):
        y1 = 0
        y2 = len(rows) - 1
        top_end = bottom_end = False
        while y1 < y2 and not (top_end and bottom_end):
            if rows[y1][x] == 0 and rows[y1+1][x] == 0 and rows[y1+2][x] == 0:
                hull[y1][x] = 0
                y1 += 1
            else:
                top_end = True
            if rows[y2][x] == 0 and rows[y2-1][x] == 0 and rows[y2-2][x] == 0:
                hull[y2][x] = 0
                y2 -= 1
            else:
                bottom_end = True
    #for h in hull:
    #    print(h)
    return hull



# use precalculated offset instead of number of 2-col chunks
def varformat(plane, ncolumns, hull):
    rows = list(chunker(plane, ncolumns))
    #print(hull)

    for y in range(len(rows)):
        #print(line)
        line = rows[y]
        first = 0
        while first < len(line) and hull[y][first] == 0:
            first = first + 1
        last = len(line) - 1
        while last > first and hull[y][last] == 0:
            last = last - 1
        columns = list(chunker(line[first:last+1], VARCHUNK))
        end = len(columns)

        jump = (16 - end) * 5
        dbline = '.db %d, %d ' % (first + LEFTOFS, jump)
        #dbline = '.db %d, %d ' % (first, end)
        for c in columns[:end]:
            for i in range(VARCHUNK - len(c)):
                c.append(0)
            dbline = dbline + ',' + ','.join(['$%02x' % x for x in c])
        print(dbline)
    print('.db 255, 255 ; end of plane data')

def readPNG(filename):
    reader = None
    pix = None
    w, h = -1, -1
    try:
        if reader == None:        
            reader=png.Reader(filename)
        img = reader.read()
        #print('img=', repr(img))
        pix = list(img[2])
    except:
        print(f'; Could not open image {filename}, file exists?')
        return None
    w, h = len(pix[0]), len(pix)
    print ('; Opened image %s %dx%d' % (filename, w, h))            
    return pix

foreground=''
glitchframe=''
lineskip=2
nplanes=1


lut = [0] * 256 # for grayscale input
lut[255] = 1    # if grayscale b&w
lut[1] = 1      # for indexed 1bpp
lut[2] = 2
lut[3] = 3

labels=['', '', '', '']

try:
    i = 1
    while i < len(sys.argv):
        if sys.argv[i][0] == '-':
            if sys.argv[i] == '-lineskip':
                lineskip = int(sys.argv[i+1])
                i += 1
            if sys.argv[i] == '-nplanes':
                nplanes = int(sys.argv[i+1])
                i += 1
            if sys.argv[i] == '-lut':
                userlut = [int(x) for x in sys.argv[i+1].split(',')]
                for n,c in enumerate(userlut):
                    lut[n] = c
                i += 1
            if sys.argv[i] == '-leftofs':
                LEFTOFS = int(sys.argv[i+1])
                i += 1
            if sys.argv[i] == '-labels':
                userlabels = [s for s in sys.argv[i+1].split(',')]
                for n, l in enumerate(userlabels):
                    labels[n] = l
                i += 1
            if sys.argv[i] == '-mode':
                mode = sys.argv[i+1]
                i += 1
        elif foreground == '':
            foreground = sys.argv[i]
        elif glitchframe == '':
            glitchframe = sys.argv[i]
        i += 1
except:
    sys.stderr.write('failed to parse args\n')
    exit(1)

(origname, ext) = os.path.splitext(foreground)
pic = readPNG(foreground)

ncolumns = len(pic[0])//8

# indexed color, lines of bytes
#print('xbytes=', xbytes, ' nlines=', nlines)

def starprint(pic):
    print('; ', ''.join([chr(ord('0') + x) if x > 0 else ' ' for x in pic]))


def pixels_to_bitplanes(pic, lineskip):
    planes = [[], [], [], []]
    nlines = len(pic)
    for y in range(0, nlines, lineskip):
        luc = [lut[c] for c in pic[y]]
        starprint(luc)
        for col in chunker(luc, 8):
            c1 = sum([(c & 1) << (7-i) for i, c in enumerate(col)])
            planes[0].append(c1)
            c2 = sum([((c & 2) >> 1) << (7-i) for i, c in enumerate(col)])
            planes[1].append(c2)
            c3 = sum([((c & 4) >> 1) << (7-i) for i, c in enumerate(col)])
            planes[2].append(c2)
            c4 = sum([((c & 8) >> 1) << (7-i) for i, c in enumerate(col)])
            planes[3].append(c2)
    return planes



if mode == 'varblit':
    # every line:   [offset of first column]
    #               [offset into blit code]
    #               data
    #               $ff, $ff terminates
    planes = pixels_to_bitplanes(pic, lineskip=lineskip)

    pic2 = None
    if glitchframe != '':
        pic2 = readPNG(glitchframe)

    for i in range(nplanes):
        hull = None
        if pic2 != None:
            # if a second frame is specified, create a common hull for 2 frames
            #glitchframe=sys.argv[2]
            pic2 = readPNG(glitchframe)
            print(f'; Using {glitchframe} to calculate common hull')

            glitchplanes = pixels_to_bitplanes(pic2, lineskip=lineskip)

            orplane = [x or y for x, y in zip(planes[i], glitchplanes[i])]
            # for line in chunker(orplane, ncolumns):
            #     starprint(line)

            hull = shrinkwrap(orplane, ncolumns)

        if hull == None:
            # use single hull
            hull = shrinkwrap(planes[i], ncolumns)

        if labels[i] != '':
            print(f'{labels[i]}:')
        varformat(planes[i], ncolumns, hull)
            
elif mode == 'tex2':
    # pre-shifted unwrapped texture in 2-pixel chunks
    # pixel 0:   11000000
    # pixel 1:   00110000
    # pixel 2:   00001100
    # pixel 3:   00000011

    if labels[0] != '':
        print(f'{labels[0]}:')
    #masks = [0xc0, 0x30, 0x0c, 0x03]
    masks = [0xff, 0xff, 0xff, 0xff]
    for line in pic:
        tex = [masks[i % len(masks)] * pixel for i, pixel in enumerate(line)]
        print('\t.db ', ','.join(['$%02x'%x for x in tex]))
        print('\t.db ', ','.join(['$%02x'%x for x in tex]))

elif mode == 'bits8':
    # simple 1bpp bitplane
    data = pixels_to_bitplanes(pic, lineskip=lineskip)

    if labels[0] != '':
        print(f'{labels[0]}:')
    for line in chunker(data[0], 32):
        print('\t.db ', ','.join(['$%02x'%x for x in line]))

    if nplanes > 1:
        if labels[1] != '':
            print(f'{labels[1]}:')
        for line in chunker(data[1], 32):
            print('\t.db ', ','.join(['$%02x'%x for x in line]))

