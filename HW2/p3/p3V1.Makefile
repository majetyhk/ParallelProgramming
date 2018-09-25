all: lake

lake: lakegpuV1.cu lakeV1.cu
	nvcc lakegpuV1.cu lakeV1.cu -o ./lake -O3 -lm -Wno-deprecated-gpu-targets

clean: 
	rm -f lake
