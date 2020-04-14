#include "mpi.h"
#include "pool.h"
#include "parameters.h"
#include <stdio.h>
#include "squirrel-functions.h"
#include "squirrel.h"


void Squirrel_work(int parentID)
{
    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    long seed = -1 - myRank;
    initialiseRNG(&seed);
    //Initialize arrays
    int popInflux[50] = {0};
    int infectionLvl[50] = {0};
    int alive = 1;
    float x, y;
    long state;
    int infected = 0;
    int firstTime=1;
    //Block until i receive my infeciton state
    MPI_Recv(&infected,1,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
  if (DEBUG)  printf("Worker on process %d, i am the Squirrel, infected=%d  )\n", myRank,infected);
    int infectedSteps = 0;
    //Get initial position
    squirrelStep(0, 0, &x, &y, &seed);
    MPI_Request request[2];
    int blocked, flag, cell;
    int step = 0;
    int outbound[2];
    int inbound[2];
    while (alive)
    {
        //Step to new position
        float x_new, y_new;
        squirrelStep(x, y, &x_new, &y_new, &seed);
        x = x_new;
        y = y_new;
        //Prepare Data packet for Grid Actor
        outbound[0] = getCellFromPosition(x, y);
        outbound[1] = infected;
        //Send the data
        MPI_Bsend(outbound, 2, MPI_INT, 1, SQUIRREL_TAG, MPI_COMM_WORLD);

        //Issue receive for the cell data
        MPI_Irecv(inbound, 2, MPI_INT, 1, GRID_TAG, MPI_COMM_WORLD, &request[1]);
        blocked = 0;
        //Block until i receive the data or the simulation terminates
        while (blocked == 0)
        {
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
            //If infected, send message to the master, only once
            if(infected&&firstTime){
                int send=squirrelInfected;
                MPI_Bsend(&send,1,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD);
                firstTime=0;
            }
            
        }
        else 
        {
            infectedSteps++;
        }

        //Determine if i will die
        if ((infectedSteps > 50) && willDie(&seed))
        {
            alive = 0;
            int send=squirrelDied;
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
                int send=newSquirrel;
                MPI_Bsend(&send,1,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD);
            }
        }
        if(alive)step++;
    }
  if (DEBUG)  printf("Squirrel has stopped step=%d\n", step);
   if (DEBUG) printf("Squirrel, my rank=%d Going to Sleep\n", myRank);
    workerSleep();
}

static float avgPop(int popInfux[50])
{
    int i, avg = 0;
    for (i = 0; i < 50; i++)
    {
        avg += popInfux[i];
    }
    return avg / 50;
}
static float avgInf(int infectionLvl[50])
{
    int i, avg = 0;
    for (i = 0; i < 50; i++)
    {
        avg += infectionLvl[i];
    }
    return avg / 50;
}