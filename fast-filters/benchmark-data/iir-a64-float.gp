set title "IIR single precision A64 - 64k iterations"

set xlabel "Length of filter"
set ylabel "Time (s)"

plot "iir-a64.dat" u 1:2 w lines t "SIMD compile-time N", \
     "" u 1:3 w lines t "Scalar compile-time N", \
     "" u 1:4 w lines t "SIMD run-time N", \
     "" u 1:5 w lines t "Scalar run-time N"
