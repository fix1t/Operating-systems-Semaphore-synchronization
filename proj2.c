/**
 * @file proj2.c
 * @author Gabriel Biel (xbielg00)
 * @brief VUT FIT IOS II
 * @version 1.0
 * @date 2022-04-24
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <semaphore.h>
#include <time.h>


#define MAPIT(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);} // memory alloc 
#define UNMAPIT(pointer) {munmap(pointer, sizeof(pointer));} // free memory 


// global variables
int *line; /*< Number of current line */
int *molCount; /*< Number of current molecule */
int *hydroCount; /*< Hydrogen counter every second H may call O */
int *hydroRem; /*< Number of remaining Hs */
int *oxyRem; /*< Number of remaining Os */
int *pCounter; /*< Process counter */

typedef struct
{
    int ID; 
    char type;
    int timeI; /*< TI argument */
    int timeB; /*< TB argument */
}element_t;


FILE *file; /*< Output file*/

sem_t *mutex = NULL; /*< Binary semaphore to allow writing*/
sem_t *molCreation = NULL; /*< Only 1 molecule can be created at a time*/
sem_t *molCreated = NULL; /*< Hs wait for molecule to be created */
sem_t *oxyQueue = NULL; 
sem_t *hydroQueue = NULL;
sem_t *barrier = NULL; /*< Allows only 2 Hs to create a molecule at a time*/
sem_t *finishMain = NULL; /*< Process 0 (main) waits for other processes to finsih */



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
 * @brief Unlink used semaphores
 * 
 */
void unlinkSemaphores()
{
    sem_unlink("xbielg00.sem.barrier");
    sem_unlink("xbielg00.sem.molCreation");
    sem_unlink("xbielg00.sem.molCreated");
    sem_unlink("xbielg00.sem.oxyQueue");
    sem_unlink("xbielg00.sem.hydroQueue");
    sem_unlink("xbielg00.sem.mutex");
    sem_unlink("xbielg00.sem.finishMain");
}
/**
 * @brief Close used semaphores and free shared memory
 * 
 */
void cleanup()
{
    sem_close(hydroQueue);
    sem_close(oxyQueue);
    sem_close(molCreation);
    sem_close(molCreated);
    sem_close(barrier);
    sem_close(mutex);
    sem_close(finishMain);

    UNMAPIT(line) /*< shared memory line number */
    UNMAPIT(molCount)
    UNMAPIT(hydroCount)
    UNMAPIT(oxyRem)
    UNMAPIT(hydroRem)
    UNMAPIT(hydroRem)
}

/**
 * @brief Initiate semaphores, open file, allocate memory for variables
 *  
 * @return int Return -1 if unsuccessful and 0 if successful
 */
int init()
{
    MAPIT(line) 
    MAPIT(molCount)
    MAPIT(hydroCount)
    MAPIT(oxyRem)
    MAPIT(hydroRem)
    MAPIT(pCounter)

    srand(time(NULL));

    *line = 1;      
    *molCount = 0;
    *hydroCount = 0;
    *pCounter = 0;

    unlinkSemaphores();

    file = fopen("proj2.out", "w");

    setbuf(file, NULL);

    if(file == NULL)
    {
        fprintf(stderr, "Error: Output file cannot be opened.\n");
        return 1;
    }
    if ((finishMain = sem_open("xbielg00.sem.finishMain", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        return 1;
    if ((molCreation = sem_open("xbielg00.sem.molCreation", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
        return 1;
    if ((molCreated = sem_open("xbielg00.sem.molCreated", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        return 1;
    if ((barrier = sem_open("xbielg00.sem.barrier", O_CREAT | O_EXCL, 0666, 2)) == SEM_FAILED)
        return 1;
    if ((oxyQueue = sem_open("xbielg00.sem.oxyQueue", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        return 1;
    if ((hydroQueue = sem_open("xbielg00.sem.hydroQueue", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        return 1;
    //binary lock - writing is enable for only 1 element at a time
    if ((mutex = sem_open("xbielg00.sem.mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
        return 1;
    return 0;
}

/**
 * @brief Bonding to molecule, print creating molecule, simulate creation, print molecule created
 * 
 * @param element Element H or O of which molecule is created 2Hs and 1O is needed
 * @param molCount Number of molecule that is being created 
 */
void bond(element_t* element, int molCount)
{
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : %c %d: creating molecule %d\n",*line,element->type,element->ID,molCount);
    *line += 1;
    sem_post(mutex); //open for other writing


    //"H waits for signal from O, that creation of molecule is done"
    if (element->type == 'O')
    {
        if (element->timeB != 0)
            //creating molecule ...
            usleep(1000*rand()%element->timeB);
        //signal 2 Hs that molecule creation is done
        sem_post(molCreated);
        sem_post(molCreated);
    }
    else
        sem_wait(molCreated);   // H waits for O to signal when molecule is created

    sem_wait(mutex); //wait to write
    fprintf(file,"%d : %c %d: molecule %d created\n",*line,element->type,element->ID,molCount);
    *line += 1;
    sem_post(mutex); //open for other writing
    return;
}

/**
 * @brief Synchorized attemp of oxygen to bond with hydorogens to create a molecule H2O 
 * 
 * @param element Oxygen element  
 */
void oxyProcess(element_t* element)
{
    // started
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : O %d: started\n",*line, element->ID);
    *line += 1;
    sem_post(mutex); //open for other writing

    // sleep
if (element->timeI != 0)
        usleep(1000*rand()%element->timeI);

    // going to queue
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : O %d: going to queue\n",*line, element->ID);
    *line += 1;
    sem_post(mutex); //open for other writing

    if (*oxyRem == 1 && *hydroRem == 1 )
    {
        sem_wait(mutex); //wait to write
        fprintf(file,"%d : O %d: not enough H\n",*line, element->ID);
        *line += 1;
        sem_post(mutex); //open for other writing
        return ;
    }
    

    // in queue ...
    sem_wait(oxyQueue); //waits for 2 Hs to release 1 oxygen  

    if (*hydroRem < 2) //if another molecule can not be created 
    {
        // alternative finish, not enough H
        sem_wait(mutex); //wait to write
        fprintf(file,"%d : O %d: not enough H\n",*line, element->ID);
        *line += 1;
        sem_post(mutex); //open for other writing
        return ;
    }

    // set up remainings 
    *oxyRem -=1;
    *hydroRem -=2;

    sem_wait(molCreation); //only 1 molecule could be created at once

    *molCount += 1; //increase molecule count
    sem_post(hydroQueue); //lets 2 Hs of queue 
    sem_post(hydroQueue);

    bond(element, *molCount);

    sem_post(molCreation); //another molecule can be created

    // last one to bond frees the otheres waiting in queue
    if (*hydroRem < 2 )
    {
        for (int i = 0; i < *oxyRem; i++)
            sem_post(oxyQueue);
    }
    return ;    
}

/**
 * @brief Synchorized attemp of hydrogen to bond with hydorogens to create a molecule H2O 
 * 
 * @param element Hydrogen element  
 */
void hydroProcess(element_t *element)
{
    // started
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : H %d: started\n",*line,element->ID);
    *line += 1; 
    sem_post(mutex); //open for other writing
    
    // sleep
    if (element->timeI != 0)
        usleep(1000*rand()%element->timeI);

    // going to queue
    sem_wait(mutex); //wait to write
    fprintf(file,"%d : H %d: going to queue\n",*line,element->ID);
    *line += 1;
    sem_post(mutex); //open for other writing

    // only two Hs, may pass at once
    sem_wait(barrier);

    // alternative finish, when not enough elements are in place
    if (*oxyRem == 0 || *hydroRem == 1) 
    {
        sem_wait(mutex); //wait to write
        fprintf(file,"%d : H %d: not enough O or H\n",*line,element->ID);
        *line += 1;
        sem_post(mutex); //open for other writing
        return ;
    }
    
    *hydroCount += 1; //2nd H frees O

    if (*hydroCount >= 2)
        sem_post(oxyQueue);
    
    sem_wait(hydroQueue); //only 1 oxygen is let in 

    *hydroCount -= 1;       

    bond(element, *molCount);
    
    // No oxygen remaining, free others
    if (*oxyRem == 0) 
    {
        for (int i = 0; i <= *hydroRem; i++)
        {
            sem_post(barrier);
        }
    }

    sem_post(barrier); //let another H int 
    return ;
}



int main(int argc, char const *argv[])
{
    // check if correct number of arguments were passed in
    if (argc != 5) 
    {
    fprintf(stderr, "ERROR: Invalid amount of arguments.\n");
    usage();
    return 1;
    }

    // parse arguments
    int NO = atoi(argv[1]); /*< Number of Oxygens*/
    int NH = atoi(argv[2]); /*< Number of Hydrogens*/
    int TI = atoi(argv[3]); /*< Max time to queue*/
    int TB = atoi(argv[4]); /*< Max time to make molecule*/

    // check for invalid arguments
    if (NO<=0 || NH<=0 || TI<0 || TI>1000 || TB<0 || TB>1000)
    {
        fprintf(stderr, "ERROR: Invalid argumet's size."
        "NO > 0, NH > 0, 0 < TI < 1000, 0 < TB < 1000");
        return 1;
    }

    // initiate program
    if (init())
    {
        cleanup(); //if unsuccessful (ret 1) free memory 
        return 1;
    }
    *hydroRem = NH;
    *oxyRem = NO; 

    //setup struct for each element
    element_t element;
    element.timeB = TB;
    element.timeI = TI;

    //generate Os
    for (int i = 1; i <= NO; i++)
    {
        element.type = 'O';
        element.ID = i;

        pid_t pid = fork();
        if (pid == 0) // child do ...
        {
            oxyProcess(&element);
            if (++(*pCounter) == NO+NH) // check if you are last, then unlock process 0
                sem_post(finishMain);
            return 0; // child dies
        }
    }

    //generate Hs
    for (int i = 1; i <= NH; i++)
    {
        element.type = 'H';
        element.ID = i;

        pid_t pid = fork();
        if (pid == 0) //child do ...
        {
            hydroProcess(&element);
            if (++(*pCounter) == NO+NH) // check if you are last, then unlock process 0
                sem_post(finishMain);
            return 0;
        }
    }
    
    sem_wait(finishMain); // wait for last child to finish

    cleanup();

    unlinkSemaphores();

    return 0;
}
