//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
using namespace std;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue)
    {
        sem_init(&mSemaphore, 0, initialValue);
    }
    // Destructor
    ~Semaphore()
    {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // wait
    void wait()
    {
        sem_wait(&mSemaphore);
    }
    // signal
    void signal()
    {
        sem_post(&mSemaphore);
    }
    
    
private:
    sem_t mSemaphore;
};


class Lightswitch{
public:
    int counter;
    Semaphore mutex;

    Lightswitch() : counter(0), mutex(1) {}

    void lock(Semaphore &semaphore) {
        mutex.wait();
        counter++;
        if (counter == 1)
            semaphore.wait();
        mutex.signal();
    }

    void unlock(Semaphore &semaphore) {
        mutex.wait();
        counter--;

        if (counter == 0)
            semaphore.signal();
        mutex.signal();
    }
};

/* global vars */
const int numPhilosophers = 5;
Lightswitch readSwitch;
Lightswitch readSwitchWriter_Priority;
Lightswitch writeSwitchWriter_Priority;
Semaphore roomEmpty(1);
Semaphore turnstile(1);
Semaphore noReaders(1);
Semaphore noWriters(1);
const int bufferSize = 5;

Semaphore footman(4);
Semaphore forks[numPhilosophers] = {
    Semaphore(1),  Semaphore(1),  Semaphore(1),  Semaphore(1) , Semaphore(1)
};

enum { THINKING, HUNGRY, EATING };

int state[numPhilosophers] = { THINKING, THINKING, THINKING, THINKING, THINKING };
Semaphore sol2[numPhilosophers] = {
    Semaphore(0), Semaphore(0), Semaphore(0), Semaphore(0), Semaphore(0)
};

Semaphore mutexSol2(1);

int left(int i) { return (i + numPhilosophers - 1) % numPhilosophers; }
int right(int i) { return (i + 1) % numPhilosophers; }

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */
Semaphore Mutex(1);
Semaphore Spaces(bufferSize);
Semaphore Items(0);             


/*
    Reader function 
*/
void *Reader ( void *threadID )
{
    // Thread number 
    long x = (long)threadID;

    while( 1 )
    {
        turnstile.wait();
        turnstile.signal();
        readSwitch.lock(roomEmpty);

        printf("Reader %ld: READING\n", x);

        readSwitch.unlock(roomEmpty);
    }

}

/*
    Writer function 
*/
void *Writer ( void *threadID )
{
    // Thread number 
    long x = (long)threadID;
    
    while( 1 )
    {
        turnstile.wait();
        roomEmpty.wait();

        printf("Writer %ld: WRITING\n", x);

        turnstile.signal();
        roomEmpty.signal();
    }

}

void* readerWP(void* threadID)
{
    long id = (long)threadID;

    while (1)
    {
        noReaders.wait();

        readSwitchWriter_Priority.lock(noWriters);

        noReaders.signal();

        printf("Reader %ld (Writer Priority): READING\n", id);
        usleep(100000);

        readSwitchWriter_Priority.unlock(noWriters);
        usleep(100000);
    }
}

void* writerWP(void* threadID)
{
    long id = (long)threadID;

    while (1)
    {
        writeSwitchWriter_Priority.lock(noReaders);
        noWriters.wait();

        printf("Writer %ld (Writer priority): WRITING\n", id);
        usleep(100000);

        noWriters.signal();
        writeSwitchWriter_Priority.unlock(noReaders);

        usleep(500000);
    }
}

void* Philosophers1(void* threadID)
{
    long id = (long)threadID;

    while (1)
    
    {
        printf("Philosopher %ld (Solution 1): THINKING\n", id);

        footman.wait();

        forks[right(id)].wait();
        forks[left(id)].wait();

        printf("Philosopher %ld (Sol1): EATING\n", id);

        forks[right(id)].signal();
        forks[left(id)].signal();

        footman.signal();
    }
}

void testSol2(int i)
{
    if (state[i] == HUNGRY &&
        state[left(i)] != EATING &&
        state[right(i)] != EATING)
    {
        state[i] = EATING;
        sol2[i].signal();
    }
}

void pickupSol2(int i)
{
    mutexSol2.wait();
    state[i] = HUNGRY;
    testSol2(i);
    mutexSol2.signal();
    sol2[i].wait();    // block until allowed to eat
}

void putdownSol2(int i)
{
    mutexSol2.wait();
    state[i] = THINKING;

    testSol2(left(i));
    testSol2(right(i));

    mutexSol2.signal();
}

void* Philosophers2(void* threadID)
{
    long id = (long)threadID;

    while (1)
    {
        printf("Philosopher %ld (Sol2): THINKING\n", id);

        pickupSol2(id);

        printf("Philosopher %ld (Sol2): EATING\n", id);

        putdownSol2(id);
    }
}


int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s <problem #>\n", argv[0]);
        printf("Problems:\n");
        printf("  1 = No-starve Readers-Writers\n");
        printf("  2 = Writer-Priority Readers-Writers\n");
        printf("  3 = Dining Philosophers Solution 1\n");
        printf("  4 = Dining Philosophers Solution 2\n");
        exit(1);
    }

    int problem = atoi(argv[1]);
    pthread_t threads[10];

    if (problem == 1) {
        printf("Running Problem 1: No-starve Readers-Writers\n");

        int numReaders = 5;
        int numWriters = 5;

        for (long i = 0; i < numReaders; i++) {
            pthread_create(&threads[i], NULL, Reader, (void*)(i+1));
        }
        for (long i = 0; i < numWriters; i++) {
            pthread_create(&threads[numReaders + i], NULL, Writer, (void*)(i+1));
        }
    }

    else if (problem == 2) {
        printf("Running Problem 2: Writer-Priority Readers-Writers\n");

        int numReaders = 5;
        int numWriters = 5;

        for (long i = 0; i < numReaders; i++) {
            pthread_create(&threads[i], NULL, readerWP, (void*)(i+1));
        }
        for (long i = 0; i < numWriters; i++) {
            pthread_create(&threads[numReaders + i], NULL, writerWP, (void*)(i+1));
        }
    }

    else if (problem == 3) {
        printf("Running Problem 3: Dining Philosophers — Solution 1\n");

        for (long i = 0; i < numPhilosophers; i++) {
            pthread_create(&threads[i], NULL, Philosophers1, (void*)(i));
        }
    }

    else if (problem == 4) {
        printf("Running Problem 4: Dining Philosophers — Solution 2\n");

        for (long i = 0; i < numPhilosophers; i++) {
            pthread_create(&threads[i], NULL, Philosophers2, (void*)(i));
        }
    }

    else {
        printf("Invalid problem number: %d\n", problem);
        exit(1);
    }

    pthread_exit(NULL);

    return 0;
}




