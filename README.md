# Simulation parameters
The simulation parameters can be configured in the parameters.h file

NCELL               Number of land cells  
RUNTIME             Simulation run time in months  
TIME_STEP           Simulation time step in seconds(seconds per month)  
BUFF_SIZE           MPI buffer size  
MAX_SQUIRRELS       Maximum allowed number of squirrels  
INIT_SQUIRRELS      Initial squirrels  
INFECTED            Initial infected squirrels  
DEBUG               Output debug messages(0=NO, 1=YES)  
STOP_MAX_SQUIRRELS  Stop simulation when max number of squirrels reached(0=NO, 1=YES)   

# Compilation
The intel compiler is used with the MPI interface(mpicc). The program can
be compiled using the supplied Makefile just use

> $ make

To clean the output files use
> $ make clean

# Execution
The program has to be executed using mpirun or mpiexec_mpt. Always ensure there are enough
processes available. The program requires 1 process for every squirrel+3.

> $ mpirun -n 36 ./simulation

# Example output 
Example output for 10 initial squirrels, of which 1 infected and a max number of 33
```
----------------------------------------------------------------------------------------------------------------------------
MONTH= 1         ALIVE SQUIRRELS= 9      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        43845    44119   43848   44085   43653   44085   43430   43880   43823   43692   43852   43643   43776   43669   44176   43510   
INFLVL          3        1       6       5       3       1       4       2       5       6       4       5       2       1       3       5       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 2         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        44427    44672   44414   44649   44250   44663   43998   44403   44331   44269   44440   44185   44367   44222   44731   44078   
INFLVL          37       35      39      32      38      32      40      25      38      44      38      25      37      32      29      35      
----------------------------------------------------------------------------------------------------------------------------
MONTH= 3         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        44427    44672   44414   44649   44250   44663   43998   44403   44331   44269   44440   44185   44367   44222   44731   44078   
INFLVL          34       34      33      27      35      31      36      23      33      38      34      20      35      31      26      30      
----------------------------------------------------------------------------------------------------------------------------
MONTH= 4         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        582      553     566     564     597     578     568     523     508     577     588     542     591     553     555     568     
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 5         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 6         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 7         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 8         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 9         ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
----------------------------------------------------------------------------------------------------------------------------
MONTH= 10        ALIVE SQUIRRELS= 0      INFECTED SQUIRRELS= 0
CELL No          1       2       3       4       5       6       7       8       9       10      11      12      13      14      15      16
POPINFLX        0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0       
INFLVL          0        0       0       0       0       0       0       0       0       0       0       0       0       0       0       0  

```