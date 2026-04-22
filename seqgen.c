/* ========================================================================== /
// Sam Siewert, December 2017
// Modified by Trevor Sribar, 3/29/2026
// Modified by Trevor Sribar, 4/12/2026
// Modified by Trevor Sribar, 4/18/2026
//
//  Final project for running BB84
//  Service 1 = Servos
//  Service 2 = Laser/Photosensor
//  Service 3 = Encryption/Decryption
//  Service 4 = Keygen
//  Service 5 = UART
//  Service 6 = terminal
//
/ ========================================================================== */

// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

// file includes
#include "generic.h"
#include "keygen.h"
#include "sentenceLL.h"
#include "servo.h"

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
#define DELAY_SCHEDLUER_MSEC (struct timespec) {0,333333} // delay for 333.333 usec, 3000 Hz
#define TYPE_SENDER 0
#define TYPE_RECEIVOR 1
#define RPI_TYPE TYPE_SENDER
#define NUM_THREADS (6+1)
#define SERVO_PRIO              (6)
#define EXTERNAL_PRIO           (5)
#define ENCRYPT_DECRYPT_PRIO    (4)
#define KEYGEN_PRIO             (3)
#define UART_PRIO               (2)
#define TERMINAL_PRIO           (1)


// added code for finding WCET
#define FINDING_WCET TRUE
#if (FINDING_WCET == TRUE)
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 1000
#define NUM_TIMES_TEST 10000 // number of times run WCET for each function
#define PRINT 0
#define LOG 1
#define FIND_MODE PRINT
uint32_t WCET_task[NUM_THREADS] = {0};
void print_WCETs();
void *Service_WCET(void *threadp);
static inline void getElapsedTime(uint8_t task, struct timespec releaseTime, struct timespec completionTime);
#endif

// structs
typedef struct {
    int threadIdx;
    unsigned long long sequencePeriods;
} threadParams_t;

// variables
sem_t task_sems[NUM_THREADS];
uint8_t abort_service[NUM_THREADS] = {0};
int task_priorities[NUM_THREADS] = {0,SERVO_PRIO,EXTERNAL_PRIO,ENCRYPT_DECRYPT_PRIO,KEYGEN_PRIO,UART_PRIO,TERMINAL_PRIO};
sentenceLinkedList_t *addHead, *encryptHead, *sendHead; 
uint8_t servoPosition[ENCRYPTION_KEY_LENGTH*8];
uint8_t communicatedServoBasis[ENCRYPTION_KEY_LENGTH*8];
#if (RPI_TYPE == TYPE_RECEIVOR)
uint8_t sensedData[ENCRYPTION_KEY_LENGTH*8];
#endif


// function prototypes
void *Service_1_Servos(void *threadp);
void *Service_2_Periferal(void *threadp);
void *Service_3_Encrypt(void *threadp);
void *Service_4_Keygen(void *threadp);
void *Service_5_UART(void *threadp);
void *Service_6_Terminal(void *threadp);

void main(void)
{
    // Thread parameters
    threadParams_t threadParams[NUM_THREADS];       // thread info for paramaterizing threads
    pthread_t threads[NUM_THREADS];                 // thread info for creating threads  
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    struct sched_param rt_param[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    int rc;
    
    // main parameters
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;

    // logging
    struct timespec start_time;

    printf("Initializing BB84 Project\n");
    clock_gettime(CLOCK_MONOTONIC,&start_time); // start_time->tv_sec, start_time->tv_nsec
    syslog(LOG_INFO, "Initalization start:\tsec=%lu\tnsec=%lu\n", start_time.tv_sec, start_time.tv_nsec);

    // initalizing other files
    encryption_init();
    sentenceLL_init(&addHead, &encryptHead, &sendHead);
    servo_init();

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
        print_WCETs();
    #endif

    // joining threads and clearing semaphors
    for(uint8_t i=1;i<NUM_THREADS;i++){
        abort_service[i] = TRUE;
        sem_post(&task_sems[i]);
        pthread_join(threads[i], NULL);
        sem_destroy(&task_sems[i]);
    }
    encryption_destroy();
    sentenceLL_destroy(&sendHead);
    printf("\nEnd Program\n");
    return;
}

// services
// servo service
void *Service_1_Servos(void *threadp) 
{
    uint16_t servoMoveCount = 0;
    printf("Servo service started\t");

    while(!abort_service[1])
    {
        sem_wait(&task_sems[1]);
        servoPosition[servoMoveCount] = servo_set_angle_random();
        servoMoveCount++;
    }

    pthread_exit((void *)0);
}

// external periferal service TODO must fix modify change to do
void *Service_2_Periferal(void *threadp) 
{
    printf("external periferal service started\t");

    while(!abort_service[2])
    {
        sem_wait(&task_sems[2]);
    }

    pthread_exit((void *)0);
}

// Encryption service
void *Service_3_Encrypt(void *threadp) 
{
    printf("Encryption service started\t");

    while(!abort_service[3])
    {
        sem_wait(&task_sems[3]);
        // if we have any data to encrypt
        while(sentenceLL_getNumSentencesToEncrypt()!=0){
            // create a new nonce
            encryption_updateNonce();
            // encrypt the data where we need to
            encryption_encryptData(encryptHead.sentence, encryptHead.numCharacters);
            // say that we have encrypted the data
            sentenceLL_encryptedSentence(&encryptHead,encryption_getNonceAddress());
        }
    }

    pthread_exit((void *)0);
}

// keygen service
void *Service_4_Keygen(void *threadp) 
{
    printf("Keygen service started\t");

    while(!abort_service[4])
    {
        sem_wait(&task_sems[4]);
        // function changes based on whether we are the sender or receiver pi
        #if (RPI_TYPE == TYPE_SENDER)
        keygen_sender(servoPosition,communicatedServoBasis);
        #else
        keygen_receiver(sensedData,servoPosition,communicatedServoBasis);
        #endif
    }

    pthread_exit((void *)0);
}

// UART service TODO must fix modify change to do
void *Service_5_UART(void *threadp) 
{
    printf("UART service started\t");

    while(!abort_service[5])
    {
        sem_wait(&task_sems[5]);
        
    }

    pthread_exit((void *)0);
}

// terminal service TODO must fix modify change to do
void *Service_6_Terminal(void *threadp){
    printf("Terminal service started\t");

    while(!abort_service[6])
    {
        sem_wait(&task_sems[6]);
        
    }

    pthread_exit((void *)0);
}


#if (FINDING_WCET == TRUE)
// finds WCETs of each task individually
void *Service_WCET(void *threadp){
    printf("WCET finder started\n");
    syslog(LOG_INFO,("WCET finder started\n"));
    struct timespec startTime;
    struct timespec endTime;
    for(int i = 1; i < NUM_TIMES_TEST; i++){
        for(int j = 1; j < NUM_THREADS; j++){
            clock_gettime(CLOCK_MONOTONIC,&startTime);
            sem_post(&task_sems[j]);
            sched_yield();
            clock_gettime(CLOCK_MONOTONIC,&endTime);
            getElapsedTime(j,startTime,endTime);
        }
    }
    pthread_exit((void *)0);
}

// gets the epalsied time and adds it to the WCET if it is the greatest time that has elapsed for that function
static inline void getElapsedTime(uint8_t task, struct timespec releaseTime, struct timespec completionTime){
    uint32_t completionTimeS_inUS = (completionTime.tv_sec - releaseTime.tv_sec)*USEC_PER_SEC;
    uint32_t completionTimeUS = completionTimeS_inUS + (completionTime.tv_nsec - releaseTime.tv_nsec)/NSEC_PER_USEC;
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