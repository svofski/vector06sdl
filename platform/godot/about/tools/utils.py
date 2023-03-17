
def chunker(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

# msb first
def bitstream(bytes):
    for b in bytes:
        for i in xrange(8):
            yield (b & 0x80) >> 7
            b <<= 1

def bytestream(bits):
    for ch in chunker(bits, 8):
        byte = 0
        for bit in reversed(ch):
            byte = (byte >> 1) | (bit << 7)
        yield byte


# rle: 
#  0    x x x x x x x x  8 unpacked bits verbatim
#  1 0  n n n n n n n n  (n+1) zeros
#  1 1  n n n n n n n n  (n+1) ones
def rle(bytes):
    value = 0
    stream = []
    lastbit = 0
    run = []
    runtype = 0 # 0 = packed 0, 1 = packed 1, 2 = unpacked
    
    allequal = True
    flush = False

    for bit in bitstream(bytes):
        if allequal:
            if bit == lastbit:
                run.append(bit)                
                if len(run) == 256:
                    chunk = [1, bit] + [x for x in bitstream([len(run) - 1])]
                    stream += chunk
                    #print "Flushed chunk: ", chunk
                    run = []
            else:
                # if run is longer than 8 bits, flush it as packed
                if len(run) > 7:
                    chunk = [1, lastbit] + [x for x in bitstream([len(run) - 1])]
                    stream += chunk
                    #print "Flushed chunk: ", chunk
                    allequal = True # be optimistic, expect single inversion
                    run = [bit]     # and reinit the run
                else:
                    allequal = False
        
        if not allequal:
            # unpacked run
            run.append(bit)
            if len(run) == 8:
                chunk = [0] + run
                stream += chunk
                #print "Flushed chunk: ", chunk
                run = []
                allequal = True # optimism
        lastbit = bit

    if len(run) > 0:
        if allequal:
            stream += [1, lastbit] + [x for x in bitstream([len(run) - 1])]
        else:
            for i in xrange(8 - len(run)):
                run.append(0)
            stream += [0] + run

    return bytestream(stream)

def getmode(bitstream):
    b1 = next(bitstream)
    if b1 == 0:
        return 0
    else:
        b2 = next(bitstream)
        return (b1 << 1) | b2

def getbits(bitstream, n):
    return [next(bitstream) for x in xrange(n)]

def getbyte(bitstream):
    return next(bytestream(getbits(bitstream, 8)))

def unrle(bytes):
    run = []
    stream = bitstream(bytes)

    outstream = []
    try:
        while True:
            mode = getmode(stream)
            if mode == 0:
                #print "unpacked bits=",
                bits = getbits(stream, 8)
                #print bits
            else:
                npacked = getbyte(stream) + 1
                bits = [mode & 1] * npacked
                #print "packed ", mode & 1, npacked, bits
            outstream += bits
    except StopIteration:
        pass
    return bytestream(outstream)


# byte rle: 
# 0xxx xxxx following byte repeats x times
# 1xxx xxxx x unpacked bytes follow

def brle(bytes):
    run = []
    packed = True
    lastbyte = -1
    stream = []
    could = 0
    for byte in bytes:
        if packed:
            if byte == lastbyte or len(run) == 0:
                run.append(byte)
                if len(run) == 128:
                    stream += [127] + [byte]
                    run = []
            else:
                if len(run) > 2:
                    stream += [len(run)-1] + [lastbyte]
                    packed = True
                    run = [byte]
                else:
                    packed = False
                    could = 0
        if not packed:
            run.append(byte)
            if byte != lastbyte:
                could = 0
            if byte == lastbyte:
                could += 1
                if could > 2:
                    unpacked_run = run[:len(run)-could]
                    stream += [0x80 | (len(unpacked_run)-1)] + unpacked_run
                    packed = True
                    run = run[-could:]
                    #print "breaking up: unp=%s newrun=%s" % (repr(unpacked_run), repr(run))
                    could = 0
            elif len(run) == 127:
                stream += [0xfe] + run
                packed = True
                run = []

        lastbyte = byte

    if len(run) > 0:
        if packed:
            stream += [len(run)-1] + [lastbyte]
        else:
            stream += [0x80 | (len(run)-1)] + run
    stream += [0xff]
    return stream

def unbrle(stream):
    output = []
    while True:
        byte = next(stream)
        #print "byte = ", hex(byte)
        if byte == 0xff:
            break
        if byte & 0x80 == 0x80:
            for i in xrange((byte & 0x7f) + 1):
                output.append(next(stream))
        else:
            output += [next(stream)] * ((byte & 0x7f) + 1)

    return output

def test():
    global packed, unpacked
    input = range(10) + [0]*8 + range(10) + [255]*8 + [0x55]
    packed = rle(input)
    print("packed=", [hex(x) for x in packed])
    packed = rle(input)
    unpacked = unrle(packed)
    print("unpacked=", [x for x in unpacked])
    #print [x for x in rle([0]*100)]

#test()
