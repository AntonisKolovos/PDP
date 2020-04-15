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
