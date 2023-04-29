#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define SCHEDULE_INTERVAL 1 /* scheduling interval in second */

#define NUM_THREADS 5

#define SCHED_VECTOR_SIZE 5

#define SLOWDOWNFACTOR 6000000

/* prototypes --------------------------------------------------------- */
void child_thread_routine(void *arg); // definition of a child thread
void clock_interrupt_handler(void);   // definition of the interrupt handler for SIGALM interrupt

/* pthread mutex semaphore -------------------------------------------- */
pthread_mutex_t condition_mutex; // pthread mutex_condition

/* pthread condition varaiable ---------------------------------------- */
pthread_cond_t t_condition[NUM_THREADS]; // pthread condition

/* The thread schedule vector ----------------------------------------- */
int schedule_vector[SCHED_VECTOR_SIZE]; // the required thread schedule vector

int current_thread = 0; // initialize the current thread index

int loop_counter = 1; // initialize the loop counter for the interrupt hanlder

/* pthread barrier ---------------------------------------------------- */
pthread_barrier_t start_barrier;

/* the main (the parent thread) --------------------------------------- */
int main(void)
{
    int i = 0; // the loop counter for the parent thread

    /* thread attributes for managing child threads ------------------- */
    pthread_t threads[NUM_THREADS]; // thread IDs (assigned by OS)
    pthread_attr_t attr;            // thread attributes
    int thread_ids[NUM_THREADS];    // thread arguments

    /* initialize the schedule vector --------------------------------- */
    schedule_vector[0] = 0; // the first thread
    schedule_vector[1] = 1; // the second thread
    schedule_vector[2] = 2; // the third thread
    schedule_vector[3] = 3; // the fourth thread
    schedule_vector[4] = 4; // the fifth thread

    /* specify the clock interrupt to be sent to this process --- */
    signal(SIGALRM, clock_interrupt_handler);

    /* initialize pthread mutex --------------------------------------- */
    pthread_mutex_init(&condition_mutex, NULL);

    /* initialize pthread condition variables -------------------------- */
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_cond_init(&t_condition[i], NULL);
    }

    /* initialize pthread barrier ------------------------------------- */
    pthread_barrier_init(&start_barrier, NULL, NUM_THREADS + 1);

    /* create child threads ------------------------------------------- */

    for (i = 0; i < NUM_THREADS; i++)
    {
        thread_ids[i] = i;
        pthread_create(           // create a child thread
            &threads[i],          // thread ID (system assigned)
            NULL,                 // default thread attributes
            child_thread_routine, // thread routine name
            &thread_ids[i]);      // arguments to be passed
    }

    /* wait for all child threads to be initialized -------------------- */
    pthread_barrier_wait(&start_barrier);

    /* set the interrupt interval to 1 second --- */
    alarm(SCHEDULE_INTERVAL);

    /* infinite loop for the parent thread --------------------------- */
    while (1)
    {
        sleep(1);
    }

    /* The main (parent) thread terminates itself ------------------ */
    return (0);
}
/* THE END OF MAIN ===================================================== */

/* child thread implementation ----------------------------------------- */
void child_thread_routine(void *arg)
{
    int myid = *(int *)arg; // child thread number (not ID)
    int i = 0;              // loop counter
    /* declarare the start of this child thread ---------- */
    printf("child thread %d started ...\n", myid);

    /* wait for all child threads to be initialized ------- */
    pthread_barrier_wait(&start_barrier);

    /* infinite loop for the child thread -------------------------- */
    while (1)
    {
        /* child thread performs "wait" on the condition variable */
        pthread_mutex_lock(&condition_mutex);
        while (myid != current_thread)
        {
            pthread_cond_wait(&t_condition[myid], &condition_mutex);
        }
        pthread_mutex_unlock(&condition_mutex);

        /* child thread starts running again -------------------- */
        if ((i % SLOWDOWNFACTOR) == 0)
        {
            printf("Thread: %d is running ...\n", myid);
        }
        i++;
    }
}

/* The interrupt handler for SIGALM interrupt ---------------------- */
void clock_interrupt_handler(void)
{
    /* to be displayed at each time I woke up ---------------- */
    printf("    I woke up on the timer interrupt (%d) .... \n", loop_counter);

    pthread_mutex_lock(&condition_mutex);

    // Use the loop counter to access the schedule vector for the next thread to run
    current_thread = schedule_vector[loop_counter % SCHED_VECTOR_SIZE];
    pthread_cond_signal(&t_condition[current_thread]);

    pthread_mutex_unlock(&condition_mutex);

    /* increase the loop counter for the interrupt handler --- */
    loop_counter = loop_counter + 1;

    /* scheduler wakes up again one second later -------------------- */
    alarm(SCHEDULE_INTERVAL);
}
