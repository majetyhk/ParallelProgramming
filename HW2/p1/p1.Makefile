mpimake: my_rtt.c my_mpi.c
	mpicc -lm -O3 -o my_rtt my_rtt.c my_mpi.c