#include "mpi.h"
#include "pool.h"
#include "actors.h"
#include <stdio.h>
#include "squirrel-functions.h"

int calcPopInfux();
int calcInfectionLvl();
void Grid_work(int parentID)
{

    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    int myActor = getCommandData(); // We encode the parent ID in the wake up command data
    int populationInflux[NCELL][3];
    int infectionLevel[NCELL][3];
    int i, j;
    for (i = 0; i < NCELL; i++)
    {
        for (j = 0; j < 3; j++)
        {
            populationInflux[i][j] = 0;
            infectionLevel[i][j] = 0;
        }
    }

    printf("Worker on process %d, i am the Grid \n", myRank);

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


        //Issue receive for clock signal
        MPI_Irecv(NULL, 0, MPI_INT, 2, CLOCK_TAG, MPI_COMM_WORLD, &request[0]);
        inMonth = 1;
        while (inMonth)
        {

            if (!pendingReceive)
            {   
                //Issue Receive for squirrel message
                MPI_Irecv(inbound, 2, MPI_INT, MPI_ANY_SOURCE, GRID_TAG, MPI_COMM_WORLD, &request[1]);
                pendingReceive=1;
            }

            int flag = 0;
            MPI_Test(&request[0], &flag, MPI_STATUS_IGNORE);
            if (flag)
            {
                printf("Grid received clock signal, month=%d, sum=%d \n", month + 1,sum);
                month++;
                inMonth = 0;
                //pendingReceive = 0;

                //if(pendingReceive) MPI_Wait(&request[1],MPI_STATUS_IGNORE);

            }
            int receiveMsg = 0;
            MPI_Test(&request[1], &receiveMsg, &status);

            if (receiveMsg)
            {
                sum++;
                MPI_Ssend(outbound, 2, MPI_INT, status.MPI_SOURCE, SQUIRREL_TAG, MPI_COMM_WORLD);
                //Send values to squirrel
                outbound[0] = calcPopInfux();
                outbound[1] = calcInfectionLvl();
                //Add the squirrel values
                populationInflux[inbound[0]][month % 3]++;
                infectionLevel[inbound[0]][month % 2] += inbound[1];
              
                //  Issue another receive
                pendingReceive = 0;
                //MPI_Wait(&request[3], MPI_STATUS_IGNORE);

            }
            //Check if the simulation is still running
            if (shouldWorkerStop())
            {
                running = 0;
                inMonth = 0;
            }
        }
        
    }
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < NCELL; i++)
        {

            printf("%d, ", populationInflux[i][j]);
        }
        printf("\n");
    }
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < NCELL; i++)
        {

            printf("%d, ", infectionLevel[i][j]);
        }
        printf("\n");
    }
    //Send termination code to master
    MPI_Send(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
    printf("Grid, my sum=%d Going to Sleep\n", sum);
}

void Clock_work(int parentID)

{
    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    int workerStatus = 1;
    while (workerStatus)
    {
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
            printf("1 month passed!\n");
            MPI_Send(NULL, 0, MPI_INT, gridPid, CLOCK_TAG, MPI_COMM_WORLD);
        }

        MPI_Ssend(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("Clock, my rank=%d Going to Sleep\n", myRank);
        shutdownPool();

        workerStatus = workerSleep();
    }
}

void Squirrel_work(int parentID)
{
    int myRank, parentId = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    printf("Worker on process %d, i am the Squirrel  )\n", myRank);

    int workerStatus = 1;
    long seed = -1234;

    initialiseRNG(&seed);
    int move = 1;
    float x, y;
    long state;
    int infected = 0;
    squirrelStep(0, 0, &x, &y, &state);
    printf("Initial position %f,%f\n", x, y);
    MPI_Request request[2];
    // MPI_Irecv(NULL,0,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD,&request[0]);
    int blocked, flag, cell;
    int step = 0;
    int outbound[2];
    int inbound[2];
    while (move)
    {
        float x_new, y_new;
        squirrelStep(x, y, &x_new, &y_new, &state);
        x = x_new;
        y = y_new;
        //Prepare Data packet for Grid Actor
        outbound[0] = getCellFromPosition(x, y);
        outbound[1] = infected;
        MPI_Isend(outbound, 2, MPI_INT, 1, GRID_TAG, MPI_COMM_WORLD,&request[0]);
        MPI_Irecv(inbound, 2, MPI_INT, 1, SQUIRREL_TAG, MPI_COMM_WORLD, &request[1]);
        blocked = 1;

        while (blocked)
        {
            MPI_Test(&request[1], &blocked, MPI_STATUS_IGNORE);
            if (shouldWorkerStop())
            {
                printf("Squirrel Terminating... \n");
                blocked = 0;
                move = 0;
            }
        }
        MPI_Wait(&request[0],MPI_STATUS_IGNORE);
        step++;
    }
    printf("Squirrel has stopped step=%d\n", step);

    MPI_Ssend(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
    printf("Squirrel, my rank=%d Going to Sleep\n", myRank);
    workerStatus = workerSleep();
}

int calcPopInfux()
{
    return 666;
}

int calcInfectionLvl()
{
    return 777;
}