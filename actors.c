#include "mpi.h"
#include "pool.h"
#include "actors.h"
#include <stdio.h>
#include "squirrel-functions.h"

void calcPopInfux(int populationStore[NCELL][3], int popInfluxLvl[NCELL]);
void calcInfectionLvl(int infectionStore[NCELL][2], int infectionLvl[NCELL]);
float avgPop(int popInfux[50]);
float avgInf(int infectionLvl[50]);
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
    for (i = 0; i < NCELL; i++)
    {
        popInfluxLvl[i] = 0;
        infectionLvl[i] = 0;
        for (j = 0; j < 3; j++)
        {
            populationStore[i][j] = 0;
            if (j < 2)
                infectionStore[i][j] = 0;
        }
    }

   if (DEBUG) printf("Worker on process %d, i am the Grid \n", myRank);

    int inMonth = 1;
    int month = 0;
    MPI_Request request[3];
    MPI_Status status;
    int inbound[2], outbound[2];
    int running = 1;
    int pendingReceive = 0;

    int sum = 0;

    while (running)
    {

        int receiveMsg = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, GRID_TAG, MPI_COMM_WORLD, &receiveMsg, &status);
        if (receiveMsg)
        {
            MPI_Recv(inbound, 2, MPI_INT, status.MPI_SOURCE, GRID_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum++;

            //Send values to squirrel
            outbound[0] = popInfluxLvl[inbound[0]];
            outbound[1] = infectionLvl[inbound[0]];
            MPI_Bsend(outbound, 2, MPI_INT, status.MPI_SOURCE, SQUIRREL_TAG, MPI_COMM_WORLD);

            //Add the squirrel values
            populationStore[inbound[0]][month % 3]++;
            infectionStore[inbound[0]][month % 2]+= inbound[1];
           
        }

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

void Clock_work(int parentID)

{
    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    int gridPid = 1;
    MPI_Status status;
    MPI_Request request;
    // MPI_Recv(&gridPid, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    printf("Worker on process %d Clock started! gridPId=%d \n", myRank, gridPid);
    // Tell my parent that I have been alive and am about to die
    double start = MPI_Wtime();
    while (MPI_Wtime() - start < RUNTIME)
    {
        double time = MPI_Wtime();
        while (MPI_Wtime() - time < 1)
            ;
      if (DEBUG)  printf("1 month passed!\n");
        MPI_Send(NULL, 0, MPI_INT, gridPid, CLOCK_TAG, MPI_COMM_WORLD);
    }
    shutdownPool();

    MPI_Ssend(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
   if (DEBUG) printf("Clock, my rank=%d Going to Sleep\n", myRank);
}

void Squirrel_work(int parentID)
{
    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    long seed = -1 - myRank;
    initialiseRNG(&seed);
    int popInflux[50] = {0};
    int infectionLvl[50] = {0};
    int alive = 1;
    float x, y;
    long state;
    int infected = 0;
    MPI_Recv(&infected,1,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
  if (DEBUG)  printf("Worker on process %d, i am the Squirrel, infected=%d  )\n", myRank,infected);
    int infectedSteps = 0;
    squirrelStep(0, 0, &x, &y, &seed);
    //printf("Initial position %f,%f\n", x, y);
    MPI_Request request[2];
    // MPI_Irecv(NULL,0,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD,&request[0]);
    int blocked, flag, cell;
    int step = 0;
    int outbound[2];
    int inbound[2];
    while (alive)
    {
        float x_new, y_new;
        squirrelStep(x, y, &x_new, &y_new, &seed);
        x = x_new;
        y = y_new;
        //Prepare Data packet for Grid Actor
        outbound[0] = getCellFromPosition(x, y);
        outbound[1] = infected;
        //Send the data

        MPI_Bsend(outbound, 2, MPI_INT, 1, GRID_TAG, MPI_COMM_WORLD);
        MPI_Irecv(inbound, 2, MPI_INT, 1, SQUIRREL_TAG, MPI_COMM_WORLD, &request[1]);
        blocked = 0;
        while (blocked == 0)
        {
            //MPI_Irecv(inbound, 2, MPI_INT, 1, SQUIRREL_TAG, MPI_COMM_WORLD, &request[1]);
            MPI_Test(&request[1], &blocked, MPI_STATUS_IGNORE);

            if (shouldWorkerStop())
            {
             if (DEBUG)   printf("Squirrel Terminating... \n");
                blocked = 1;
                alive = 0;
            }
        }

        //Store data
        popInflux[step % 50] = inbound[0];
        infectionLvl[step % 50] = inbound[1];

        //Determine if infected
        if(!infected) {
            infected=willCatchDisease(avgInf(infectionLvl), &seed);
            }
        else 
        {
            infectedSteps++;
        }

        //Determine if i will die
        if ((infectedSteps > 50) && willDie(&seed))
        {
            alive = 0;
            int send=0;
            //Notify the master that I have died,
            MPI_Bsend(&send,1,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD);
          if (DEBUG)  printf("Squirrel %d ,I died!\n",myRank);

        }

        //Determine if i will give birth
        if (step % 50 == 0)
        {
            if (willGiveBirth(avgPop(popInflux), &seed))
            {
              if (DEBUG)  printf("New Squirrel!\n");
                int send=1;
                MPI_Bsend(&send,1,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD);
            }
        }
        //  MPI_Wait(&request[0],MPI_STATUS_IGNORE);
        if(alive)step++;
    }
  if (DEBUG)  printf("Squirrel has stopped step=%d\n", step);
   if (DEBUG) printf("Squirrel, my rank=%d Going to Sleep\n", myRank);
    workerSleep();
}

void calcPopInfux(int populationStore[NCELL][3], int popInfluxLvl[NCELL])
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

void calcInfectionLvl(int infectionStore[NCELL][2], int infectionLvl[NCELL])
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

float avgPop(int popInfux[50])
{
    int i, avg = 0;
    for (i = 0; i < 50; i++)
    {
        avg += popInfux[i];
    }
    return avg / 50;
}
float avgInf(int infectionLvl[50])
{
    int i, avg = 0;
    for (i = 0; i < 50; i++)
    {
        avg += infectionLvl[i];
    }
    return avg / 50;
}