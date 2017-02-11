## Project Marking Problem

A C solution to the Project Marking Problem.
TODO
  - Test to see if we can remove any print statements from mutex locks, e.g. use local variables
  - Clean project


### Build & Run

- Ensure Make & gcc is installed on your system
- `make` to build
- `./demo` to run


### Program Overview

Each student and each marker is represented by a separate thread, using the Posix threads standard.

A student thread should execute the following steps:
1. Panic for a random time between 0 and (T-D-1) minutes..
2. Enter the lab.
3. Grab K markers when they are idle.
4. Do a demo for D minutes.
5. Exit from the lab.

A marker thread executes the following steps:
1. Enter the lab.
2. Repeat (N times):
  a. Wait to be grabbed by a student.
  b. Wait for the student's demo to begin.
  c. Wait for the student's demo to finish.
3. Exit from the lab.

#### Options

However, if the end of the lab session approaches and there is not time for another demo left then all students and markers not currently in a demo immediately exit the lab. In this case, the corresponding threads print a “timeout” message.

All parts not directly related to threading are implemented for you already in the skeleton program. For example, the function panic() is step 1. of the student thread and step 4. is just a call to demo().

Since your program will be run with some automated tests, do not modify the printf statements in the skeleton program. You can move them around if you change the structure of the program.

The skeleton program runs the demo with time scaled up so that 1 minute becomes 10ms. If you want to pause a certain number of “minutes”, call delay(int t) where t is the number of minutes. For example, the demo() function is simply delay(D) as a demo takes D minutes. You must not busy-wait under any circumstances.

Your program should work correctly for all reasonable values of S, M, K, N, T, and D (that is, meeting the constraints checked by the main function). Your program should work correctly for any scheduling strategy; i.e., regardless of the relative speeds of the threads.
The only parts of the program that you need to modify are the marker() and student() threads and possibly the run() function. In addition, you may want to add global variables for synchronization.


## The Problem

There are S students on a course. Each student does a project, which is assessed by K markers. There is a panel of M markers, each of whom is required to assess up to N projects. We assume that S*K ≤ M*N.

- S: Students on a course
- K: Markers
- M: Markers on a panel
- N: Projects that each M assess
- Assume S * K <= M * N
- Assume (Students * Markers) <= (Markers on a Panel * Projects)

Note that there is no guarantee that all students will complete their demos during the session even with this constraint. Near the end of the course a session is arranged at which each student demonstrates their project to K markers (together).

The session lasts T minutes and each demonstration takes exactly D minutes. Students enter the lab at random intervals during the session except in the last D minutes.

- T: Minutes a session lasts
- D: Minutes a demo lasts
- Students enter at random intervals during a session except in last D mins

All markers are on duty at the beginning of the session and each remains there until they have attended N demos or the session ends. At the end of the session all students and markers must leave the lab. Moreover, any students and markers who are not actively involved in a demo D minutes before the end must leave at that time, because there would be no time to complete a demo that started later. Whenever a student enters the lab, they start by finding K idle markers (one at a time). If there are any markers available, they will be "grabbed" by the student. Having grabbed K markers, the student does the demo and then leaves the lab. Each marker stays idle until grabbed by a student, waits until other idle markers are found (during which time the marker cannot be grabbed by other students), attends the demo and then becomes idle again, leaving the lab after attending N demos.


# Marking

- Dont submit compiled program
- Test on LAB machines
- Don't use busy-waiting


