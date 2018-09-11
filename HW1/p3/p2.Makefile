mpimake: p2_mpi.c p2_func.c
	mpicc -lm -O3 -o p2 p2_mpi.c p2_func.c