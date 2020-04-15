#include "mpi.h"
#include "pool.h"
#include "parameters.h"
#include <stdio.h>
#include "squirrel-functions.h"
#include "squirrel.h"
#include "grid.h"

void Grid_work(int parentID)
{

    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    int myActor = getCommandData(); // We encode the parent ID in the wake up command data
    int populationStore[NCELL][3]={0};
    int infectionStore[NCELL][2]={0};
    int popInfluxLvl[NCELL]={0};
    int infectionLvl[NCELL]={0};
    int currentMonthPop=0;
    int currentMonthInf=0;
    int i, j;

   if (DEBUG) printf("Worker on process %d, i am the Grid \n", myRank);

    int inMonth = 1;
    int month = 1;
    MPI_Status status;
    int inbound[2], outbound[2];
    int running = 1;
    int pendingReceive = 0;

    int sum = 0;

    while (running)
    {
        //Check for Squirrel Messages
        int receiveMsg = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, SQUIRREL_TAG, MPI_COMM_WORLD, &receiveMsg, &status);
        if (receiveMsg)
        {
            MPI_Recv(inbound, 2, MPI_INT, status.MPI_SOURCE, SQUIRREL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum++;

            //Send values to squirrel
            outbound[0] = popInfluxLvl[inbound[0]];
            outbound[1] = infectionLvl[inbound[0]];
            MPI_Bsend(outbound, 2, MPI_INT, status.MPI_SOURCE, GRID_TAG, MPI_COMM_WORLD);

            //Add the squirrel values
            populationStore[inbound[0]][month % 3]++;
            infectionStore[inbound[0]][month % 2]+= inbound[1];
           
        }

        //Check for Clock Message
        int flag = 0;
        MPI_Iprobe(2, CLOCK_TAG, MPI_COMM_WORLD, &flag, &status);

        if (flag)
        {
            MPI_Recv(NULL, 0, MPI_INT, 2, CLOCK_TAG, MPI_COMM_WORLD, &status);
           if (DEBUG) printf("Grid received clock signal, month=%d, sum=%d \n", month + 1, sum);

            //calculate the current values
            calcInfectionLvl(infectionStore, infectionLvl);
            calcPopInfux(populationStore, popInfluxLvl);

            //Send them to the master for printing
            MPI_Ssend(popInfluxLvl,NCELL,MPI_INT,0,GRID_TAG,MPI_COMM_WORLD);
            MPI_Ssend(infectionLvl,NCELL,MPI_INT,0,GRID_TAG,MPI_COMM_WORLD);
            MPI_Ssend(&month,1,MPI_INT,0,GRID_TAG,MPI_COMM_WORLD);
            month++;
            if(DEBUG) printf("Sent month msg to master!\n");
            //Zero this month's values 
            for (i=0;i<NCELL;i++){
                populationStore[i][month % 3]=0;
                infectionStore[i][month % 2]=0;
            }         
 
        }
        //Check if the simulation is still running
        if (shouldWorkerStop())
        {
            running = 0;
        }
    }

  if (DEBUG)  printf("Grid, my sum=%d Going to Sleep\n", sum);
}

static void calcPopInfux(int populationStore[NCELL][3], int popInfluxLvl[NCELL])
{

    int i, j;
    for (i = 0; i < NCELL; i++)
    {
        popInfluxLvl[i] = 0;
        for (j = 0; j < 3; j++)
        {
            popInfluxLvl[i] += populationStore[i][j];
        }
    }
}

static void calcInfectionLvl(int infectionStore[NCELL][2], int infectionLvl[NCELL])
{
    int i, j;
    for (i = 0; i < NCELL; i++)
    {
        infectionLvl[i]=0;
        for (j = 0; j < 2; j++)
        {
            infectionLvl[i] += infectionStore[i][j];
        }
    }
}