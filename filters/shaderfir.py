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
    fu.write('const float %s[%d] = float[%d] (%s);' % 
            (identifier, len(b), len(b), "".join(["%-3.18f,"%x for x in b])[:-1]));
    fu.close()

def plot_filter(coefs_m, legend_m, figfilename):
    fig = mpl.pyplot.figure()
    ax1 = fig.add_subplot(111)

    for coefs in coefs_m:
        (w,H) = signal.freqz(coefs)
        ax1.plot(w, 20*log10(abs(H)))

    ax1.legend(legend_m)
    #bx = bands*2*pi
    #ax1.axvspan(bx[1], bx[2], facecolor='g', alpha=0.33)
    ax1.plot(pi/2, -6, 'go')
    ax1.axvline(pi/2, color='g', linestyle='--')
    #ax1.axis([0,pi * 44e3/1.5e6,-64,3])
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
N = 19
bands = numpy.array([0., .01, .1, .5])
h = signal.remez(N+1, bands, [1,0], [1,1])
h[abs(h) <= 1e-4] = 0.
(w,H) = signal.freqz(h)

# ~~[Filter Design with Windowed freq]~~
b = signal.firwin(N+1, 0.05, window=('kaiser', 1.0))
b[abs(h) <= 1e-4] = 0.
(wb, Hb) = signal.freqz(b)

old=[-0.008030271,0.003107906,0.016841352,0.032545161,0.049360136,0.066256720,0.082120150,0.095848433,0.106453014,0.113151423,0.115441842,0.113151423,0.106453014,0.095848433,0.082120150,0.066256720,0.049360136,0.032545161,0.016841352,0.003107906]
export_header("FIR", b, "pal-fir.h")
plot_filter([b,h,old], ["pal fir", "remez", "old fir"], "pal-fir.png")
