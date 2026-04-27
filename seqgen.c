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
#define SERVO_MOVE_TIME         (900*NS_PER_MS)
#define SERVO_MOVE_DIVISOR      9
#define DIVIDED_SERVO_MOVE_TIME (SERVO_MOVE_TIME/SERVO_MOVE_DIVISOR)
#define TIMER_RELATIVE 0
#define LOGGING TRUE
#define PERIF_DEBUG_PRINTS FALSE
#define SERVO_SYNC_DEBUG_PRINTS FALSE
#define UART_SERVO_DEBUG_PRINTS FALSE
#define SERVICE_DEBUGPRINTS FALSE

// added code for finding WCET
#define FINDING_WCET FALSE
#if (FINDING_WCET == TRUE)
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 1000
#define NUM_TIMES_TEST 250 // number of times run WCET for each function
#define PRINT 0
#define LOG 1
#define FIND_MODE PRINT
uint32_t WCET_task[NUM_THREADS] = {0};
void print_WCETs();
void *Service_WCET(void *);
static inline void getElapsedTime(uint8_t task, struct timespec releaseTime, struct timespec completionTime);
#endif

// variables
sentenceLinkedList_t *addHead, *encryptHead, *sendHead; 
sem_t task_sems[NUM_THREADS];
uint8_t abort_service[NUM_THREADS] = {0};
int task_priorities[NUM_THREADS] = {0,SERVO_PRIO,EXTERNAL_PRIO,ENCRYPT_DECRYPT_PRIO,KEYGEN_PRIO,UART_PRIO,TERMINAL_PRIO};
uint8_t servoPosition[ENCRYPTION_KEY_LENGTH*8];
uint8_t communicatedServoBasis[ENCRYPTION_KEY_LENGTH*8];
uint8_t generateNewKey = FALSE;
uint16_t numRunPeriferal = 0;
uint8_t keygenIndex = 0;
#if (RPI_TYPE == TYPE_RECEIVOR)
uint8_t sensedData[ENCRYPTION_KEY_LENGTH*8];
#endif


// function prototypes
uint8_t init_all();
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

    printf("Running BB84 Project\n");
    clock_gettime(CLOCK_MONOTONIC,&start_time); // start_time->tv_sec, start_time->tv_nsec
    syslog(LOG_INFO, "Initalization start:\tsec=%lu\tnsec=%lu\n", start_time.tv_sec, start_time.tv_nsec);

    if(init_all()!=0){
        perror("Initalization failed\n\r");
        return;
    }

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
            if(sem_init (&task_sems[i], 0, 0)) {printf ("Failed to initialize semaphore number %d\n",i); exit (-1); }
        }
    }
    
    // thread creation
    printf("pthread created: ");

    rc=pthread_create(&threads[1],&rt_sched_attr[1],Service_1_Servos, NULL);
    if(rc < 0)  { perror("\nError creating service 1");}
    #if (SERVICE_DEBUGPRINTS == TRUE)
    else        {printf("S1, ");}
    #endif

    rc=pthread_create(&threads[2],&rt_sched_attr[2],Service_2_Periferal, NULL);
    if(rc < 0)  { perror("\nError creating service 2");}
    #if (SERVICE_DEBUGPRINTS == TRUE)
    else        {printf("S2, ");}
    #endif

    rc=pthread_create(&threads[3],&rt_sched_attr[3],Service_3_Encrypt, NULL);
    if(rc < 0)  { perror("\nError creating service 3");}
    #if (SERVICE_DEBUGPRINTS == TRUE)
    else        {printf("S3, ");}
    #endif

    rc=pthread_create(&threads[4],&rt_sched_attr[4],Service_4_Keygen, NULL);
    if(rc < 0)  { perror("\nError creating service 4");}
    #if (SERVICE_DEBUGPRINTS == TRUE)
    else        {printf("S4, ");}
    #endif
    
    rc=pthread_create(&threads[5],&rt_sched_attr[5],Service_5_UART, NULL);
    if(rc < 0)  { perror("\nError creating service 5");}
    #if (SERVICE_DEBUGPRINTS == TRUE)
    else        {printf("S5, ");}
    #endif

    rc=pthread_create(&threads[6],&rt_sched_attr[6],Service_6_Terminal, NULL);
    if(rc < 0)  { perror("\nError creating service 6");}
    #if (SERVICE_DEBUGPRINTS == TRUE)
    else        {printf("S6, ");}
    #endif

    // schedueler
    uint32_t numNanosecondsSleep;
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("\nStarting scheduler\n");
    #endif
    #if (FINDING_WCET == TRUE)
    printf("Finding WCETs, scheduler %u times\n", NUM_TIMES_TEST);
    #endif
    #if (RPI_TYPE == TYPE_SENDER)
    echo_uartSender();
    #else
    echo_uartReceiver();
    #endif
    #if (FINDING_WCET == TRUE)
    for(int i = 0; i<NUM_TIMES_TEST; i++){
    #else
    while(1){
    #endif
        #if(LOGGING == TRUE || FINDING_WCET == TRUE)
        clock_gettime(CLOCK_MONOTONIC,&start_time);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "Scheduler start:\tsec=%lu\tnsec=%lu\n", start_time.tv_sec, start_time.tv_nsec);
        #endif
        sem_post(&task_sems[1]);
        sem_post(&task_sems[3]);
        sem_post(&task_sems[4]);
        sem_post(&task_sems[5]);

        for(uint8_t i = 0; i < SERVO_MOVE_DIVISOR; i++){
            start_time.tv_nsec += DIVIDED_SERVO_MOVE_TIME;
            if(start_time.tv_nsec>NS_PER_SEC){
                start_time.tv_nsec-=NS_PER_SEC;
                start_time.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &start_time, NULL);

            sem_post(&task_sems[3]);
            sem_post(&task_sems[5]);
        }

        sem_post(&task_sems[2]);
        #if (LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC,&start_time);
        syslog(LOG_INFO, "Scheduler end:\tsec=%lu\tnsec=%lu\n", start_time.tv_sec, start_time.tv_nsec);
        #endif
        sem_wait(&task_sems[0]);
    }
    #if (FINDING_WCET == TRUE)
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
void *Service_1_Servos(void *) 
{
    uint16_t servoMoveCount = 0;
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("Servo service started\t");
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif

    while(!abort_service[1])
    {
        sem_wait(&task_sems[1]);
        #if (SERVO_SYNC_DEBUG_PRINTS == TRUE)
        printf("Servo counter \t\t%u\n\r", servoMoveCount); // remove change modify
        #endif
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S1 start:\tsec=%lu\tnsec=%lu\n", releaseTime.tv_sec, releaseTime.tv_nsec);
        #endif
        if(servoMoveCount>ENCRYPTION_KEY_LENGTH*8){ // waiting for new key criteria change modify fix
            if(generateNewKey!=TRUE){
                continue;
            }
            servoMoveCount = 0;
        }
        servoPosition[servoMoveCount] = servo_set_angle_random();
        servoMoveCount++;
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        #endif
        #if (FINDING_WCET == TRUE)
        getElapsedTime(1, releaseTime, completionTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S1 end:\tsec=%lu\tnsec=%lu\n", completionTime.tv_sec, completionTime.tv_nsec);
        #endif
    }

    pthread_exit((void *)0);
}

// external periferal service modify change needs something
void *Service_2_Periferal(void *) 
{
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("external periferal service started\t");
    #endif
    numRunPeriferal = 0;
    #if (RPI_TYPE != TYPE_SENDER)
    uint16_t readData;
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif

    while(!abort_service[2])
    {
        sem_wait(&task_sems[2]);
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S2 start:\tsec=%lu\tnsec=%lu\n", releaseTime.tv_sec, releaseTime.tv_nsec);
        #endif
        if(numRunPeriferal>=ENCRYPTION_KEY_LENGTH*8){ // waiting for new key criteria change modify fix
            if(generateNewKey!=TRUE){
                sem_post(&task_sems[0]);
                continue;
            }
            numRunPeriferal = 0;
        }
        #if (RPI_TYPE == TYPE_SENDER)
        laser_on();
        #if (PERIF_DEBUG_PRINTS == TRUE)
        printf("Turned laser on, waiting for capture end\n\r");
        #endif
        while(get_laserCaptureState()==0);
        laser_off();
        #if (PERIF_DEBUG_PRINTS == TRUE)
        printf("Capture start found, laser off, waiting for capture reset\n\r");
        #endif
        while(get_laserCaptureState()==1); // this ensures that the ADC releases faster, and this is good so that it will always capture the laser
        #else
        #if (PERIF_DEBUG_PRINTS == TRUE)
        printf("waiting for laser on\n\r");
        #endif
        while(get_laser_state_gpio()==0);
        readData = read_ads1115();
        if(readData > ADC_PHOTOSENSOR_READ_HIGH){
            sensedData[numRunPeriferal] = 1;
        }
        else{
            sensedData[numRunPeriferal] = 0;
        }
        laser_capturedOn();
        #if (PERIF_DEBUG_PRINTS == TRUE)
        printf("laser is on, capture is done, waiting for laser to be off\n\r");
        #endif
        while(get_laser_state_gpio()==1);
        #if (PERIF_DEBUG_PRINTS == TRUE)
        printf("turing capture off\n\r");
        #endif
        laser_capturedOff();
        #endif
        numRunPeriferal++;
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        #endif
        #if (FINDING_WCET == TRUE)
        getElapsedTime(2, releaseTime, completionTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S2 end:\tsec=%lu\tnsec=%lu\n", completionTime.tv_sec, completionTime.tv_nsec);
        #endif
        sem_post(&task_sems[0]);
    }

    pthread_exit((void *)0);
}

// Encryption service
void *Service_3_Encrypt(void *) 
{
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("Encryption service started\t");
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif
    while(!abort_service[3])
    {
        sem_wait(&task_sems[3]);
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S3 start:\tsec=%lu\tnsec=%lu\n", releaseTime.tv_sec, releaseTime.tv_nsec);
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
            if(encryption_decryptData(encryptHead->sentence, encryptHead->numCharacters, encryptHead->sentenceNonce)==ENCRYPTION_ERROR){
                printf("Decryption Error\n");
                break;
            }
            if(sentenceLL_encryptedSentence(&encryptHead,encryptHead->sentenceNonce)==SENTENCELL_ERROR) {
                perror("Sentence LL Encryption Error\n");
                break;
            }
            #endif
        }
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        #endif
        #if (FINDING_WCET == TRUE)
        getElapsedTime(3, releaseTime, completionTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S3 end:\tsec=%lu\tnsec=%lu\n", completionTime.tv_sec, completionTime.tv_nsec);
        #endif
    }

    pthread_exit((void *)0);
}

// keygen service
void *Service_4_Keygen(void *) 
{
    uint8_t lastComputedKeygenIndex;
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("Keygen service started\t");
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif
    while(!abort_service[4])
    {
        sem_wait(&task_sems[4]);
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S4 start:\tsec=%lu\tnsec=%lu\n", releaseTime.tv_sec, releaseTime.tv_nsec);
        #endif
        
        // function changes based on whether we are the sender or receiver pi
        if(keygenIndex>lastComputedKeygenIndex){ // waiting for new key criteria change modify fix
            #if (RPI_TYPE == TYPE_SENDER)
            keygen_senderByByte(servoPosition,communicatedServoBasis,lastComputedKeygenIndex,keygenIndex);
            #else
            keygen_receiverByByte(sensedData,servoPosition,communicatedServoBasis,lastComputedKeygenIndex,keygenIndex);
            #endif
            lastComputedKeygenIndex=keygenIndex;
        }

        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        #endif
        #if (FINDING_WCET == TRUE)
        getElapsedTime(4, releaseTime, completionTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S4 end:\tsec=%lu\tnsec=%lu\n", completionTime.tv_sec, completionTime.tv_nsec);
        #endif
    }

    pthread_exit((void *)0);
}

// UART service TODO must fix modify change to do
void *Service_5_UART(void *) 
{
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("UART service started\t");
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif

    while(!abort_service[5])
    {
        sem_wait(&task_sems[5]);
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S5 start:\tsec=%lu\tnsec=%lu\n", releaseTime.tv_sec, releaseTime.tv_nsec);
        #endif



        #if (RPI_TYPE == TYPE_SENDER)

        // Servo data
        if(numRunPeriferal - (8 * keygenIndex) >= 8){ // if we need to send keygen, we have a byte to send
            uint8_t numServoDataToSend = (numRunPeriferal - 8 * keygenIndex)/8; // the number of bytes we have to send
            uint16_t servoBitLen = numServoDataToSend * 8; // it's just the number of bits we have to send instead of bytes
            char servoPositionBasisesToSend[ENCRYPTION_KEY_LENGTH*8]; // array for sending data

            #if (UART_SERVO_DEBUG_PRINTS == TRUE)
            printf("Num run periferal: %u\n\r",numRunPeriferal); //remove change modify fix

            // pull the servo basis data
            printf("Sender: Waiting for Servo Data\n\r");
            #endif
            while(uart_receive(servoPositionBasisesToSend, NULL) != UART_DATA_TYPE_SERVO); // we know exactily how much infromation SHOULD be sent, and we wait till we get the servo info
            #if (UART_SERVO_DEBUG_PRINTS == TRUE)
            printf("Sender: Recieved servo data\n\r");
            #endif

            // save the servo basis data
            for(uint16_t i = 0; i < servoBitLen; i++){
                communicatedServoBasis[i + 8 * keygenIndex] = servoPositionBasisesToSend[i];
            }

            // generate the servo basis data
            for(uint16_t i = 0; i < servoBitLen; i++){
                servoPositionBasisesToSend[i] = servoPosition[i + 8 * keygenIndex]%2;
            }

            // send the current servo basis data
            uart_send(servoPositionBasisesToSend, (uint8_t) servoBitLen, UART_DATA_TYPE_SERVO);
            #if (UART_SERVO_DEBUG_PRINTS == TRUE)
            printf("Sender: Sent servo data\n\r"); // remove this
            #endif

            keygenIndex += numServoDataToSend;
        }

        // Sentence Data
        while(sentenceLL_getNumSentencesToSend()>0){
            char bytesToSend[ENCRYPTION_NONCE_LENGTH+SENTENCELL_SENTENCE_SIZE];
            uint8_t length;
            // copy in the nonce to send
            memcpy(bytesToSend,sendHead->sentenceNonce,ENCRYPTION_NONCE_LENGTH);

            // ask for the current sentence, and copy it AFTER the nonce location
            sentenceLL_getSentence(&sendHead, &bytesToSend[ENCRYPTION_NONCE_LENGTH], &length);

            // add the nonce length to the message size
            length+=ENCRYPTION_NONCE_LENGTH;

            // send it
            uart_send(bytesToSend, length, UART_DATA_TYPE_SENTENCE);
            sentenceLL_removeSentence(&sendHead);
        }


        // we are the receiver
        #else
        // Servo Data
        if(numRunPeriferal - (8 * keygenIndex) >= 8){ // if we need to send keygen, we have a byte to send
            uint8_t numServoDataToSend = (numRunPeriferal - 8 * keygenIndex)/8; // the number of bytes we have to send
            uint16_t servoBitLen = numServoDataToSend * 8; // it's just the number of bits we have to send instead of bytes
            char servoPositionBasisesToSend[ENCRYPTION_KEY_LENGTH*8]; // array for sending data

            #if (UART_SERVO_DEBUG_PRINTS == TRUE)
            printf("Num run periferal: %u\n\r",numRunPeriferal); //remove change modify fix
            #endif

            // generate the servo basis data
            for(uint16_t i = 0; i < servoBitLen; i++){
                servoPositionBasisesToSend[i] = servoPosition[i + 8 * keygenIndex]%2;
            }

            // send the current servo basis data
            uart_send(servoPositionBasisesToSend, (uint8_t) servoBitLen, UART_DATA_TYPE_SERVO);
            #if (UART_SERVO_DEBUG_PRINTS == TRUE)
            printf("Receiver: Sent servo data\n\r"); // remove this
            #endif

            // pull the servo basis data
            while(uart_receive(servoPositionBasisesToSend, NULL) != UART_DATA_TYPE_SERVO); // we know exactily how much infromation SHOULD be sent, and we wait till we get the servo info
            #if (UART_SERVO_DEBUG_PRINTS == TRUE)
            printf("Receiver: Reveived servo data\n\r"); // remove this
            #endif

            // save the servo basis data
            for(uint16_t i = 0; i < servoBitLen; i++){
                communicatedServoBasis[i + 8 * keygenIndex] = servoPositionBasisesToSend[i];
            }
            keygenIndex += numServoDataToSend;

        }

        // Sentence Data
        for(uint8_t i = 0; i < UART_NUM_CHECK_EMPTY_BE_SURE; i++){
            char sentenceReveived[ENCRYPTION_NONCE_LENGTH+SENTENCELL_SENTENCE_SIZE];
            uint8_t sizeOfSentence;
            uint8_t dataType = uart_receive(sentenceReveived,&sizeOfSentence);
            if(dataType==0){
                continue; // skip the reset of this
            }
            else if (dataType == UART_DATA_TYPE_SENTENCE){
                sentenceLL_setNonce(&addHead,sentenceReveived);
                // must be done after adding the nonce else it will add the nonce to the next sentence instead of the current sentence
                sentenceLL_addSentence(&addHead,&sentenceReveived[ENCRYPTION_NONCE_LENGTH],sizeOfSentence-ENCRYPTION_NONCE_LENGTH);
            }
        }
        #endif



        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        #endif
        #if (FINDING_WCET == TRUE)
        getElapsedTime(5, releaseTime, completionTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S5 end:\tsec=%lu\tnsec=%lu\n", completionTime.tv_sec, completionTime.tv_nsec);
        #endif
    }

    pthread_exit((void *)0);
}

// terminal service
void *Service_6_Terminal(void *){
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("Terminal service started\n\r");
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif
    #if (RPI_TYPE == TYPE_SENDER)
    printf(" --- Welcome to the send Terminal, type in a sentence to send ---\n >> ");
    #endif
    while(!abort_service[6])
    {
        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &releaseTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S6 start:\tsec=%lu\tnsec=%lu\n", releaseTime.tv_sec, releaseTime.tv_nsec);
        #endif

        // check RPI type
        #if (RPI_TYPE == TYPE_SENDER)
        int8_t terminalReadReturnValue = terminal_read_char(); 
        if(terminalReadReturnValue==TERIMINAL_ERROR){
            perror("Terminal get char error\n\r");
        }
        else if(terminalReadReturnValue == TERMINAL_ADDED_SENTENCE){
            printf("\n >> ");
        }
        #else
        if(sentenceLL_getNumSentencesToSend()>0){
            terminal_print_and_delete_DecryptedSentence(&sendHead);
        }
        #endif


        #if (FINDING_WCET == TRUE || LOGGING == TRUE)
        clock_gettime(CLOCK_MONOTONIC, &completionTime);
        #endif
        #if (FINDING_WCET == TRUE)
        getElapsedTime(6, releaseTime, completionTime);
        #endif
        #if (LOGGING == TRUE)
        syslog(LOG_INFO, "S6 end:\tsec=%lu\tnsec=%lu\n", completionTime.tv_sec, completionTime.tv_nsec);
        #endif
    }

    pthread_exit((void *)0);
}

// initalizes all external files
uint8_t init_all(){
    // initalizing other files
    if (wiringPiSetup() == -1){
        printf("Could not initialize WiringPi library\n");
        syslog(LOG_PERROR, "Could not initialize WiringPi library\n");
        return 1;
    }

    if(encryption_init()==ENCRYPTION_ERROR){
        perror("Encryption init error\n\r");
        return 1;
    }
    sentenceLL_init(&addHead, &encryptHead, &sendHead);
    servo_init();
    if(initialize_uart()!=0){
        perror("UART init error\n\r");
        return 1;
    }
    terminal_init(&addHead);
    keygen_init();
    #if (RPI_TYPE == TYPE_SENDER)
    init_laser_send();
    #else
    init_laser_receive();
    init_ads1115();
    calibrate_ads1115();
    #endif
    return 0;
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
#endif // (FINDING_WCET == TRUE)
