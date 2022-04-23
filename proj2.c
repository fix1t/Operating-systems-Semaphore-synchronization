#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <semaphore.h>
#include <time.h>


#define EXPECTED_ARGS 5
#define MAPIT(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);} // memory alloc 
#define UNMAPIT(pointer) {munmap(pointer, sizeof(pointer));} // free memory 


/**
 * @brief Global variables 
 * 
 */
int *line =0;
int *hydroCount = 0;
int *oxyCount = 0;

FILE *file;

sem_t *mutex = NULL;
sem_t *molCreation = NULL;
sem_t *oxyQueue = NULL;
sem_t *hydroQueue = NULL;



/**
 * @brief Prints usage
 * 
 */
void usage()
{
    printf("Program must start with these arguments ./proj2 NE NR TE TR\n"
    "NO Number of oxygens \n"
    "NH Number of hydrogens \n"
    "TI Maximal time in ms H/O must wait to get to queue. 0<=TI<=1000\n"
    "TB Maximal time in ms to make a molekule. 0<=TB<=1000\n");
}

/**
 * @brief Function to initiate all semaphores, files, variables
 * 
 * @return int Return -1 if unsuccessful and 0 if successful
 */
int init()
{
    //unlinked in case any of semaphores were in use
    sem_unlink(molCreation);
    sem_unlink(oxyQueue);
    sem_unlink(hydroQueue);
    sem_unlink(mutex);

    MAPIT(line) /*< shared memory line number */
    MAPIT(oxyCount)
    MAPIT(hydroCount)


    file = fopen("proj2.out", "w");
    
    //setbuf(file, NULL);

    if(file == NULL)
    {
        fprintf(stderr, "Error: Output file cannot be opened.\n");
        exit(-1);
    }
    if ((molCreation = sem_open(molCreation, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        return -1;
    }
    if ((oxyQueue = sem_open(oxyQueue, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        return -1;
    }
    if ((hydroQueue = sem_open(hydroQueue, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        return -1;
    }
    //binary lock - writing is enable for only 1 element at a time
    if ((mutex = sem_open("xbielg00.sem.mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief TODO
 * 
 */
void failure()
{
    /* TODO */
}


int oxyProcess(int oID)
{
    
    sem_wait(mutex); //wait to write
    printf("%d : O %d: started",*line,oID);
    *line++;
    sem_post(mutex); //open for others

/* change to random */    sleep(1);
    sem_wait(mutex); //wait to write
    printf("%d : O %d: going to queue",*line,oID);
    *line++;
    sem_post(mutex); //open for others

    *oxyCount++; //number of oxygens in queue
    //in queue ...
    sem_wait(oxyQueue); //only 1 oxygen is let in 
    sem_wait(molCreation); //when when no other molecule is being created go further
    if (*hydroCount >= 2)
    {
        sem_post(hydroQueue);
        sem_post(hydroQueue);
        
    }
    
    
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


    /**
     * @brief chceck if arguments were entered correctly
     * 
     */
    if (argc != EXPECTED_ARGS) {
    fprintf(stderr, "ERROR: Invalid amount of arguments.\n");
    usage();
    return 1;
    }
    // parse arguments
    int NO = stoi(argv[1]); /*< Number of Oxygens*/
    int NH = stoi(argv[2]); /*< Number of Hydrogens*/
    int TI = stoi(argv[3]); /*< Max time to queue*/
    int TB = stoi(argv[4]); /*< Max time to make molecule*/
 
    /**
     * @brief check if argumets are expected size
     * 
     */
    if (NO<=0 || NH<=0 || TI<0 || TI>1000 || TB<0 || TB>1000)
    {
        fprintf(stderr, "ERROR: Invalid argumet's size."
        "NO > 0, NH > 0, 0 < TI < 1000, 0 < TB < 1000");
        return 1;
    }
    
    initiate(); //sets up semaphores




// MAIN process
    // immediately starts pumping Os & Hs
    // then waits for all processes to finsih and exits with value of 1


    //generate all Hydorogens
    for (int i = 1; i <= NH; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            hydroProcess();
        }
        else if (pid <= 0)
        {
            failure();
        }
    }
    //generate all Oxygens
    for (int i = 1; i <= NO; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            oxyProcess(i);
        }
        else if (pid <= 0)
        {
            failure();
        }
    }
    
    

//creating molecule process 
    // usleep(/* interval random 0 - TB */);


    return 0;
}
