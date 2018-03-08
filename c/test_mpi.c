#include <mpi.h>
#include <stdio.h>

/*
 * MPI_Init
 * MPI_Comm_size
 * ......rank
 * MPI_Recv
 * */

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	printf("hello, parallel world\n");
	MPI_Finalize();
	return 0;
}
