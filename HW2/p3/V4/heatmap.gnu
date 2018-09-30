# heatmap for lake.cu

set terminal png

set xrange[0:1]
set yrange[0:1]

set output '1.png'
plot '0.dat' using 1:2:3 with image

set output '2.png'
plot '1.dat' using 1:2:3 with image

set output '3.png'
plot '2.dat' using 1:2:3 with image

set output '4.png'
plot '3.dat' using 1:2:3 with image

set output '0_gpu.png'
plot '0_gpu.dat' using 1:2:3 with image

set output '1_gpu.png'
plot '1_gpu.dat' using 1:2:3 with image

set output '2_gpu.png'
plot '2_gpu.dat' using 1:2:3 with image

set output '3_gpu.png'
plot '3_gpu.dat' using 1:2:3 with image