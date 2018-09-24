nvccmake: p2.cu
	nvcc p2.cu -o p2 -O3 -lm -lcurand -Wno-deprecated-gpu-targets