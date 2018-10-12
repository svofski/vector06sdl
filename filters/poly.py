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
    for ii in range(len(b)):
        fu.write("%-3.8f,\n" % b[ii])
    fu.write("};")
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
    ax1.axis([0,pi * 44e3/1.5e6,-64,3])
    #ax1.axis([0,pi,-64,3])
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
Fs=1.5e6
L = 5 # interpolation
M = 156 # decimation
#N = 1284 # 256 * L # is perfect but fat
#N = 128 * L # + firwin = almost perfect but a bit whistly on serduk
#N = 132 * L # + firwin = almost perfect but a bit whistly on serduk
#N = 152 * L # + firwin = almost perfect but a bit whistly on serduk
N = 182 * L # + firwin .7/M = whistle only heard after normalisation in atrocity

# ~~[Filter Design with Windowed freq]~~
fw = signal.firwin(N+1, .6/M, window=('kaiser', 7.8562))
#fw = signal.remez(N+1, [0, 0.7/M, 1./M, 0.5], [1,0], [1,1])
fw[abs(fw) <= 1e-4] = 0.
print(fw)
print "Full filter has this many taps: N+1=", N+1
trim=fw[214:len(fw)-214]
plot_filter([fw,trim], ['firwin','trim'], 'interp.png')
print "But they have plenty of zeroes which we trim down to: ", len(trim)
export_header('coefs', trim, 'interp.h')


