/*
 * Example code to run and test the process pool. To compile use something like mpicc -o test test.c pool.c
 */

#include <stdio.h>
#include "mpi.h"
#include "pool.h"
#include "squirrel-functions.h"
#include "actors.h"

static void workerCode();


int main(int argc, char *argv[])
{

	// Call MPI initialize first
	MPI_Init(&argc, &argv);
	/*
	 * Initialise the process pool.
     * The return code is = 1 for worker to do some work, 0 for do nothing and stop and 2 for this is the master so call master poll
     * For workers this subroutine will block until the master has woken it up to do some work
	 */
	int statusCode = processPoolInit();
	if (statusCode == 1)
	{
		// A worker so do the worker tasks
		workerCode();
	}
	else if (statusCode == 2)
	{

		/*
		 * This is the master, each call to master poll will block until a message is received and then will handle it and return
         * 1 to continue polling and running the pool and 0 to quit.
         * Basically it just starts 10 workers and then registers when each one has completed. When they have all completed it
         * shuts the entire pool down
		 */
		int myRank;
		void *buffer =malloc(BUFF_SIZE);
		MPI_Buffer_attach(buffer,BUFF_SIZE);
		MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
		printf("Master, my rank=%d \n", myRank);
		//Create Workers
		MPI_Request initialWorkerRequests[NWORK];
		int gridPid = startWorkerProcess(); //Grid pid=1
		setActorType(gridPid,Grid);
		int clockPid = startWorkerProcess();//Clock pid =2
		setActorType(clockPid,Clock);

		
		int squirrelPid =startWorkerProcess();
		setActorType(squirrelPid,Squirrel);

		MPI_Irecv(NULL, 0, MPI_INT, gridPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[0]);
		MPI_Irecv(NULL, 0, MPI_INT, clockPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[1]);
		MPI_Irecv(NULL, 0, MPI_INT, squirrelPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[2]);

		int activeWorkers = 3;
		int i, returnCode;

	
		int masterStatus = masterPoll();
		while (masterStatus)
		{
			masterStatus = masterPoll();
			for (i = 0; i < NWORK; i++)
			{
				// Checks all outstanding workers that master spawned to see if they have completed
				if (initialWorkerRequests[i] != MPI_REQUEST_NULL)
				{
					MPI_Status status;
					MPI_Test(&initialWorkerRequests[i], &returnCode, &status);
					if (returnCode)
					{
						activeWorkers--;
						printf("Received termination code from %d \n", status.MPI_SOURCE);
						
					}
					
				}
			}
			// If we have no more active workers then quit poll loop which will effectively shut the pool down when  processPoolFinalise is called
			if (activeWorkers == 0)
				break;
		}
	}
	// Finalizes the process pool, call this before closing down MPI
	//printf("Finalizing Pool...\n");
	processPoolFinalise();
	// Finalize MPI, ensure you have closed the process pool first
	MPI_Finalize();
	return 0;
}

static void workerCode()
{	
	int parentID=getCommandData();
	int myActor = getActorType(parentID);
	if (myActor == Grid) Grid_work(parentID);
	else if (myActor == Clock) Clock_work(parentID);
	else if (myActor == Squirrel)Squirrel_work(parentID);
}
