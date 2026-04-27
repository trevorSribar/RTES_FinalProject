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
#include "adc.h"
#include "laser.h"
#include "uart.h"

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

#define NUM_THREADS (2+1)
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
int task_priorities[NUM_THREADS] = {0,SERVO_PRIO,EXTERNAL_PRIO};
uint8_t servoPosition[ENCRYPTION_KEY_LENGTH*8];
uint8_t communicatedServoBasis[ENCRYPTION_KEY_LENGTH*8];
uint8_t generateNewKey = FALSE;
uint16_t numRunPeriferal = 0;
uint8_t keygenIndex = 0;
#if (RPI_TYPE == TYPE_RECEIVOR)
uint8_t sensedData[ENCRYPTION_KEY_LENGTH*8];
#endif

uint8_t currentSevoAngle; // this is added to keep track of what pos the servo is in
uint8_t ignoreFlag = 0;
uint8_t newAngleFlag = 0;


// function prototypes
uint8_t init_all();
int8_t iterate_servo();
void *Service_1_Servos(void *);
void *Service_2_Periferal(void *);

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

    #if (RPI_TYPE == TYPE_SENDER)
    // lining up the laser
    while(iterate_servo()!=1){
        laser_on();
    }
    laser_off();
    #endif

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
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("pthread created: ");
    #endif

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

        for(uint8_t i = 0; i < SERVO_MOVE_DIVISOR; i++){
            start_time.tv_nsec += DIVIDED_SERVO_MOVE_TIME;
            if(start_time.tv_nsec>NS_PER_SEC){
                start_time.tv_nsec-=NS_PER_SEC;
                start_time.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &start_time, NULL);
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
    #if (SERVICE_DEBUGPRINTS == TRUE)
    printf("Servo service started\t");
    #endif
    #if (FINDING_WCET == TRUE || LOGGING == TRUE)
    struct timespec releaseTime, completionTime;
    #endif
    currentSevoAngle = 0;
    uint8_t firstCall = 1;
    uint8_t ignoreIterate = 0;

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

        #if (RPI_TYPE == TYPE_SENDER)
        if(iterate_servo()==1 || firstCall==1){
            servo_set_angle(currentSevoAngle);
            printf("New Send Servo Angle: %d\n",currentSevoAngle*45);
            currentSevoAngle++;
            currentSevoAngle=currentSevoAngle%4;
            firstCall = 0;
        }
        #else
        if(firstCall==1){
            servo_set_angle(currentSevoAngle);
            printf("New Send Servo Angle: 0\n");
            printf("New Receive Servo Angle: 0\n");
            currentSevoAngle++;
            currentSevoAngle=currentSevoAngle%4;
            firstCall = 0;
        }
        if(iterate_servo()==1){
            ignoreIterate++;
            if(ignoreIterate%2==1){
                printf("\nIgnoring till next input\n");
                ignoreFlag = 1;
            }
            else{
                printf("New Send Servo Angle: %d",ignoreIterate/2*45);
                if(ignoreIterate==8){
                    servo_set_angle(currentSevoAngle);
                    printf("New Receive Servo Angle: %d\n", currentSevoAngle*45);
                    ignoreIterate = 0;
                    currentSevoAngle++;
                    currentSevoAngle=currentSevoAngle%4;
                }
                ignoreFlag = 0;
                newAngleFlag++;
            }
        }
        #endif


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
    #if (RPI_TYPE != TYPE_SENDER)
    uint16_t readData;
    uint16_t maxReadData = 0;
    uint16_t minReadData = 0xFFFF;
    uint8_t newMaxMin = 0;
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
        // this is where we change for this setup function
        if(ignoreFlag != 1){
            if(newAngleFlag==1){
                newAngleFlag = 0;
                maxReadData = 0;
                minReadData = 0xFFFF;
            }
            if(readData>maxReadData){
                maxReadData = readData;
                newMaxMin = 1;
            }
            if(readData<minReadData){
                minReadData = readData;
                newMaxMin = 1;
            }
            if(newMaxMin == 1){
                printf("\rRead data \tmax: %d\tmin: %d",maxReadData,minReadData);
                newMaxMin = 0;
            }
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

// initalizes all external files
uint8_t init_all(){
    // initalizing other files
    if (wiringPiSetup() == -1){
        printf("Could not initialize WiringPi library\n");
        syslog(LOG_PERROR, "Could not initialize WiringPi library\n");
        return 1;
    }

    servo_init();
    if(initialize_uart()!=0){
        perror("UART init error\n\r");
        return 1;
    }
    terminal_init(&addHead);
    #if (RPI_TYPE == TYPE_SENDER)
    init_laser_send();
    #else
    init_laser_receive();
    init_ads1115();
    calibrate_ads1115();
    #endif
    return 0;
}

// lets the servo move to the next position
int8_t iterate_servo(){

    // this function is just a modifed terminal get char, that returns if got a valid input
    fd_set readfds;
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    if (ready < 0)
    {
        return TERIMINAL_ERROR;
    }

    if (ready == 0 || !FD_ISSET(STDIN_FILENO, &readfds))
    {
        return 0; // nothing in stdin right now
    }

    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
    {
        return TERIMINAL_ERROR;
    }
    return 1;
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
