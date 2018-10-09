# Credit: Christopher Felton
# https://www.dsprelated.com/showcode/270.php
#
# The following is a Python/scipy snippet to generate the 
# coefficients for a halfband filter.  A halfband filter
# is a filter where the cutoff frequency is Fs/4 and every
# other coeffecient is zero except the cetner tap.
# Note: every other (even except 0) is 0, most of the coefficients
#       will be close to zero, force to zero actual

import numpy
from numpy import log10, abs, pi
import scipy
from scipy import signal
import matplotlib
import matplotlib.pyplot
import matplotlib as mpl

def export_header(identifier, b, filename):
    # export firwin-designed halfband filter to halfband.h
    # on pwm18i2 firwin gives slightly lower energy in  phantom frequencies 
    # above 9khz
    fu = open(filename, "w")
    fu.write('static constexpr float %s[]={\n' % identifier)
    for ii in range(N+1):
        fu.write("%-3.8f,\n" % b[ii])
    fu.write("};")
    fu.close()

def plot_filter(coefs, legend, figfilename):
    (w,H) = signal.freqz(coefs)

    fig = mpl.pyplot.figure()
    ax1 = fig.add_subplot(111)
    ax1.plot(w, 20*log10(abs(H)))
    ax1.legend([legend])
    bx = bands*2*pi
    ax1.axvspan(bx[1], bx[2], facecolor='g', alpha=0.33)
    ax1.plot(pi/2, -6, 'go')
    ax1.axvline(pi/2, color='g', linestyle='--')
    ax1.axis([0,pi,-64,3])
    ax1.grid(True)
    ax1.set_ylabel('Magnitude (dB)')
    ax1.set_xlabel('Normalized Frequency (radians)')
    ax1.set_title('Filter Frequency Response')
    fig.savefig(figfilename)


## ~~[Filter Design with Parks-McClellan Remez]~~
# half-band filter, 33 taps - used 6 times to bring down
# 1.5e6 down to 23437.5
#
# Both designs are okay, but firwin seems to win over remez
#
N = 32
bands = numpy.array([0., .22, .28, .5])
h = signal.remez(N+1, bands, [1,0], [1,1])
h[abs(h) <= 1e-4] = 0.
(w,H) = signal.freqz(h)

# ~~[Filter Design with Windowed freq]~~
b = signal.firwin(N+1, 0.5)
b[abs(h) <= 1e-4] = 0.
(wb, Hb) = signal.freqz(b)

# Dump the coefficients for comparison and verification
print('          remez       firwin')
print('------------------------------------')
for ii in range(N+1):
    print(' tap %2d   %-3.6f    %-3.6f' % (ii, h[ii], b[ii]))


export_header("coefs", b, "halfband.h")

# Design the final stage a bit narrower for less whistly sound
# 0.35 is a bit muffled
# 0.4 is a bit whistly
final_b = signal.firwin(N+1, 0.37)
final_b[abs(final_b) <= 1e-4] = 0.
plot_filter(final_b, "end stage", "res_final.png")
export_header("coefs", final_b, "endstage.h")


## ~~[Plotting]~~
# Note: the pylab functions can be used to create plots,
#       and these might be easier for beginners or more familiar
#       for Matlab users.  pylab is a wrapper around lower-level
#       MPL artist (pyplot) functions.
fig = mpl.pyplot.figure()
ax0 = fig.add_subplot(211)
ax0.stem(numpy.arange(len(h)), h)
ax0.grid(True)
ax0.set_title('Parks-McClellan (remez) Impulse Response')
ax1 = fig.add_subplot(212)
ax1.stem(numpy.arange(len(b)), b)
ax1.set_title('Windowed Frequency Sampling (firwin) Impulse Response')
ax1.grid(True)
fig.savefig('hb_imp.png')

fig = mpl.pyplot.figure()
ax1 = fig.add_subplot(111)
ax1.plot(w, 20*log10(abs(H)))
ax1.plot(w, 20*log10(abs(Hb)))
ax1.legend(['remez', 'firwin'])
bx = bands*2*pi
ax1.axvspan(bx[1], bx[2], facecolor='g', alpha=0.33)
ax1.plot(pi/2, -6, 'go')
ax1.axvline(pi/2, color='g', linestyle='--')
ax1.axis([0,pi,-64,3])
ax1.grid(True)
ax1.set_ylabel('Magnitude (dB)')
ax1.set_xlabel('Normalized Frequency (radians)')
ax1.set_title('Half Band Filter Frequency Response')
fig.savefig('hb_rsp.png')
