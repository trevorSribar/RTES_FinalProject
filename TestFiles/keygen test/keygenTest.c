// this file creates three tasks, one for encrytion and one for decryption that uses the linked list with a max character length sentence
// the third task generates a key the same way it would be generated given made-up servo and sensor data, and prints if the keys don't match
// the third task releases the first two when it is done

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
#include "encryption.h"
#include "sentenceLL.h"

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
#define TRUE 1
#define FALSE 0
#define TYPE_SENDER 0
#define TYPE_RECEIVOR 1
#define RPI_TYPE TYPE_SENDER
#define NUM_THREADS (3+1)
#define SERVO_PRIO              (6)
#define EXTERNAL_PRIO           (5)
#define ENCRYPT_DECRYPT_PRIO    (4)
#define KEYGEN_PRIO             (3)
#define UART_PRIO               (2)
#define TERMINAL_PRIO           (1)


// added code for finding WCET
#define FINDING_WCET FALSE
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
int task_priorities[NUM_THREADS] = {0,SERVO_PRIO,EXTERNAL_PRIO,KEYGEN_PRIO};
sentenceLinkedList_t *addHead, *encryptHead, *sendHead; 
uint8_t servoSendAllData[ENCRYPTION_KEY_LENGTH*8];
uint8_t servoReceiveAllData[ENCRYPTION_KEY_LENGTH*8];
uint8_t servoReceuveMeasured[ENCRYPTION_KEY_LENGTH*8];
uint8_t servoSendBasis[ENCRYPTION_KEY_LENGTH*8];
uint8_t servoReceiveBasis[ENCRYPTION_KEY_LENGTH*8];


// function prototypes
void *Service_1_Encrypt(void *threadp);
void *Service_2_Decrypt(void *threadp);
void *Service_3_Keygen(void *threadp);

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

    printf("Encryption/Decryption Test\n");
    clock_gettime(CLOCK_MONOTONIC,&start_time); // start_time->tv_sec, start_time->tv_nsec
    syslog(LOG_INFO, "Initalization start:\tsec=%lu\tnsec=%lu\n", start_time.tv_sec, start_time.tv_nsec);

    // initalizing other files
    if(encryption_init()==ENCRYPTION_ERROR) {return;}
    sentenceLL_init(&addHead, &encryptHead, &sendHead);

    // configuring the main thread
    mainpid=getpid();
    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);
    rc = sched_getparam(mainpid, &main_param);
    if(rc < 0) perror("getting main_param");
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
        if(i!=0){
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

    rc=pthread_create(&threads[1],&rt_sched_attr[1],Service_1_Encrypt, (void *)&(threadParams[1]));
    if(rc < 0)  { perror("\nError creating service 1");}
    else        {printf("\tS1");}

    rc=pthread_create(&threads[2],&rt_sched_attr[2],Service_2_Decrypt, (void *)&(threadParams[2]));
    if(rc < 0)  { perror("\nError creating service 2");}
    else        {printf("\tS2");}

    rc=pthread_create(&threads[3],&rt_sched_attr[3],Service_3_Keygen, (void *)&(threadParams[3]));
    if(rc < 0)  { perror("\nError creating service 2");}
    else        {printf("\tS3");}
 
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
    printf("\n\r");

    // creating the servo data
    for(uint16_t i = 0; i < ENCRYPTION_KEY_LENGTH*8;i++){
        servoSendAllData[i] = rand()%4;
        servoReceiveAllData[i] = rand()%4;
        servoReceuveMeasured[i] = (servoSendAllData[i]==servoReceiveAllData[i]);
        servoSendBasis[i] = servoSendAllData[i]%2;
        servoReceiveBasis[i] = servoReceiveAllData[i]%2;
    }
    // creating sentences to send and releasing the tasks
    char sentence[] = {"Do what you must to get this project done, cuz you are presenting on Thursday and that is too close."};
    uint8_t length = 100;
    for(uint8_t i = 0; i < length; i++){
        printf("%c",sentence[i]);
    }
    printf("\n\r");
    if(sentenceLL_addSentence(&addHead, sentence, length)==SENTENCELL_ERROR) {printf("Failed to add sentence\n\r");};
    if(sentenceLL_addSentence(&addHead, sentence, length)==SENTENCELL_ERROR) {printf("Failed to add sentence\n\r");};
    if(sentenceLL_addSentence(&addHead, sentence, length)==SENTENCELL_ERROR) {printf("Failed to add sentence\n\r");};
    sleep(2);
    sem_post(&task_sems[3]);
    sleep(2);

    // joining threads and clearing semaphors
    for(uint8_t i=1;i<NUM_THREADS;i++){
        abort_service[i] = TRUE;
        sem_post(&task_sems[i]);
        pthread_join(threads[i], NULL);
        sem_destroy(&task_sems[i]);
    }
    encryption_destroy();
    printf("\nEnd Program\n");
    return;
}

// services

// Dencryption service
void *Service_2_Decrypt(void *threadp) 
{
    while(!abort_service[2])
    {
        sem_wait(&task_sems[2]);
        printf("decryption service started\n\r");
        while(sentenceLL_getNumSentencesToSend()!=0){
            // decrypt the data
            if(encryption_decryptData(sendHead->sentence, sendHead->numCharacters, sendHead->sentenceNonce)==ENCRYPTION_ERROR){
                printf("Decryption Error\n");
                break;
            }
            printf("valid sentence\n\r");
            for(uint16_t i = 0; i < sendHead->numCharacters; i++){
                printf("%c",sendHead->sentence[i]);
            }
            printf("\n\r");
            if(sentenceLL_removeSentence(&sendHead)==SENTENCELL_ERROR) {break;}
        }
        abort_service[2] == TRUE;
    }

    pthread_exit((void *)0);
}

// Encryption service
void *Service_1_Encrypt(void *threadp) 
{
    while(!abort_service[1])
    {
        sem_wait(&task_sems[1]);
        printf("Encryption service started\n\r");
        // if we have any data to encrypt
        while(sentenceLL_getNumSentencesToEncrypt()!=0){
            // create a new nonce
            encryption_updateNonce();
            // encrypt the data where we need to
            if(encryption_encryptData(encryptHead->sentence, encryptHead->numCharacters)==ENCRYPTION_ERROR){
                printf("Encryption Error\n");
                break;
            }
            for(uint8_t i = 0; i < encryptHead->numCharacters; i++){
                printf("%u",encryptHead->sentence[i]);
            }
            printf("\n\r");
            // say that we have encrypted the data
            if(sentenceLL_encryptedSentence(&encryptHead,encryption_getNonceAddress())==SENTENCELL_ERROR) {break;}
        }
        sem_post(&task_sems[2]);
        printf("\n\rEncryption done\n\r");
        abort_service[1] == TRUE;
    }

    pthread_exit((void *)0);
}

// keygen service
void *Service_3_Keygen(void *threadp) 
{

    while(!abort_service[3])
    {
        sem_wait(&task_sems[3]);
        printf("Keygen service started\n\r");
        keygen_sender(servoSendAllData,servoReceiveBasis);
        char tempKey[ENCRYPTION_KEY_LENGTH];
        char *key;
        key = encryption_getKey();
        for(uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++){
            tempKey[i]=key[i];
        }
        keygen_receiver(servoReceuveMeasured,servoReceiveAllData,servoSendBasis);
        key = encryption_getKey();
        for(uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++){
            if(tempKey[i]!=key[i]){
                printf("Key error #%d\tK1: %d\tK2: %d\n\r",i,tempKey[i],key[i]);
            }
        }
        printf("\n\rKeygen done\n\r");
        sem_post(&task_sems[1]);
        abort_service[3] == TRUE;
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