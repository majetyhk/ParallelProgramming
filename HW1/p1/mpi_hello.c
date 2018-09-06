/******************************************************************************
 * FILE: mpi_hello.c
 * DESCRIPTION:
 *   MPI tutorial example code: Simple hello world program
 * AUTHOR: Blaise Barney
 * LAST REVISED: 08/19/12
 * REVISED BY: Christopher Mauney
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

/* This is the root process */
#define  ROOT       0

int main (int argc, char *argv[])
{

        /* process information */
        int   numproc, rank, len;

        /* current process hostname */
        char  hostname[MPI_MAX_PROCESSOR_NAME];

        /* initialize MPI */
        MPI_Init(&argc, &argv);

        /* get the number of procs in the comm */
        MPI_Comm_size(MPI_COMM_WORLD, &numproc);

        /* get my rank in the comm */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        /* get some information about the host I'm running on */
        MPI_Get_processor_name(hostname, &len);

        /* everyone says this */
        printf ("Hello from task %d on %s!\n", rank, hostname);

        /* only the rank 0 process says this */
        if (rank == ROOT)
                printf("MASTER: Number of MPI tasks is: %d\n", numproc);

        /* graceful exit */
        MPI_Finalize();

}

