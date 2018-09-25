lake: lakegpu.cu lake.cu
        nvcc lakegpu.cu lake.cu -o lake -O3 -lm -Wno-deprecated-gpu-targets

lake-mpi: lake_mpi.cu  lakegpu_mpi.cu
        nvcc -c lakegpu_mpi.cu -O3 -lm -Wno-deprecated-gpu-targets -I/opt/ohpc/pub/mpi/mvapich2-gnu/2.2/include
            nvcc -c lake_mpi.cu -O3 -lm -Wno-deprecated-gpu-targets -I/opt/ohpc/pub/mpi/mvapich2-gnu/2.2/include
                mpicc lakegpu_mpi.o lake_mpi.o -o lake -O3 -lm -L/usr/local/cuda/lib64/ -lcudart

clean: 
        rm -f lake *.o
