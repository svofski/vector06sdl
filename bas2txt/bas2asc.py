#!/usr/bin/env python3

# Copyright 2020 Viacheslav Slavinsky
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

import sys
import os

ENCODING=None # try 'utf-8' or 'cp1251'

class State:
    DUMMY0 = 0
    DUMMY1 = 1
    LINENUM0 = 2
    LINENUM1 = 3
    TOKENS = 4
    END = 5

class Mode:
    INITIAL = 0x00
    TOKENIZE = 0x01
    QUOTE = 0x20
    VERBATIM = 0x40

class Tokens:
    QUOTE='"'
    Chars = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABC' + \
        'DEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ЮАБЦДЕФГ' + \
        'ХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧ' + chr(127)
    Words = ['CLS','FOR','NEXT','DATA','INPUT','DIM','READ','CUR','GOTO',
            'RUN','IF','RESTORE','GOSUB','RETURN','REM','STOP','OUT','ON',
            'PLOT','LINE','POKE','PRINT','DEF','CONT','LIST','CLEAR',
            'CLOAD','CSAVE','NEW','TAB(','TO','SPC(','FN','THEN','NOT',
            'STEP','+','-','*','/','^','AND','OR','>','=','<','SGN','INT',
            'ABS','USR','FRE','INP','POS','SQR','RND','LOG','EXP','COS',
            'SIN','TAN','ATN','PEEK','LEN','STR$','VAL','ASC','CHR$',
            'LEFT$','RIGHT$','MID$','POINT','INKEY$','AT','&','BEEP',
            'PAUSE','VERIFY','HOME','EDIT','DELETE','MERGE','AUTO','HIMEM',
            '@','ASN','ADDR','PI','RENUM','ACS','LG','LPRINT','LLIST',
            'SCREEN','COLOR','GET','PUT','BSAVE','BLOAD','PLAY','PAINT',
            'CIRCLE']

    by_initial = {key:[] for key in "ABCDEFGHIJKLMNOPQRSTUVWXYZ*/^>=<&@+-"}
    for w in Words:
        by_initial[w[0]].append(w)

    def gettext(c):
        if c < 0x20:
            return c
        elif c < 0x80:
            return ord(Tokens.Chars[c - 0x20])
        else:
            return Tokens.Words[c - 0x80]

    def chars(text):
        result = []
        for c in text:
            index = Tokens.Chars.find(c)
            if index != -1:
                result.append(0x20 + index)
            else:
                result.append(ord(c))
        return result


def format_token(t):
    if isinstance(t,int):
        return chr(t)
    if isinstance(t, str):
        return t

def process_line(line):
    return str(line[0]) + ' ' + ''.join([format_token(x) for x in line[1:]])

def readbas(path):
    result = []
    with open(path, 'rb') as fi: 
        mv = memoryview(fi.read())

        state = State.DUMMY0 
        fin = 0
        for i in range(len(mv)):
            c = mv[i]
            if state == State.DUMMY0:
                if c == 0: 
                    fin = fin + 1
                state = State.DUMMY1
            elif state == State.DUMMY1:
                if c == 0:
                    fin = fin + 1
                if fin == 3:
                    state = State.END
                else:
                    state = State.LINENUM0
                line = []
            elif state == State.LINENUM0:
                line.append(c)
                state = State.LINENUM1
            elif state == State.LINENUM1:
                line[0] = line[0] + c * 256
                state = State.TOKENS
            elif state == State.TOKENS:
                if c == 0:
                    fin = 1
                    state = State.DUMMY0
                    result.append(process_line(line))
                elif c > 0 and c <= 31:
                    line.append(c)
                elif c <= 228:
                    line.append(Tokens.gettext(c))
                else:
                    line.append(c)
            elif state == State.END:
                break
    return result

def isnum(c):
    return c >= '0' and c <= '9'

# For given initial letter in position i, return list of keywords 
# that start with this letter.
# Each entry is ["keyword", count=0, position=i]
# If the mode is INITIAL, clear list of words and complete words
# If the mode is VERBATIM or QUOTE, return nothing
def pick_keywords(initial, i, mode, words, complete):
    #by_initial = {key:[] for key in "ABCDEFGHIJKLMNOPQRSTUVWXYZ*/^>=<&@+-"}
    #for w in Tokens.Words:
    #    by_initial[w[0]].append(w)

    try:
        if (mode & (Mode.VERBATIM|Mode.QUOTE)) != 0:
            return

        kws = [[x,0,i] for x in Tokens.by_initial[initial][:]]
        for w in kws:
            if len(w[0]) == 1:
                complete.append(w)
            else:
                words.append(w)
    except:
        if mode == Mode.INITIAL:
            words.clear()
            complete.clear()

# Parse number at the beginning of line and skip initial spaces
def get_linenumber(s):
    linenum = 0
    text = 0
    for text,c in enumerate(s):
        if isnum(c):
            linenum = linenum * 10 + int(c)
        else:
            if c == ' ':
                pass
            else:
                break
    return linenum,text

# Update word match count, move complete words to complete, delete mismatches
# See pick_keywords
def trackwords(c, words, complete):
    todelete=[]
    for j,wc in enumerate(words):   # track keywords
        k = wc[1] + 1
        if wc[0][k] == c: 
            wc[1] = k 
            if len(wc[0])-1 == k:
                complete.append(wc)
                todelete.append(j) 
        else:
            todelete.append(j)  # char mismatch, this word is out

    for j in reversed(todelete):
        del words[j]            # purge untracked words

# input: sorted by start position [[tok, x, pos]]
# output: only one token per pos, the longest
def suppress_nonmax(complete):
    pos = -1
    result = []
    for t in complete:
        if t[2] == pos:
            if len(t[0]) > len(result[-1][0]):
                result[-1] = t
        else:
            result.append(t)
            pos = t[2]
    return result

# for overlapping tokens, pick the longest even if it starts later
# IFK=ATHEN3 
#     AT
#      THEN <-- winrar
def suppress_overlaps(complete):
    current_end=-1
    current_len=-1
    result=[]
    for t in complete:
        if t[2] < current_end:
            if len(t[0]) > current_len:
                result[-1] = t
                current_len = len(t[0])
                current_end = t[2]+current_len
        else:
            result.append(t)
            current_len = len(t[0])
            current_end = t[2]+current_len
    return result

def tokenize2(s,addr):
    tokens=[]
    words=[]
    complete=[]
    linenum,i = get_linenumber(s)
    seq_start = i
    pick_keywords(s[i], i, Mode.INITIAL, words, complete)
    mode = Mode.TOKENIZE
    while i < len(s):
        trackwords(s[i], words, complete)
        breakchar = s[i]
        if breakchar == Tokens.QUOTE:
            mode ^= Mode.QUOTE

        # add keywords that start at the current position to tracking
        pick_keywords(s[i], i, mode, words, complete)

        # all tracked words ended, or end of line
        if len(words) == 0 or i + 1 == len(s):
            words = []
            if len(complete) > 0:
                # make sure that the tokens are in order of occurrence
                complete.sort(key=lambda x: x[2], reverse=False)
                complete = suppress_nonmax(complete)   # INPUT vs INP..
                complete = suppress_overlaps(complete) # THEN over AT in ATHEN 
                # flush dangling character tokens 
                tokens = tokens + Tokens.chars(s[seq_start:complete[0][2]])
                for j,b in enumerate(complete):
                    if j > 0 and b[2] != i: # tokens overlap, only keep 1st
                        break
                    tokens.append(Tokens.Words.index(b[0]) + 0x80)
                    i = b[2] + len(b[0]) 
                    seq_start = i
                    if b[0] in ['DATA','REM']:
                        mode = Mode.VERBATIM
                        break # mode switch, cancel following tokens
                if breakchar != Tokens.QUOTE:
                    i = i - 1
                complete = []
            else: 
                pass

        i = i + 1         

    tokens = tokens + Tokens.chars(s[seq_start:]) + [0]
    recsize = len(tokens)+4
    addr += recsize
    tokens = [addr&255,addr>>8] + [linenum & 255, (linenum >> 8) & 255] + tokens
    return tokens,addr

def enbas(path):
    result = []
    size = 0
    addr = 0x4301
    with open(path, 'r', encoding=ENCODING) as fi:
        for text in fi:
            #print("Input=["+text.strip("\n")+"]")
            tokens,addr=tokenize2(text.strip("\n"), addr)
            result = result + tokens

    result.append(0)
    result.append(0)

    # add padding for perfect file match
    padding = [0]*(256-(len(result) % 256))
    return bytearray(result+padding)

#print(suppress_overlaps([["AT",0,3],["TO",0,4]]))
#print(suppress_overlaps([["AT",0,3],["THEN",0,4]]))
#print(suppress_nonmax([["INP",0,3],["INPUT",0,3],["PUT",0,2]]))
#print([chr(x) for x in tokenize2('5 FORI=ATOB', 0x4301)[0]]) # AT wins
#print([chr(x) for x in tokenize2('5 IFK=ATHEN3', 0x4301)[0]]) # THEN wins
#print([chr(x) for x in tokenize2('5 INPUT"ABC",D', 0x4301)[0]])
#print([chr(x) for x in tokenize2('5 REM*LOL*', 0x4301)[0]])
#print([chr(x) for x in tokenize2('5 IFN=1', 0x4301)[0]])
#print([chr(x) for x in tokenize2('5 PRINT"ABC";K:CUR0,24', 0x4301)[0]])
#print([chr(x) for x in tokenize2('5 PLAY "A","B","C"', 0x4301)[0]])
#print([chr(x) for x in tokenize2('5 COLOR10,128,0', 0x4301)[0]])
#print([chr(x) for x in tokenize2('18 IFZ=0THENC(I,J)=14:C0=C0+1:IFC0>3THEN 17', 0x4301)[0]])
#print([chr(x) for x in tokenize2('1 ""THEN', 0x4301)[0]]) # evil yet unreal
#print([(x) for x in tokenize2('1 IFA$="+"THEN1', 0x4301)[0]])
#print([chr(x) for x in tokenize2('1 PRINT"ABC";3', 0x4301)[0]])
#print(tokenize2('40 A$=INKEY$',0x4301))
#print(tokenize2("1 THENPLOT",0x4301))
#print(tokenize2("185 IFX<0ANDY<0THENPLOTABS(X),ABS(Y),1:GOTO 187",0x4301))
#print(tokenize2("1 DATA -123",0x4301))
#print("t2=", tokenize2("1 CLS",0x4301))
#print("t2=", tokenize2("10 RESTORE6",0x4300))
#print("t2=", tokenize2("10 RESTOR6",0x4300))
#print("t2=", tokenize2("10 QTOA",0x4300))
#print("t2=", tokenize2("10 FORI=ATOB",0x4300))
#print("t2=", tokenize2("10 FORJ=0TO5",0x4300))
#print(get_linenumber("1234 PRINTA"))
#exit()

def usagi():
    n =sys.argv[0].split(os.path.sep)[-1]
    usagi=[
        f"Vector-06c BASIC->ASC and ASC->BASIC converter by Viacheslav Slavinsky, svofski@gmail.com",
        "Usage:",
        f" {n} source.bas debas.asc rebas.bas     complete round-trip for testing",
        f" {n} source.bas debas.asc               convert basic tokenized file into plain text",
        f" {n} source.asc rebas.bas               tokenize plain text and produce .bas file"]
    print("\n".join(usagi))

if len(sys.argv) == 1:
    usagi()
    sys.exit(1)

def filenames():
    ext1 = os.path.splitext(sys.argv[1])
    ext2 = (ext1[0],False)
    try:
        ext2 = os.path.splitext(sys.argv[2])
    except:
        pass

    if ext1[1].lower == ".bas":
        output_ext = ".asc"
    else:
        output_ext = ".bas"

    fin = ext1[0] + ext1[1]
    fon = ext2[0] + (output_ext if ext2[1] == 0 else ext2[1])

    return fin,fon,ext1[1].lower()

if len(sys.argv) == 4:
    # test mode
    debas = readbas(sys.argv[1])
    with open(sys.argv[2], 'w', encoding=ENCODING) as fo:
        for line in debas:
            fo.write(line + '\n')

    bas = enbas(sys.argv[2])
    with open(sys.argv[3], 'wb') as fob:
        fob.write(bas)
elif len(sys.argv) in [2,3]:
    fin,fon,input_ext = filenames()
    if input_ext == ".bas":
        print("Converting BAS->ASC: ", fin, fon)
        debas = readbas(fin)
        with open(fon, 'w', encoding=ENCODING) as fo:
            for line in debas:
                fo.write(line + '\n')
    elif input_ext in [".asc",".txt"]:
        print("Converting asc->bas: ", fin, fon)
        bas = enbas(fin)
        with open(fon, 'wb') as fob:
            fob.write(bas)
    else:
        usagi()
        sys.exit(1)
