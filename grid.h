#include "mpi.h"
#include "pool.h"
#include "parameters.h"
#include <stdio.h>
#include "squirrel-functions.h"


static void calcPopInfux(int populationStore[NCELL][3], int popInfluxLvl[NCELL]);
static void calcInfectionLvl(int infectionStore[NCELL][2], int infectionLvl[NCELL]);
void Grid_work(int);

