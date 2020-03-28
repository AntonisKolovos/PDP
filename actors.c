#include "mpi.h"
#include "pool.h"
#include "actors.h"
#include <stdio.h>
#include "squirrel-functions.h"


void Grid_work(int parentID)
{

    int workerStatus = 1;
    while (workerStatus)
    {
        int myRank, parentId = 0;
        MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
        int myActor = getCommandData(); // We encode the parent ID in the wake up command data
        int populationInflux[NCELL];
        int infectionLevel[NCELL];

        printf("Worker on process %d, i am the Grid \n", myRank);

        int running = 1;
        int month = 0;
        MPI_Request request[2];
        MPI_Status status;
        int inbound[2],outbound[2];
        MPI_Irecv(inbound,2,MPI_INT,MPI_ANY_SOURCE,SQUIRREL_TAG,MPI_COMM_WORLD,&request[1]);

        while (month < 5)
        {
            
            MPI_Irecv(NULL, 0, MPI_INT, 2, CLOCK_TAG, MPI_COMM_WORLD, &request[0]);
            running=1;
            while (running)
            {
                int flag = 0;
                MPI_Test(&request[0], &flag, MPI_STATUS_IGNORE);
                //printf("Grid in loop, flag=%d\n", flag);
                if (flag)
                {
                    printf("Grid received clock signal, month=%d \n",month+1);
                    month++;
                    running=0;
                }
                flag=0;
                MPI_Test(&request[1],&flag,&status);
                if (flag){
                    
                    outbound[0]=111;
                    outbound[1]=111;
                    MPI_Ssend(outbound,2,MPI_INT,status.MPI_SOURCE,SQUIRREL_TAG,MPI_COMM_WORLD);
                  //  printf("received Cell=%d Infected=%d\n",inbound[0],inbound[1]);
                    MPI_Irecv(inbound,2,MPI_INT,MPI_ANY_SOURCE,SQUIRREL_TAG,MPI_COMM_WORLD,&request[1]);

                }
            }
        }
        
        MPI_Send(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("Grid, my rank=%d Going to Sleep\n", myRank);
        workerStatus = workerSleep();
    }
}

void Clock_work(int parentID)

{
int myRank, parentId = 0;
MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
		
int workerStatus = 1;
while (workerStatus)
    {
        int gridPid=1;
        MPI_Status status;
        MPI_Request request;
       // MPI_Recv(&gridPid, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("Worker on process %d Clock started! gridPId=%d \n", myRank, gridPid);
        // Tell my parent that I have been alive and am about to die
        double start = MPI_Wtime();
        while (MPI_Wtime() - start < 5)
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
    long seed =-1234;
 
    initialiseRNG(&seed);
    int move = 1;
    float x, y;
    long state;
    int infected=0;
    squirrelStep(0, 0, &x, &y, &state);
    printf("Initial position %f,%f\n",x,y);
    MPI_Request request[2];
    MPI_Irecv(NULL,0,MPI_INT,0,SQUIRREL_TAG,MPI_COMM_WORLD,&request[0]);
    int blocked,flag,cell,step=0;
    int outbound[2];
    int inbound[2];
    while (move)
    {   float x_new,y_new;
        squirrelStep(x,y,&x_new,&y_new,&state);
        x=x_new;
        y=y_new;
        //Prepare Data packet for Grid Actor
        outbound[0]=getCellFromPosition(x,y);
        outbound[1]=infected;
        MPI_Send(outbound,2,MPI_INT,1,SQUIRREL_TAG,MPI_COMM_WORLD);
        MPI_Irecv(inbound,2,MPI_INT,1,SQUIRREL_TAG, MPI_COMM_WORLD,&request[1]);
        blocked=1;
        
        while (blocked){
            MPI_Test(&request[1],&blocked,MPI_STATUS_IGNORE);
            MPI_Test(&request[0],&flag,MPI_STATUS_IGNORE);
            if (shouldWorkerStop()){
            printf("Squirrel Terminating... \n");
            blocked=0;
            move=0;
            }

        }
        step++;
        //mprintf("New step Cell=%d, Position=%f,%f\n",outbound[0],x,y);
       // MPI_Test(&request[0],&flag,MPI_STATUS_IGNORE);
       // if (flag){
       //     printf("Squirrel Terminating... \n");
        //    move=0;
       // }
       
        
    }
    printf("Squirrel has stopped step=%d\n",step);
    
    MPI_Ssend(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
    printf("Squirrel, my rank=%d Going to Sleep\n", myRank);
    workerStatus = workerSleep();
}