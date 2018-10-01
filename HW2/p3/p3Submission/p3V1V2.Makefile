lakeV1V2: lakegpu.cu lake.cu
	nvcc lakegpu.cu lake.cu -o ./lake -O3 -lm -Wno-deprecated-gpu-targets
clean: 
	rm -f lake *.o
