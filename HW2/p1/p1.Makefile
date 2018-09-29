mpimake: p1.c
	mpicc -lm -O3 -o p1 p1.c my_mpi.c