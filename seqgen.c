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
#include "keygen.h"
#include "servo.h"
#include "terminal.h"
#include "uart.h"
#include "adc.h"
#include "laser.h"

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
#define SERVO_PRIO              (5)
#define EXTERNAL_PRIO           (6)
#define ENCRYPT_DECRYPT_PRIO    (4)
#define KEYGEN_PRIO             (3)
#define UART_PRIO               (2)
#define TERMINAL_PRIO           (1)

#define NS_PER_USEC             1000
#define NS_PER_MS               1000000
#define NS_PER_SEC              (NS_PER_MS * 1000)
#define SERVO_MOVE_TIME         (425*NS_PER_MS)
#define HALF_SERVO_MOVE_TIME    (SERVO_MOVE_TIME/2)

// added code for finding WCET
#define FINDING_WCET TRUE
#if (FINDING_WCET == TRUE)
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 1000
#define NUM_TIMES_TEST 10000 // number of times run WCET for each function
#define PRINT 0
#define LOG 1
#define FIND_MODE PRINT
#define TIMER_RELATIVE 0
uint32_t WCET_task[NUM_THREADS] = {0};
void print_WCETs();
void *Service_WCET(void *);
static inline void getElapsedTime(uint8_t task, struct timespec releaseTime, struct timespec completionTime);
#endif

// variables
sem_t task_sems[NUM_THREADS];
uint8_t abort_service[NUM_THREADS] = {0};
int task_priorities[NUM_THREADS] = {0,SERVO_PRIO,EXTERNAL_PRIO,ENCRYPT_DECRYPT_PRIO,KEYGEN_PRIO,UART_PRIO,TERMINAL_PRIO};
sentenceLinkedList_t *addHead, *encryptHead, *sendHead; 
uint8_t servoPosition[ENCRYPTION_KEY_LENGTH*8];
uint8_t communicatedServoBasis[ENCRYPTION_KEY_LENGTH*8];
uint8_t generateNewKey = FALSE;
uint16_t numRunPeriferal = 0;
uint8_t keygenIndex = 0;
#if (RPI_TYPE == TYPE_RECEIVOR)
uint8_t sensedData[ENCRYPTION_KEY_LENGTH*8];
#endif


// function prototypes
void *Service_1_Servos(void *);
void *Service_2_Periferal(void *);
void *Service_3_Encrypt(void *);
void *Service_4_Keygen(void *);
void *Service_5_UART(void *);
void *Service_6_Terminal(void *);

void main(void)
{
    // Thread parameters
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
    if(encryption_init()==ENCRYPTION_ERROR){
        perror("Encryption init error\n\r");
        return;
    }
    sentenceLL_init(&addHead, &encryptHead, &sendHead);
    servo_init();
    if(initialize_uart()!=0){
        perror("UART init error\n\r");
        return;
    }
    echo_UART();
    terminal_init(&addHead);
    keygen_init();
    #if (RPI_TYPE == TYPE_SENDER)
    init_laser_send();
    #else
    init_laser_receive();
    init_ads1115();
    calibrate_ads1115();
    #endif


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

    rc=pthread_create(&threads[1],&rt_sched_attr[1],Service_1_Servos, NULL);
    if(rc < 0)  { perror("\nError creating service 1");}
    else        {printf("S1, ");}

    rc=pthread_create(&threads[2],&rt_sched_attr[2],Service_2_Periferal, NULL);
    if(rc < 0)  { perror("\nError creating service 2");}
    else        {printf("S2, ");}

    rc=pthread_create(&threads[3],&rt_sched_attr[3],Service_3_Encrypt, NULL);
    if(rc < 0)  { perror("\nError creating service 3");}
    else        {printf("S3, ");}

    rc=pthread_create(&threads[4],&rt_sched_attr[4],Service_4_Keygen, NULL);
    if(rc < 0)  { perror("\nError creating service 4");}
    else        {printf("S4, ");}
    
    rc=pthread_create(&threads[5],&rt_sched_attr[5],Service_5_UART, NULL);
    if(rc < 0)  { perror("\nError creating service 5");}
    else        {printf("S5, ");}

    rc=pthread_create(&threads[6],&rt_sched_attr[6],Service_6_Terminal, NULL);
    if(rc < 0)  { perror("\nError creating service 6");}
    else        {printf("S6, ");}

    // schedueler
    struct timespec startIterationTime;
    uint32_t numNanosecondsSleep;
    printf("\nStarting scheduler\n");
    #if (FINDING_WCET == TRUE)
    printf("Finding WCETs, scheduler %u times\n", NUM_TIMES_TEST);
    for(int i = 0; i<NUM_TIMES_TEST; i++){
        clock_gettime(CLOCK_MONOTONIC,&startIterationTime);
        sem_post(&task_sems[1]);
        sem_post(&task_sems[3]);
        sem_post(&task_sems[4]);
        sem_post(&task_sems[5]);

        startIterationTime.tv_nsec += HALF_SERVO_MOVE_TIME;
        if(startIterationTime.tv_nsec>NS_PER_SEC){
            startIterationTime.tv_nsec-=NS_PER_SEC;
            startIterationTime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &startIterationTime, NULL);

        sem_post(&task_sems[3]);
        sem_post(&task_sems[5]);

        startIterationTime.tv_nsec += HALF_SERVO_MOVE_TIME;
        if(startIterationTime.tv_nsec>NS_PER_SEC){
            startIterationTime.tv_nsec-=NS_PER_SEC;
            startIterationTime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &startIterationTime, NULL);

        sem_post(&task_sems[2]);
        sem_wait(&task_sems[0]);
    }
    print_WCETs();
    #else
    for(;;){
        clock_gettime(CLOCK_MONOTONIC,&startIterationTime);
        sem_post(&task_sems[1]);
        sem_post(&task_sems[3]);
        sem_post(&task_sems[4]);
        sem_post(&task_sems[5]);

        startIterationTime.tv_nsec += HALF_SERVO_MOVE_TIME;
        if(startIterationTime.tv_nsec>NS_PER_SEC){
            startIterationTime.tv_nsec-=NS_PER_SEC;
            startIterationTime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &startIterationTime, NULL);

        sem_post(&task_sems[3]);
        sem_post(&task_sems[5]);

        startIterationTime.tv_nsec += HALF_SERVO_MOVE_TIME;
        if(startIterationTime.tv_nsec>NS_PER_SEC){
            startIterationTime.tv_nsec-=NS_PER_SEC;
            startIterationTime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &startIterationTime, NULL);

        sem_post(&task_sems[2]);
        sem_wait(&task_sems[0]);
    }
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
void *Service_1_Servos(void *) 
{
    uint16_t servoMoveCount = 0;
    printf("Servo service started\t");
    #if (FINDING_WCET == TRUE)
        struct timespec releaseTime, completionTime;
        #endif

    while(!abort_service[1])
    {
        sem_wait(&task_sems[1]);
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        if(servoMoveCount>ENCRYPTION_KEY_LENGTH*8){
            if(generateNewKey!=TRUE){
                continue;
            }
            servoMoveCount = 0;
        }
        servoPosition[servoMoveCount] = servo_set_angle_random();
        servoMoveCount++;
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        getElapsedTime(1, releaseTime, completionTime);
        #endif
    }

    pthread_exit((void *)0);
}

// external periferal service modify change needs something
void *Service_2_Periferal(void *) 
{
    printf("external periferal service started\t");
    uint16_t readData;
    numRunPeriferal = 0;
    #if (FINDING_WCET == TRUE)
        struct timespec releaseTime, completionTime;
        #endif

    while(!abort_service[2])
    {
        sem_wait(&task_sems[2]);
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        if(numRunPeriferal>=ENCRYPTION_KEY_LENGTH*8){
            if(generateNewKey!=TRUE){
                sem_post(&task_sems[0]);
                continue;
            }
            numRunPeriferal = 0;
        }
        #if (RPI_TYPE == TYPE_SENDER)
        laser_on();
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELATIVE, &(struct timespec){.tv_sec = 0, .tv_nsec = LASER_TIME_ON*NS_PER_USEC}, NULL);
        laser_off();
        #else
        while(get_laser_state_gpio==0);
        readData = read_ads1115();
        if(readData > ADC_PHOTOSENSOR_READ_HIGH){
            sensedData[numRunPeriferal] = 1;
        }
        else{
            sensedData[numRunPeriferal] = 0;
        }
        while(get_laser_state_gpio==1);
        #endif
        numRunPeriferal++;
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        getElapsedTime(2, releaseTime, completionTime);
        #endif
        sem_post(&task_sems[0]);
    }

    pthread_exit((void *)0);
}

// Encryption service
void *Service_3_Encrypt(void *) 
{
    printf("Encryption service started\t");
    #if (FINDING_WCET == TRUE)
        struct timespec releaseTime, completionTime;
        #endif

    while(!abort_service[3])
    {
        sem_wait(&task_sems[3]);
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        // if we have any data to encrypt
        while(sentenceLL_getNumSentencesToEncrypt()!=0){
            #if (RPI_TYPE == TYPE_SENDER) // encrypting data
            // create a new nonce
            encryption_updateNonce();
            // encrypt the data where we need to
            if(encryption_encryptData(encryptHead->sentence, encryptHead->numCharacters)==ENCRYPTION_ERROR){
                perror("Encryption Error\n");
                break;
            }
            // say that we have encrypted the data
            if(sentenceLL_encryptedSentence(&encryptHead,encryption_getNonceAddress())==SENTENCELL_ERROR) {
                perror("Sentence LL Encryption Error\n");
                break;
            }
            #else // decrypting data
            if(encryption_decryptData(sendHead->sentence, sendHead->numCharacters, sendHead->sentenceNonce)==ENCRYPTION_ERROR){
                printf("Decryption Error\n");
                break;
            }
            if(sentenceLL_removeSentence(&sendHead)==SENTENCELL_ERROR) {
                perror("Sentence LL Decryption Error\n");
                break;
            }
            #endif
        }
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        getElapsedTime(3, releaseTime, completionTime);
        #endif
    }

    pthread_exit((void *)0);
}

// keygen service
void *Service_4_Keygen(void *) 
{
    uint8_t lastComputedKeygenIndex;
    printf("Keygen service started\t");
    #if (FINDING_WCET == TRUE)
        struct timespec releaseTime, completionTime;
        #endif

    while(!abort_service[4])
    {
        sem_wait(&task_sems[4]);
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        // function changes based on whether we are the sender or receiver pi
        if(keygenIndex>lastComputedKeygenIndex){
            #if (RPI_TYPE == TYPE_SENDER)
            keygen_senderByByte(servoPosition,communicatedServoBasis,lastComputedKeygenIndex,keygenIndex);
            #else
            keygen_receiverByByte(sensedData,servoPosition,communicatedServoBasis,lastComputedKeygenIndex,keygenIndex);
            #endif
            lastComputedKeygenIndex=keygenIndex;
        }

        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        getElapsedTime(4, releaseTime, completionTime);
        #endif
    }

    pthread_exit((void *)0);
}

// UART service TODO must fix modify change to do
void *Service_5_UART(void *) 
{
    printf("UART service started\t");
    #if (FINDING_WCET == TRUE)
        struct timespec releaseTime, completionTime;
        #endif

    while(!abort_service[5])
    {
        sem_wait(&task_sems[5]);
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (RPI_TYPE == TYPE_SENDER)
        if(numRunPeriferal - 8 * keygenIndex >= 8){ // if we need to send keygen
            uint8_t numServoDataToSend = (numRunPeriferal - 8 * keygenIndex)/8;
            char servoPositionBasisesToSend[numServoDataToSend*8];
            for(uint16_t i = 0; i < numServoDataToSend*8; i++){
                servoPositionBasisesToSend[i] = servoPosition[i + 8 * keygenIndex]%2;
            }
            // send the current servo data
            uart_send(servoPositionBasisesToSend, numServoDataToSend, UART_SENDER_SERVO_DATA);
            // receive the current servo data
            char *receiverServoBasisData;
            receiverServoBasisData = uart_receive();
            while(receiverServoBasisData==NULL){
                receiverServoBasisData = uart_receive();
            }
            for(uint16_t i = 0; i < numServoDataToSend*8; i++){
                communicatedServoBasis[i + 8 * keygenIndex] = receiverServoBasisData[i+2];
            }
            keygenIndex += numServoDataToSend;
        }
        while(sentenceLL_getNumSentencesToSend()>0){
            char *sentenceToSend;
            uint8_t length;
            sentenceLL_getSentence(&sendHead, sentenceToSend, &length);
            uart_send(sentenceToSend, length, UART_SENDER_SENTENCE_ENCRYPTED);
            sentenceLL_removeSentence(&sendHead);
        }
        #else
        if(numRunPeriferal - 8 * keygenIndex >= 8){ // if we need to send keygen
            uint8_t numServoDataToSend = (numRunPeriferal - 8 * keygenIndex)/8;
            char servoPositionBasisesToSend[numServoDataToSend*8];
            for(uint16_t i = 0; i < numServoDataToSend*8; i++){
                servoPositionBasisesToSend[i] = servoPosition[i + 8 * keygenIndex]%2;
            }
            char *receiverServoBasisData;
            receiverServoBasisData = uart_receive();
            // receive the current servo data
            while(receiverServoBasisData==NULL){
                receiverServoBasisData = uart_receive();
            }
            // send the current servo data
            uart_send(servoPositionBasisesToSend, numServoDataToSend, UART_SENDER_SERVO_DATA);
            for(uint16_t i = 0; i < numServoDataToSend*8; i++){
                communicatedServoBasis[i + 8 * keygenIndex] = receiverServoBasisData[i+2];
            }
            keygenIndex += numServoDataToSend;
        }
        for(uint8_t i = 0; i < UART_NUM_CHECK_EMPTY_BE_SURE; i++){
            char *sentenceToReceive;
            sentenceToReceive = uart_receive();
            if(sentenceToReceive!=NULL){
                i = 0;
                *addHead->sentenceNonce = &(sentenceToReceive[2]);
                sentenceLL_addSentence(&addHead, &(sentenceToReceive[2+ENCRYPTION_NONCE_LENGTH]), sentenceToReceive[1]);
            }
        }
        #endif
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        getElapsedTime(5, releaseTime, completionTime);
        #endif
    }

    pthread_exit((void *)0);
}

// terminal service
void *Service_6_Terminal(void *){
    printf("Terminal service started\t");
    #if (FINDING_WCET == TRUE)
        struct timespec releaseTime, completionTime;
        #endif

    while(!abort_service[6])
    {
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (RPI_TYPE == TYPE_SENDER)
        terminal_read_char();
        #else
        if(sentenceLL_getNumSentencesToSend()>0){
            terminal_printDecryptedSentence(&sendHead);
        }
        #endif
        #if (FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        getElapsedTime(6, releaseTime, completionTime);
        #endif
    }

    pthread_exit((void *)0);
}


#if (FINDING_WCET == TRUE)
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

// ensure that the RPis are echoing characters between each other over UART
void echo_UART()
{
    printf("Running UART echo function...\n");
    char *sentenceToReceive;

    #if (RPI_TYPE == TYPE_SENDER)

    uart_send("a", 1, UART_SENDER_SENTENCE_UNENCRYPTED);
    *sentenceToReceive;
    while (sentenceToReceive == NULL)
    {
        sentenceToReceive = uart_receive();
    }

    #else

    *sentenceToReceive;
    while (sentenceToReceive == NULL)
    {
        sentenceToReceive = uart_receive();
    }
    uart_send("a", 1, UART_SENDER_SENTENCE_UNENCRYPTED);
}
#endif // (FINDING_WCET == TRUE)
