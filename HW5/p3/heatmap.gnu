# heatmap for lake.cu

set terminal png

set xrange[0:1]
set yrange[0:1]
#set cbrange[-1:1]

set output 'lake_1.png'
plot 'lake_1.out' using 1:2:3 with image

set output 'lake_2.png'
plot 'lake_2.out' using 1:2:3 with image

