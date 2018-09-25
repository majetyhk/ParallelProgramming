all: lake

lake: lakegpuV2.cu lakeV2.cu
	nvcc lakegpuV2.cu lakeV2.cu -o ./lake -O3 -lm -Wno-deprecated-gpu-targets

clean: 
	rm -f lake
