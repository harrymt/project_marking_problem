/*
 * This is a skeleton program for COMSM2001 (Server Software) coursework 1
 * "the project marking problem". Your task is to synchronise the threads
 * correctly by adding code in the places indicated by comments in the
 * student, marker and run functions.
 * You may create your own global variables and further functions.
 * The code in this skeleton program can be used without citation in the files
 * that you submit for your coursework.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

/*
 * Parameters of the program. The constraints are D < T and
 * S*K <= M*N.
 */
struct demo_parameters {
    int S;   /* Number of students */
    int M;   /* Number of markers */
    int K;   /* Number of markers per demo */
    int N;   /* Number of demos per marker */
    int T;   /* Length of session (minutes) */
    int D;   /* Length of demo (minutes) */
};

/* Global object holding the demo parameters. */
struct demo_parameters parameters;

/* The demo start time, set in the main function. Do not modify this. */
struct timeval starttime;

/*
 * You may wish to place some global variables here.
 * Remember, globals are shared between threads.
 * You can also create functions of your own.
 */

/* Lock for marker when waiting to be grabbed by a student */
// static pthread_mutex_t marker_lock = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t students_waiting_for_markers_condition;
// int number_of_finished_markers = 0;


int number_of_available_markers = 0;
int number_of_finished_markers = 0;

pthread_mutex_t marker_available_mutex;
pthread_mutex_t finished_markers_mutex;
pthread_mutex_t demo_end_mutex;
pthread_cond_t signal_students_waiting_cv;
pthread_cond_t grabbed_wait_cv;
pthread_cond_t demo_end_cv;

#define max_markers 100

// If its -2 left the lab
// If its -1 its available
// If its positive, then its the student ID its marking
int arr_markers[max_markers]; // Maps between the MarkerID and its availability

// extern int nanosleep();
// extern long int random(void);

/*
 * timenow(): returns current simulated time in "minutes" (cs).
 * Assumes that starttime has been set already.
 * This function is safe to call in the student and marker threads as
 * starttime is set in the run() function.
 */
int timenow() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - starttime.tv_sec) * 100 + (now.tv_usec - starttime.tv_usec) / 10000;
}

/* delay(t): delays for t "minutes" (cs) */
void delay(int t) {
    struct timespec rqtp, rmtp;
    t *= 10;
    rqtp.tv_sec = t / 1000;
    rqtp.tv_nsec = 1000000 * (t % 1000);
    nanosleep(&rqtp, &rmtp);
}

/* panic(): simulates a student's panicking activity */
void panic() {
    delay(random() % (parameters.T - parameters.D));
}

/* demo(): simulates a demo activity */
void demo() {
    delay(parameters.D);
}

void debug_delay() {
    delay(10); // Delay for 2 mins
}

/*
 * A marker thread. You need to modify this function.
 * The parameter arg is the number of the current marker and the function
 * doesn't need to return any values.
 * Do not modify the printed output as it will be used as part of the testing.
 */
void *marker(void *arg) {
    int markerID = *(int *)arg;

    /*
     * The following variable is used in the printf statements when a marker is
     * grabbed by a student. It shall be set by this function whenever the
     * marker is grabbed - and before the printf statements referencing it are
     * executed.
     */
    int studentID = 0;

    /* 1. Enter the lab. */
    printf("%d marker %d: enters lab m1\n", timenow(), markerID);

    /**
     * The current job number, i.e. the demo to mark.
     */
    int job;

    /*
     * A marker marks up to N projects.
     * Repeat (N times).
    */
    for (job = 0; job < parameters.N; job++) {

        /*
         *    If the end of the session approaches (i.e. there is no time
         *    to start another demo) then the marker waits for the current
         *    demo to finish (if they are currently attending one) and then
         *    exits the lab.
         */
        if(timenow() >= parameters.T - parameters.D) { printf("%d Not enough time for a demo, jobID %d", timenow(), job); break; }

        /* Become available for students to grab */
        pthread_mutex_lock(&marker_available_mutex);
        number_of_available_markers++;
        /* Signal all students waiting, that there is another marker free */
        pthread_cond_signal(&signal_students_waiting_cv);
        pthread_mutex_unlock(&marker_available_mutex);

        /* (a) Wait to be grabbed by a student. */
        pthread_mutex_lock(&marker_available_mutex);
        pthread_cond_wait(&grabbed_wait_cv, &marker_available_mutex);

        /* The following line shall be printed when a marker is grabbed by a student. */
        printf("%d marker %d: grabbed by student %d (job %d) m2\n", timenow(), markerID, studentID, job + 1);

        number_of_available_markers--;
        pthread_mutex_unlock(&marker_available_mutex);

        /*
         *  (b) Wait for the student's demo to begin
         *  Nothing todo here
         */

        /*
         * (c) Wait for the demo to finish.
         *     Do not just wait a given time
         *     let the student signal when the demo is over.
         */
         pthread_mutex_lock(&demo_end_mutex);
         pthread_cond_wait(&demo_end_cv, &demo_end_mutex);

         /* The following line shall be printed when a marker has finished attending a demo. */
         printf("%d marker %d: finished with student %d (job %d) m3\n", timenow(), markerID, studentID, job + 1);

         pthread_mutex_unlock(&demo_end_mutex);

        /*
         *  (d) Exit the lab when all jobs have been completed
         */
    }

    /*
     * When the marker exits the lab, exactly one of the following two lines shall be
     * printed, depending on whether the marker has finished all their jobs or there
     * is no time to complete another demo.
     */
    if(job == parameters.N) {
        printf("%d marker %d: exits lab (finished %d jobs) m4a\n", timenow(), markerID, parameters.N);
    } else {
        printf("%d marker %d: exits lab (timeout) m4b\n", timenow(), markerID);
    }

    pthread_mutex_lock(&finished_markers_mutex);
    number_of_finished_markers++;
    pthread_mutex_unlock(&finished_markers_mutex);

    return NULL;
}


/*
 * A student thread. You must modify this function.
 */
void *student(void *arg) {
    /* The ID of the current student. */
    int studentID = *(int *)arg;

    /* 1. Panic! */
    printf("%d student %d: starts panicking 1\n", timenow(), studentID);
    panic();

    /* 2. Enter the lab. */
    printf("%d student %d: enters lab 2\n", timenow(), studentID);

    /* If there is still time to demo and there are some markers that haven't finished yet */
    pthread_mutex_lock(&finished_markers_mutex);
    if(timenow() >= (parameters.T - parameters.D) || (parameters.M - parameters.K) < number_of_finished_markers) {
      pthread_mutex_unlock(&finished_markers_mutex);

      printf("%d student %d: exits lab (timeout) 5b\n", timenow(), studentID);
      return NULL;
    }
    pthread_mutex_unlock(&finished_markers_mutex);


    /* Wait until there are K markers available */
    pthread_mutex_lock(&marker_available_mutex);
    while(number_of_available_markers < parameters.K) {
      pthread_cond_wait(&signal_students_waiting_cv, &marker_available_mutex);
    }

    /* 3. Now grab K markers. */
    int i, grab_count = 0;
    for(i = 0; i < max_markers; i++) {
      if(arr_markers[i] == -1) { /* If a marker is free */
        arr_markers[i] = studentID; /* Grab them */
        grab_count++;
        /* Don't grab anymore markers than we need */
        if(grab_count == parameters.K) { break; }
      }
    }
    pthread_mutex_unlock(&marker_available_mutex);


    /* If we have enough markers for our demo! */
    if(grab_count != parameters.K) {
      printf("Error something went horribly wrong, we didn't have enough markers for a demo! GrabCount: %d, ", grab_count);
      return NULL;
    }

    /* 4. Demo! */
    /*
     * If the student succeeds in grabbing K markers and there is enough time left
     * for a demo, the following three lines shall be executed in order.
     * If the student has not started their demo and there is not sufficient time
     * left to do a full demo, the following three lines shall not be executed
     * and the student proceeds to step 5.
     */
    printf("%d student %d: starts demo 3\n", timenow(), studentID);
    demo();
    printf("%d student %d: ends demo 4\n", timenow(), studentID);

    /* Release all markers by signalling them */
    pthread_mutex_lock(&marker_available_mutex);
    for(i = 0; i < max_markers; i++) {
      if(arr_markers[i] == studentID) { /* If a marker is assigned to this student */
        arr_markers[i] = -1; /* Set it to be free */
      }
    }
    pthread_mutex_unlock(&marker_available_mutex);

    /* Now signal all markers to be free! */
    pthread_cond_signal(&demo_end_cv);

    /* 5. Exit the lab. */
    printf("%d student %d: exits lab (finished) 5a\n", timenow(), studentID);

    return NULL;
}

/* The function that runs the session.
 * You MAY want to modify this function.
 */
void run() {

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&marker_available_mutex, NULL);
    pthread_mutex_init(&finished_markers_mutex, NULL);
    pthread_mutex_init(&demo_end_mutex, NULL);
    pthread_cond_init (&signal_students_waiting_cv, NULL);
    pthread_cond_init (&grabbed_wait_cv, NULL);
    pthread_cond_init (&demo_end_cv, NULL);

    int i;
    int markerID[100], studentID[100];
    pthread_t markerT[100], studentT[100];

    printf("S=%d M=%d K=%d N=%d T=%d D=%d\n",
        parameters.S,
        parameters.M,
        parameters.K,
        parameters.N,
        parameters.T,
        parameters.D);
    gettimeofday(&starttime, NULL);  /* Save start of simulated time */

    /* Create S student threads */
    for (i = 0; i<parameters.S; i++) {
        studentID[i] = i;
        if(pthread_create(&studentT[i], NULL, student, &studentID[i])) {
          fprintf(stderr, "Error creating student thread, studentID %d\n", i);
          exit(1);
        }
    }

    /* Create M marker threads */
    for (i = 0; i<parameters.M; i++) {
        markerID[i] = i;
        if(pthread_create(&markerT[i], NULL, marker, &markerID[i])) {
          fprintf(stderr, "Error creating marker thread, markerID %d\n", i);
          exit(1);
        }
    }

    /* With the threads now started, the session is in full swing ... */
    delay(parameters.T - parameters.D);

    /*
     * When we reach here, this is the latest time a new demo could start.
     *
     */

    /* Wait for student threads to finish */
    for (i = 0; i < parameters.S; i++) {
        if(pthread_join(studentT[i], NULL)) {
          fprintf(stderr, "Error ending student thread studentID %d\n", i);
          exit(1);
        }

        // if(timenow() >= parameters.T) { break; } // If timeout signal has occured?
    }

    /* Wait for marker threads to finish */
    for (i = 0; i < parameters.M; i++) {
      if(pthread_join(markerT[i], NULL)) {
        fprintf(stderr, "Error ending student thread markerID %d\n", i);
        exit(1);
      }
    }

    /* Cleanup and exit */
    pthread_mutex_destroy(&marker_available_mutex);
    pthread_mutex_destroy(&finished_markers_mutex);
    pthread_mutex_destroy(&demo_end_mutex);
    pthread_cond_destroy(&signal_students_waiting_cv);
    pthread_cond_destroy(&grabbed_wait_cv);
    pthread_cond_destroy(&demo_end_cv);
    pthread_exit(NULL);
}

/*
 * main() checks that the parameters are ok. If they are, the interesting bit
 * is in run() so please don't modify main().
 */
int main(int argc, char *argv[]) {
    if (argc < 6) {
        puts("Usage: demo S M K N T D\n");
        exit(1);
    }
    parameters.S = atoi(argv[1]);
    parameters.M = atoi(argv[2]);
    parameters.K = atoi(argv[3]);
    parameters.N = atoi(argv[4]);
    parameters.T = atoi(argv[5]);
    parameters.D = atoi(argv[6]);
    if (parameters.M > 100 || parameters.S > 100) {
        puts("Maximum 100 markers and 100 students allowed.\n");
        exit(1);
    }
    if (parameters.D >= parameters.T) {
        puts("Constraint D < T violated.\n");
        exit(1);
    }
    if (parameters.S*parameters.K > parameters.M*parameters.N) {
        puts("Constraint S*K <= M*N violated.\n");
        exit(1);
    }

    // We're good to go.

    run();

    return 0;
}
