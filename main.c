/*
 * Author:B158495
 * Simulation paremeters can be set in actors.h
 */

#include <stdio.h>
#include "mpi.h"
#include "pool.h"
#include "squirrel-functions.h"
#include "parameters.h"
#include "clock.h"
#include "grid.h"
#include "squirrel.h"

static void workerCode();

void setInfected(int, int);
void printMonthResults(int, int,int);
void receiveSquirrelMsg(MPI_Status , int *,int*,int*);

int main(int argc, char *argv[])
{

	// Call MPI initialize first
	MPI_Init(&argc, &argv);

	int world_size,myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	MPI_Comm_size(MPI_COMM_WORLD,&world_size);
	if(world_size<(MAX_SQUIRRELS+3)){
		if(myrank==0)printf("Error: Not enough processes must have at least Max Squirrels + 3 \n");
		MPI_Finalize();
		exit(0);
	}

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
		void *buffer = malloc(BUFF_SIZE);
		MPI_Buffer_attach(buffer, BUFF_SIZE);
		MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
		if (DEBUG)
			printf("Master, my rank=%d \n", myRank);

		//Create Workers
		int gridPid = startWorkerProcess(); //Grid pid=1
		setActorType(gridPid, Grid);
		int clockPid = startWorkerProcess(); //Clock pid =2
		setActorType(clockPid, Clock);

		//Spawn initial squirrels
		int i, returnCode, activeSquirrels = 0,infectedSquirrels=0;
		for (i = 0; i < INIT_SQUIRRELS; i++)
		{
			int squirrelPid = startWorkerProcess();
			setActorType(squirrelPid, Squirrel);
			if (i < INFECTED){
				setInfected(squirrelPid, 1);
				infectedSquirrels++;
			}
			else{
				setInfected(squirrelPid, 0);
			}
			activeSquirrels++;
		}
		if (DEBUG)
			printf("Master, spawned initial squirrels\n");

		int masterStatus = 1;
		MPI_Status status;
		int receiveMsg = 0;
		while (masterStatus)
		{
			//Check for termination message from clock
			receiveMsg = 0;
			MPI_Iprobe(clockPid, MPI_ANY_TAG, MPI_COMM_WORLD, &receiveMsg, &status);
			if (receiveMsg)
			{
				MPI_Recv(NULL, 0, MPI_INT, clockPid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				if (DEBUG)
					printf("Master shutting down...\n");
				masterStatus = 0;
			}

			//Check for results message from grid
			receiveMsg = 0;
			MPI_Iprobe(gridPid, MPI_ANY_TAG, MPI_COMM_WORLD, &receiveMsg, &status);
			if (receiveMsg)
			{
				printMonthResults(gridPid, activeSquirrels,infectedSquirrels);
			}

			//Check for Squirrel message
			receiveMsg = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, SQUIRREL_TAG, MPI_COMM_WORLD, &receiveMsg, &status);
			if (receiveMsg)
			{
				receiveSquirrelMsg(status, &activeSquirrels,&infectedSquirrels,&masterStatus);
			}
		}
		printf("\nSimulation ended successfully! \n");
	}
	// Finalizes the process pool, call this before closing down MPI
	processPoolFinalise();
	// Finalize MPI, ensure you have closed the process pool first
	MPI_Finalize();
	return 0;
}



static void workerCode()
{
	int parentID = getCommandData();
	int myActor = getActorType(parentID);
	if (myActor == Grid)
		Grid_work(parentID);
	else if (myActor == Clock)
		Clock_work(parentID);
	else if (myActor == Squirrel)
		Squirrel_work(parentID);
}


void setInfected(int pid, int infected)
{
	MPI_Bsend(&infected, 1, MPI_INT, pid, SQUIRREL_TAG, MPI_COMM_WORLD);
}

void printMonthResults(int gridPid, int activeSquirrels,int infectedSquirrels)
{
	int popInfluxLvl[NCELL];
	int infectionLvl[NCELL];
	int month;
	MPI_Recv(popInfluxLvl, NCELL, MPI_INT, gridPid, GRID_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(infectionLvl, NCELL, MPI_INT, gridPid, GRID_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&month, NCELL, MPI_INT, gridPid, GRID_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	printf("\n----------------------------------------------------------------------------------------------------------------------------\n");
	printf("MONTH= %d\t ALIVE SQUIRRELS= %d\t INFECTED SQUIRRELS= %d\n", month, activeSquirrels,infectedSquirrels);
	printf("CELL No\t\t 1\t 2\t 3\t 4\t 5\t 6\t 7\t 8\t 9\t 10\t 11\t 12\t 13\t 14\t 15\t 16\n", month);
	printf("POPINFLX\t");
	int i;
	for (i = 0; i < NCELL; i++)
	{
		printf("%d\t ", popInfluxLvl[i]);
	}
	printf("\nINFLVL\t\t");
	for (i = 0; i < NCELL; i++)
	{
		printf("%d\t ", infectionLvl[i]);
	}
}

void receiveSquirrelMsg(MPI_Status status, int *activeSquirrels,int *infectedSquirrels,int *masterStatus)
{
	int data;
	MPI_Recv(&data, 1, MPI_INT, status.MPI_SOURCE, SQUIRREL_TAG, MPI_COMM_WORLD, &status);
	if (data == squirrelDied){
		(*activeSquirrels)--;
		(*infectedSquirrels)--;
		if(DEBUG) printf("\nSquirrel Died, alive=%d\t infected=%d\n",*activeSquirrels,*infectedSquirrels);

	}
	else if(data==newSquirrel)
	{
		if ((*activeSquirrels) < MAX_SQUIRRELS)
		{
			int squirrelPid = startWorkerProcess();
			setActorType(squirrelPid, Squirrel);
			setInfected(squirrelPid, 0);
			(*activeSquirrels)++;
			if (DEBUG)
				printf("MASTER Spawned new squirrel, Squirells=%d \n", activeSquirrels);
		}
		else
		{
			if(STOP_MAX_SQUIRRELS){
				printf("ERROR : EXCEEDED MAX NUM OF SQUIRRELS, TERMINATING...\n");
				shutdownPool();
				*masterStatus=0;
			}
			
		}
	}
	else if(data==squirrelInfected){
		(*infectedSquirrels)++;
	}
}
