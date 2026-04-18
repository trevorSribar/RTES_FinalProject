/* ========================================================================== /
// Sam Siewert, December 2017
// Modified by Trevor Sribar, 3/29/2026
// Modified by Trevor Sribar, 4/12/2026
// Modified by Trevor Sribar, 4/18/2026
//
//  Final project for running BB84
//
/ ========================================================================== */

// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

// file includes
#include "generic.h"
#include "encryption.h"

// includes for tasks, schedueling, and logging
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <syslog.h>
#include <sys/time.h>
#include <time.h>

// includes for errors
#include <errno.h>

// defines
#define TRUE (1)
#define FALSE (0)
#define NUM_THREADS (1+1)
#define DELAY_33P33_MSEC (struct timespec) {0,333333} // delay for 333.333 usec, 3000 Hz


// added code for finding WCET of each task and missed deadlines
#define USEC_PER_SEC 1000000
#define PRINT 0
#define LOG 1
#define FIND_MODE PRINT
#define FINDING_WCET TRUE
#define FIND_MISSES FALSE

#if (FINDING_WCET == TRUE)
#define DELAY_500_US (struct timespec) {0,500000};
#define NUM_TIMES_TEST 10000 // number of times run WCET for each function
uint32_t WCET_task[NUM_THREADS] = {0};
void print_WCETs();
void *Service_WCET(void *threadp);
static inline void getElapsedTime(uint8_t task, struct timeval releaseTime, struct timeval completionTime);
#endif

// added code for testing encryption
char input[80];
char output[80];
// added code for testing keygen
char sendServo[256];
char receiveServo[256];
// end added code

//
// variables
//
sem_t task_sems[NUM_THREADS-1];
int task_priorities[NUM_THREADS]; // this needs to be initialized
struct timeval start_time_val;
int abortS1=FALSE;

// structs
typedef struct
{
    int threadIdx;
    unsigned long long sequencePeriods;
} threadParams_t;

// function prototypes
void *Sequencer(void *threadp);
void *Service_1(void *threadp);
double getTimeMsec(void);
void print_scheduler(void);


void main(void)
{
    // Thread parameters
    threadParams_t threadParams[NUM_THREADS];       // thread info for paramaterizing threads
    pthread_t threads[NUM_THREADS];                 // thread info for creating threads  
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    struct sched_param rt_param[NUM_THREADS];
    int rt_max_prio, rt_min_prio;

    // function variables
    struct timespec start_time;
    int i, rc, scope;
    
    // main parameters
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;

    // logging
    printf("Initializing BB84 Project\n");
    clock_gettime(CLOCK_MONOTONIC,&start_time); // start_time->tv_sec, start_time->tv_nsec
    syslog(LOG_INFO, "Initalization start:\tsec=%d\tnsec=%d\n", start_time->tv-sec, start_time->tv_nsec);

    // configuring the main thread
    mainpid=getpid();
    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);
    rc = sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");


    // thread initialization
    for(uint8_t i = 0; i<NUM_THREADS; i++){
        
        // task initialization
        rc=pthread_attr_init(&rt_sched_attr[i]);
        rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
        rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
        rt_param[i].sched_priority=task_priorities[i];
        pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
        threadParams[i].threadIdx=i;

        // task semaphor initalization
        if(i<NUM_THREADS-1){
            if(sem_init (&task_sems[i], 0, 0)) { printf ("Failed to initialize semaphore number %d\n",i); exit (-1); }
        }
    }

    // assigning functions to each thread, hard to do this in loop
    // rc=pthread_create(&threads[1],             // pointer to thread descriptor
    //                 &rt_sched_attr[1],         // use specific attributes
    //                 //(void *)0,               // default attributes
    //                 Service_1,                 // thread function entry point
    //                 (void *)&(threadParams[1]) // parameters to pass in
    //                 );
    
    // thread creation
    printf("pthread created: ");

    rc=pthread_create(&threads[1],&rt_sched_attr[1],Service_1, (void *)&(threadParams[1]));
    if(rc < 0)  { perror("\nError creating service 1");}
    else        {printf("\tS1");}
 
    // Finding WCETs
    #if (FINDING_WCET == TRUE)
        threadParams[0].sequencePeriods = 0;
        rt_param[0].sched_priority = rt_min_prio;
        pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
        rc = pthread_create(&threads[0], &rt_sched_attr[0], Service_WCET, (void *)&(threadParams[0]));
        if(rc < 0)  { perror("\nError creating service WCET");}
        else        {printf("\tWCET");}
        pthread_join(threads[0],NULL);
        printf("WCET joined\n");
    #endif

    for(i=1;i<NUM_THREADS;i++)
        pthread_join(threads[i], NULL);
    sem_post(&semS1);
    mbedtls_chacha20_free(&ctx);
    print_WCETs();
    printf("\nTEST COMPLETE\n");
    return;
}

// services
void *Service_1(void *threadp)
{
    struct timeval current_time_val;
    unsigned long long S1Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    gettimeofday(&current_time_val, (struct timezone *)0);
    printf("Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS1)
    {
        sem_wait(&semS1);
        for(uint16_t i = 0; i < 256; i++){
            if(sendServo[i]==0){
                if(receiveServo[i]==0){
                    sendServo[i]=0;
                }
                else if(receiveServo[i]==1){
                    sendServo[i]=1;
                }
                else{
                    sendServo[i]=0;
                }
            }
            else if(sendServo[i]==1){
                if(receiveServo[i]==0){
                    sendServo[i]=0;
                }
                else if(receiveServo[i]==1){
                    sendServo[i]=1;
                }
                else{
                    sendServo[i]=0;
                }
            }
            else if(sendServo[i]==2){
                if(receiveServo[i]==2){
                    sendServo[i]=0;
                }
                else if(receiveServo[i]==3){
                    sendServo[i]=1;
                }
                else{
                    sendServo[i]=0;
                }
            }
            else if(sendServo[i]==3){
                if(receiveServo[i]==2){
                    sendServo[i]=0;
                }
                else if(receiveServo[i]==3){
                    sendServo[i]=1;
                }
                else{
                    sendServo[i]=0;
                }
            }
        }
        // if(mbedtls_chacha20_crypt(key, nonce, counter, 80, input, output)!=0){
        //     printf("Encrypt Error\n\r");
        // }

    }

    pthread_exit((void *)0);
}




#if (FINDING_WCET == TRUE)
// finds WCETs of each task individually
void *Service_WCET(void *threadp){
    printf("WCET finder started\n");
    syslog(LOG_INFO,("WCET finder started\n"));
    struct timeval startTime;
    struct timeval endTime;
    for(int i = 1; i < NUM_TIMES_TEST; i++){
        //re generate nonce, input
        for(uint8_t i = 0; i < 12; i++){
            nonce[i] = rand();
        }
        for(uint8_t i = 0; i < 80; i++){
            input[i] = rand();
        }
        for(uint16_t i = 0; i < 256; i++){
            sendServo[i] = rand();
            receiveServo[i] = rand();
        }
        gettimeofday(&startTime,(struct timezone *)0);
        sem_post(&semS1);
        sched_yield();
        gettimeofday(&endTime,(struct timezone *)0);
        getElapsedTime(1,startTime,endTime);
    }
    abortS1=TRUE;
    sem_post(&semS1);
    pthread_exit((void *)0);
}

// gets the epalsied time and adds it to the WCET if it is the greatest time that has elapsed for that function
static inline void getElapsedTime(uint8_t task, struct timeval releaseTime, struct timeval completionTime){
    uint32_t completionTimeS_inUS = (completionTime.tv_sec - releaseTime.tv_sec)*USEC_PER_SEC;
    uint32_t completionTimeUS = completionTimeS_inUS + completionTime.tv_usec - releaseTime.tv_usec;
    if(completionTimeUS > WCET_task[task]){
        WCET_task[task] = completionTimeUS;
    }
}

// prints the WCETs
void print_WCETs(){
    for(int i = 0; i < NUM_THREADS;i++){
        #if (FIND_MODE == LOG)
        syslog(LOG_INFO,"WCET Task %u:\t%u\n",i,WCET_task[i]);
        #else
        printf("WCET Task %u:\t%u\n",i,WCET_task[i]);
        #endif
    }
}
#endif // (FINDING_WCET == TRUE)