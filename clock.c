
#include "mpi.h"
#include "pool.h"
#include "parameters.h"
#include <stdio.h>

void Clock_work(int parentID)

{
    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    int gridPid = 1;
    MPI_Status status;
    MPI_Request request;
    if(DEBUG) printf("Worker on process %d Clock started! gridPId=%d \n", myRank, gridPid);


    int month=0;
    //Run for the specified timesteps
    while (month < RUNTIME)
    {
        double time = MPI_Wtime();
        //Wait for the time step time
        while (MPI_Wtime() - time < TIME_STEP);
        month++;
        if (DEBUG)  printf("1 month passed!\n");

        //Send message to the grid that the month has passed
        MPI_Send(NULL, 0, MPI_INT, gridPid, CLOCK_TAG, MPI_COMM_WORLD);
    }
    //When done, shutdown all workers
    shutdownPool();
    //Send termination message to the master
    MPI_Ssend(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
   if (DEBUG) printf("Clock, my rank=%d Going to Sleep\n", myRank);
}