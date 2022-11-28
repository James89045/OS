#define _GNU_SOURCE 
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <time.h>
typedef struct {
    int thread_id;
    int sched_policy;
    int sched_priority;
    float time_wait;
} thread_info_t;

pthread_barrier_t barrier;

void *thread_func(void *arg)
{

    /* 1. Wait until all threads are ready */
    thread_info_t *my_thread = (thread_info_t *) arg;
    /* 2. Do the task */
    pthread_barrier_wait(&barrier);
    //printf("priority is %d\n", my_thread -> sched_priority);
    for (int i = 0; i < 3; i++) {

        printf("Thread %d is running\n", my_thread->thread_id);
        struct timespec start_time = {0, 0}; 
        struct timespec pass_time = {0, 0};
        double start, pass;
        clock_gettime(CLOCK_REALTIME, &start_time);
        start = start_time.tv_sec*1000000000 + start_time.tv_nsec;
        start = start / 1000000000;

        while(1) {

        clock_gettime(CLOCK_REALTIME, &pass_time);
        pass = pass_time.tv_sec*1000000000 + pass_time.tv_nsec;
        pass = pass / 1000000000;
        if (pass > (start + my_thread -> time_wait)) {
            break;
        }
    }
        sched_yield();
        /* Busy for <time_wait> seconds */
    }

    //pthread_exit(NULL);
}

int main(int argc, char **argv) {
    
    
    /* 1. Parse program arguments */
    int ch;
    int thread_num = atoi(argv[2]);
    float wait_time;
    char *sched_method; /* read normal or fifo from command line */
    char *sched_prior; /* read priority from command line */
    int FIFO_or_Normal[thread_num]; /* transform into 1 or 0 */
    int priority[thread_num]; /* from string to number */
    const char split[] = ",";
    char *token;
    char *token_p;
    while ((ch = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch(ch) {
        case 'n':
            thread_num = atoi(optarg);
            //printf("%d\n", thread_num);
            break;
        
        case 't':
            wait_time = atof(optarg);
            //printf("%f\n", wait_time);
            break;

        case 's':
            sched_method = optarg;
            //printf("%s\n", sched_method);
            break;

        case 'p':
            sched_prior = optarg;
            //printf("%s\n", sched_prior);
            break;
    }
    }
    token = strtok(sched_method, split);
    for (int i = 0; token != NULL; i++ ) {
        //printf("i = %d\n", i);
        //printf( "%c\n", token[0]);
        if (token[0] == 'N') {
            FIFO_or_Normal[i] = 0;
        }
        else if (token[0] == 'F') {
            FIFO_or_Normal[i] = 1;
        }
        token = strtok(NULL, split);
    }

    token_p = strtok(sched_prior, split);
    for (int j = 0; token_p != NULL; j++) {
        priority[j] = atoi(token_p);   
        token_p = strtok(NULL, split);
    }


    /* 2. Create <num_threads> worker threads */
    thread_info_t my_threads_data[thread_num];
    pthread_t my_threads[thread_num];
    pthread_attr_t attr[thread_num];
    /* 3. Set CPU affinity */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
    pthread_barrier_init(&barrier, NULL, thread_num+1);

    for (int i = 0; i < thread_num; i++) {
        struct sched_param param;
        my_threads_data[i].thread_id = i;
        my_threads_data[i].sched_priority = priority[i];
        my_threads_data[i].time_wait = wait_time;
        pthread_attr_init (&attr[i]); //initialize attribute
        int setinhe = pthread_attr_setinheritsched(&attr[i], PTHREAD_EXPLICIT_SCHED);

        pthread_attr_setschedpolicy(&attr[i], FIFO_or_Normal[i]);
        if (FIFO_or_Normal[i] == 1) {
            param.sched_priority = priority[i];
            int ret = pthread_attr_setschedparam(&attr[i], &param);
            //pthread_getschedparam(&attr[i], &param);
        
            //printf("fifo waiting");
            /* set FIFO method */
            /* set priority for threads*/
        }
        pthread_create(&my_threads[i], &attr[i], thread_func, (void*) &my_threads_data[i]);

    }
    pthread_barrier_wait(&barrier);
    /* 5. Start all threads at once */
    pthread_barrier_destroy(&barrier);
    /* 6. Wait for all threads to finish  (make sure all threads can be done)*/ 
    pthread_exit(NULL);
    
}