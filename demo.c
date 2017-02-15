/*
 * This is a skeleton program for COMSM2001 (Server Software) coursework 1
 * "the project marking problem". Your task is to synchronise the threads
 * correctly by adding code in the places indicated by comments in the
 * student, marker and run functions.
 * You may create your own global variables and further functions.
 * The code in this skeleton program can be used without citation in the files
 * that you submit for your coursework.
 *
 * Author: Harry Mumford-Turner
 * Based on skeleton code from COMSM2001 cw1.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

/*
 * Debug printf statements.
 * Taken from here: http://stackoverflow.com/a/1941337
 * Called like
 *  DEBUG_PRINT(("Hi %d", 1));
 * Note: The extra parentheses are necessary, because some older C compilers don't support var-args in macros.
 */
/* 1: enables extra print statements, 0: disable */
/* #define DEBUG 1 */
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

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
 * Available markers, for easy comparison.
 * Mutex needed to lock the variable.
 */
int number_of_available_markers = 0;
pthread_mutex_t marker_available_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Finished markers, for easy comparison.
 * Mutex needed to lock the variable.
 */
int number_of_finished_markers = 0;
pthread_mutex_t finished_markers_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Condition variable for students waiting
 * for markers to come available.
 */
pthread_cond_t students_waiting_cv;

/*
 * Condition variable for markers waiting
 * to be grabbed by students.
 */
pthread_cond_t grabbed_wait_cv;

/*
 * Condition variable for markers
 * waiting until the demo has ended.
 */
pthread_cond_t demo_end_cv;

/*
 * Array of available markers.
 * arr_markers[studentID][markerID]
 *   arr_markers[MARKER_ID] = STUDENT_ID
 *  if -2 then finished jobs,
 *  if -1 then available,
 *  if positive (0+) then STUDENT_ID
 */
#define max_markers 100
int arr_markers[max_markers];


/*
 * Denotes the end of the session.
 */
int session_ended = 0;

/* Methods to safely lock and unlock the mutexes */
void lock_markers_available();
void unlock_markers_available();
void lock_finished_markers();
void unlock_finished_markers();
void safe_broadcast_students_waiting();
void safe_broadcast_grabbed_waiting();
void safe_broadcast_demo_end();

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
    printf("%d marker %d: enters lab\n", timenow(), markerID);

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
      if(timenow() > (parameters.T - parameters.D)) { DEBUG_PRINT(("%d Not enough time for a demo, jobID %d\n", timenow(), job)); break; }

      /* Become available for students to grab */
      lock_markers_available();
      number_of_available_markers++;
      DEBUG_PRINT(("%d marker %d: available. Total Available: %d\n", timenow(), markerID, number_of_available_markers));

      /* Signal all students waiting, that there is another marker free */
      safe_broadcast_students_waiting();

      /* While we are waiting */
      while(arr_markers[markerID] == -1) {
        /* (a) Wait to be grabbed by a student. */
        if(ETIMEDOUT == pthread_cond_wait(&grabbed_wait_cv, &marker_available_mutex)) {
          unlock_markers_available();
          break;
        }
        if(timenow() > (parameters.T - parameters.D)) { /* If time has run out so we dont have enough time to start a demo */
          unlock_markers_available();
          DEBUG_PRINT(("%d marker %d: Has timed out, wasn't grabbed by a student in time (job %d)\n", timenow(), markerID, job));
          break;
        }
      }

      unlock_markers_available();

      if(session_ended) {
        DEBUG_PRINT(("%d marker %d: Session Ended, freeing marker (job %d)\n", timenow(), markerID, job));
        break;
      }


      /* Get the student ID who grabbed us */
      lock_markers_available();
      /* arr_markers[markerID] == Student ID or -1 or -2 */
      studentID = arr_markers[markerID];
      if(arr_markers[markerID] == -1) {
        unlock_markers_available();
        printf("%d marker %d: SOMETHING WENT WRONG student %d (job %d)\n", timenow(), markerID, studentID, job + 1);
        break;
      }
      unlock_markers_available();

      /* The following line shall be printed when a marker is grabbed by a student. */
      printf("%d marker %d: grabbed by student %d (job %d)\n", timenow(), markerID, studentID, job + 1);



      /*
       *  (b) Wait for the student's demo to begin
       *  Nothing todo here
       */

      /*
       * (c) Wait for the demo to finish.
       *     Do not just wait a given time
       *     let the student signal when the demo is over.
       */
       lock_markers_available();
       if(ETIMEDOUT == pthread_cond_wait(&demo_end_cv, &marker_available_mutex)) {
        unlock_markers_available();
        printf("%d marker %d: exits lab (timeout)\n", timenow(), markerID);
        break;
      }
      if(timenow() > parameters.T) { /* If time has run out */
        unlock_markers_available();
        DEBUG_PRINT(("%d marker %d: Timed out during demo! (job %d)\n", timenow(), markerID, job));
        printf("%d marker %d: exits lab (timeout)\n", timenow(), markerID);
        break;
      }

       /* The following line shall be printed when a marker has finished attending a demo. */
       printf("%d marker %d: finished with student %d (job %d)\n", timenow(), markerID, studentID, job + 1);

       unlock_markers_available();

      /*
       *  (d) Exit the lab when all jobs have been completed
       */
    }

    lock_markers_available();
    arr_markers[markerID] = -2; /* Marker has left the building (lab) */
    unlock_markers_available();

    /*
     * When the marker exits the lab, exactly one of the following two lines shall be
     * printed, depending on whether the marker has finished all their jobs or there
     * is no time to complete another demo.
     */
    if(job == parameters.N) {
        printf("%d marker %d: exits lab (finished %d jobs)\n", timenow(), markerID, parameters.N);
    } else {
        printf("%d marker %d: exits lab (timeout)\n", timenow(), markerID);
    }

    lock_finished_markers();
    number_of_finished_markers++;
    unlock_finished_markers();

    return NULL;
}


/*
 * A student thread.
 */
void *student(void *arg) {
    /* The ID of the current student. */
    int studentID = *(int *)arg;

    /* 1. Panic! */
    printf("%d student %d: starts panicking\n", timenow(), studentID);
    panic();

    /* 2. Enter the lab. */
    printf("%d student %d: enters lab\n", timenow(), studentID);

    /* If there is still time to demo and there are some markers that haven't finished yet */
    lock_finished_markers();
    if(timenow() > (parameters.T - parameters.D) || (parameters.M - parameters.K) < number_of_finished_markers) {
      unlock_finished_markers();

      printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
      return NULL;
    }
    unlock_finished_markers();


    /* Wait until there are K markers available */
    lock_markers_available();
    while(number_of_available_markers < parameters.K) {
      DEBUG_PRINT(("> %d student %d: Waiting until markers (%d out of %d) are available.\n",  timenow(), studentID, number_of_available_markers, parameters.K));

      if(ETIMEDOUT == pthread_cond_wait(&students_waiting_cv, &marker_available_mutex)) {
        unlock_markers_available();
        printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
        return NULL;
      }

      if(timenow() > (parameters.T - parameters.D)) { /* If time has run out */
         unlock_markers_available();
         printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
         return NULL;
      } else {
         DEBUG_PRINT(("> %d student %d: Waiting timeout but still time to do stuff. available:%d\n",  timenow(), studentID, number_of_available_markers));
      }
    }
    DEBUG_PRINT(("> %d student %d: Found enough markers available:%d\n", timenow(), studentID, number_of_available_markers));

    /* 3. Now grab K markers. */
    int i, grab_count = 0;
    for(i = 0; i < max_markers; i++) {
      if(arr_markers[i] == -1) { /* If a marker is free */
        arr_markers[i] = studentID; /* Grab them */ /* i == MARKER_ID */
        grab_count++;
        /* Don't grab anymore markers than we need */
        if(grab_count == parameters.K) { break; }
      }
    }

    /* Signal all grabbed markers to proceed */
    number_of_available_markers -= grab_count;
    safe_broadcast_grabbed_waiting();
    unlock_markers_available();



    /* 4. Demo! */
    /*
     * If the student succeeds in grabbing K markers and there is enough time left
     * for a demo, the following three lines shall be executed in order.
     * If the student has not started their demo and there is not sufficient time
     * left to do a full demo, the following three lines shall not be executed
     * and the student proceeds to step 5.
     */
    printf("%d student %d: starts demo\n", timenow(), studentID);
    demo();
    printf("%d student %d: ends demo\n", timenow(), studentID);

    /* Release all markers by signalling them */
    lock_markers_available();
    for(i = 0; i < max_markers; i++) {
      if(arr_markers[i] == studentID) {
        /* If the marker is assigned to us, set it free */
        arr_markers[i] = -1;
      }
    }
    /* Now signal all markers to be free! */
    unlock_markers_available();
    safe_broadcast_demo_end();


    /* 5. Exit the lab. */
    printf("%d student %d: exits lab (finished)\n", timenow(), studentID);
    return NULL;
}


/*
 * The function that runs the session.
 */
void run() {

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&marker_available_mutex, NULL);
    pthread_mutex_init(&finished_markers_mutex, NULL);
    pthread_cond_init (&students_waiting_cv, NULL);
    pthread_cond_init (&grabbed_wait_cv, NULL);
    pthread_cond_init (&demo_end_cv, NULL);

    int i;
    for(i = 0; i < max_markers; i++) {
      arr_markers[i] = -1; /* init all to be free -1 */
    }

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
    for (i = 0; i < parameters.S; i++) {
      DEBUG_PRINT(("> Creating student %d thread\n", i));
      studentID[i] = i;
      if(pthread_create(&studentT[i], NULL, student, &studentID[i])) {
        fprintf(stderr, "Error creating student thread, student %d\n", i);
        exit(1);
      }
    }

    /* Create M marker threads */
    for (i = 0; i < parameters.M; i++) {
      DEBUG_PRINT(("> Creating marker %d thread\n", i));

      markerID[i] = i;


      if(pthread_create(&markerT[i], NULL, marker, &markerID[i])) {
        fprintf(stderr, "Error creating marker thread, marker %d\n", i);
        exit(1);
      }
    }

    /* With the threads now started, the session is in full swing ... */
    delay(parameters.T - parameters.D);

    /*
     * When we reach here, this is the latest time a new demo could start.
     */
    DEBUG_PRINT(("> %d ---- No more demos can start ----\n> Now ending all threads\n", timenow()));


    /* Wait for student threads to finish */
    for (i = 0; i < parameters.S; i++) {
      DEBUG_PRINT(("> Trying to end student thread %d\n", i));
      if(pthread_join(studentT[i], NULL)) {
        fprintf(stderr, "Error ending student thread student %d\n", i);
        exit(1);
      }

     }

    DEBUG_PRINT(("> %d All student threads ended.\n", timenow()));

    session_ended = 1;
    /* Wait for marker threads to finish */
    for (i = 0; i < parameters.M; i++) {
      safe_broadcast_grabbed_waiting();

      DEBUG_PRINT(("> Trying to end marker thread %d\n", i));
      if(pthread_join(markerT[i], NULL)) {
        fprintf(stderr, "Error ending student thread marker %d\n", i);
        exit(1);
      }
    }


    DEBUG_PRINT(("> %d SUCCESS! Ended all threads, cleaning up mutexes\n", timenow()));

    /* Cleanup and exit */
    pthread_mutex_destroy(&marker_available_mutex);
    pthread_mutex_destroy(&finished_markers_mutex);
    pthread_cond_destroy(&students_waiting_cv);
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

    /* We're good to go. */

    run();

    return 0;
}


/* Safe Locking/Unlocking methods */
void lock_markers_available() {
  int err = pthread_mutex_lock(&marker_available_mutex);
  if(err != 0) {
    DEBUG_PRINT(("%d lock_markers_available() return error %d \n", timenow(), err));
    exit(1);
  }
}

void unlock_markers_available() {
  int err = pthread_mutex_unlock(&marker_available_mutex);
  if(err != 0) {
    DEBUG_PRINT(("%d unlock_markers_available() return error %d \n", timenow(), err));
    exit(1);
  }
}

void lock_finished_markers() {
  int err = pthread_mutex_lock(&finished_markers_mutex);
  if(err != 0) {
    DEBUG_PRINT(("%d lock_finished_markers() return error %d \n", timenow(), err));
    exit(1);
  }
}

void unlock_finished_markers() {
  int err = pthread_mutex_unlock(&finished_markers_mutex);
  if(err != 0) {
    DEBUG_PRINT(("%d unlock_finished_markers() return error %d \n", timenow(), err));
    exit(1);
  }
}

void safe_broadcast_students_waiting() {
  int err = pthread_cond_broadcast(&students_waiting_cv);
  if(err != 0) {
    DEBUG_PRINT(("%d safe_broadcast_students_waiting() return error %d \n", timenow(), err));
    exit(1);
  }
}

void safe_broadcast_grabbed_waiting() {
  int err = pthread_cond_broadcast(&grabbed_wait_cv);
  if(err != 0) {
    DEBUG_PRINT(("%d safe_broadcast_grabbed_waiting() return error %d \n", timenow(), err));
    exit(1);
  }
}

void safe_broadcast_demo_end() {
  int err = pthread_cond_broadcast(&demo_end_cv);
  if(err != 0) {
    DEBUG_PRINT(("%d safe_broadcast_demo_end() return error %d \n", timenow(), err));
    exit(1);
  }
}