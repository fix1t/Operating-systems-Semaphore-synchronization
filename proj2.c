#include <stdio.h>


#define EXPECTED_ARGS 5


usage()
{
    printf("Program must start with these arguments ./proj2 NE NR TE TR\n"
    "NO Number of oxygens \n"
    "NH Number of hydrogens \n"
    "TI Maximal time in ms H/O must wait to get to queue. 0<=TI<=1000\n"
    "TB Maximal time in ms to make a molekule. 0<=TB<=1000\n");
}

int main(int argc, char const *argv[])
{
    // TODO readArg();
    // read arguments NO NH TI TB
    // if bad input exit -1 && free alocated 

    // every semaphore must be chcecked for failure

    // kazdy proces zapisuje svoje akce do proj2.out 
    // synchronizuju sa s poradovym cislom 

    // implementace citace , sdilena pamet 

    if (argc != EXPECTED_ARGS) {
    fprintf(stderr, "invalid amount of arguments given.\n");
    usage();
    return -1;
    }
 


// MAIN process
    // immediately starts pumping Os & Hs
    // then waits for all processes to finsih and exits with value of 1

// OXIGEN process 
    // attach number oID to O  - 0-NO
    
    // printf("O %d: started", oID);
    
    // TODO funkcia pre spanok 0-TI 
    // sleepRandom() ;

    // printf("O %d: going to queue", oID);
    //OxygenQueue++; added to queue

    // semaphore to wait for 1 oxygen and 2 Hydrogens

//creating molecule process 
    // usleep(/* interval random 0 - TB */);


    return 0;
}
