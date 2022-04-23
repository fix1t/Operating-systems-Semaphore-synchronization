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
int *molCount =0;
int *hydroCount = 0;
int *oxyCount = 0;

FILE *file;

sem_t *mutex = NULL;
sem_t *molCreation = NULL;
sem_t *oxyQueue = NULL;
sem_t *hydroQueue = NULL;
sem_t *barrier = NULL;



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
    // sem_unlink(molCreation);
    // sem_unlink(oxyQueue);
    // sem_unlink(hydroQueue);
    // sem_unlink(mutex);

    MAPIT(line) /*< shared memory line number */
    MAPIT(molCount)
    MAPIT(oxyCount)
    MAPIT(hydroCount)


    file = fopen("proj2.out", "w");
    
    //setbuf(file, NULL);

    if(file == NULL)
    {
        fprintf(stderr, "Error: Output file cannot be opened.\n");
        exit(-1);
    }
    if ((molCreation = sem_open("xbielg00.sem.molCreation", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        return -1;
    }
    if ((barrier = sem_open("xbielg00.sem.barrier", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        return -1;
    }
    if ((oxyQueue = sem_open("xbielg00.sem.oxyQueue", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        return -1;
    }
    if ((hydroQueue = sem_open("xbielg00.sem.hydroQueue", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
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
void bond(int ID, char type, int molCount)
{
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : %c %d: creating molecule %d\n",*line,type,ID,molCount);
    *line++;
    sem_post(mutex); //open for others

    sleep(0.5); //sleep random ... 

    sem_wait(mutex); //wait to write
    fprintf(file,"%d : %c %d: molecule %d created\n",*line,type,ID,molCount);
    *line++;
    sem_post(mutex); //open for others
    return;
}

int oxyProcess(int oID)
{
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : O %d: started\n",*line,oID);
    *line++;
    sem_post(mutex); //open for others

/* change to random */    sleep(1);

    sem_wait(mutex); //wait to write
    fprintf(file,"%d : O %d: going to queue\n",*line,oID);
    *line++;
    sem_post(mutex); //open for others

    *oxyCount++; //number of oxygens in queue
    //in queue ...
    sem_wait(oxyQueue); //waits for 2 Hs  

    sem_wait(molCreation); //locks mol creation
    *molCount++;
    *oxyCount--;
    sem_post(hydroQueue); //let 2 Hs of queue 
    sem_post(hydroQueue);
    bond(oID, "O", *molCount);
    return 0;    
}
int hydroProcess(int hID)
{
    
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : O %d: started\n",*line,hID);
    *line++;
    sem_post(mutex); //open for others

/* change to random */    sleep(0.5);
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : O %d: going to queue\n",*line,hID);
    *line++;
    sem_post(mutex); //open for others

    *hydroCount++; //number of oxygens in queue
    //in queue ...
    if (*hydroCount >= 2)
        sem_post(oxyQueue);
    
    sem_wait(hydroQueue); //only 1 oxygen is let in 

    *hydroCount--;
    bond(hID, "H", *molCount);
    return 0;
}



int main(int argc, char const *argv[])
{
    /**
     * @brief chceck if arguments were entered correctly
     * 
     */
    // if (argc != EXPECTED_ARGS) {
    // fprintf(stderr, "ERROR: Invalid amount of arguments.\n");
    // usage();
    // return 1;
    // }
    // // parse arguments
    // int NO = stoi(argv[1]); /*< Number of Oxygens*/
    // int NH = stoi(argv[2]); /*< Number of Hydrogens*/
    // int TI = stoi(argv[3]); /*< Max time to queue*/
    // int TB = stoi(argv[4]); /*< Max time to make molecule*/
    // /**
    //  * @brief check if argumets are expected size
    //  * 
    //  */
    // if (NO<=0 || NH<=0 || TI<0 || TI>1000 || TB<0 || TB>1000)
    // {
    //     fprintf(stderr, "ERROR: Invalid argumet's size."
    //     "NO > 0, NH > 0, 0 < TI < 1000, 0 < TB < 1000");
    //     return 1;
    // }
    
    int NO = 2; /*< Number of Oxygens*/
    int NH = 4; /*< Number of Hydrogens*/

    init(); //sets up semaphores

    //generate all Hydorogens
    for (int i = 1; i <= NH; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            hydroProcess(i);
            return 0;
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
            return 0;
        }
        else if (pid <= 0)
        {
            failure();
        }
    }
    
    //wait(NULL); //waits for the last child
    sleep(3);
    UNMAPIT(line) /*< shared memory line number */
    UNMAPIT(molCount)
    UNMAPIT(oxyCount)
    UNMAPIT(hydroCount)

    sem_close(hydroQueue);
    sem_close(oxyQueue);
    sem_close(molCreation);
    sem_close(barrier);
    sem_close(mutex);

    sem_unlink("xbielg00.sem.barrier");
    sem_unlink("xbielg00.sem.molCreation");
    sem_unlink("xbielg00.sem.oxyQueue");
    sem_unlink("xbielg00.sem.hydroQueue");
    sem_unlink("xbielg00.sem.mutex");
    return 0;
}


