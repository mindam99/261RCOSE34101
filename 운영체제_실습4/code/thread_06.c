#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdint.h>
#define NUM_TASKS 3
#define SPREADING 2

static _Atomic int cnt_task = NUM_TASKS;
//hint1: Use cond to eliminate the busy waiting
//hint1: A mutex is required to use condition variables.
pthread_mutex_t lock;
pthread_cond_t cond;

void spread_words() {
    sleep(SPREADING);
    printf("[subordinate] spreading words...\n");
    
    pthread_mutex_lock(&lock);
    cnt_task--;
    if (cnt_task == 0) {
        //hint2: Wake up the main thread when cnt_task becomes 0.
        pthread_cond_broadcast(&cond);
    }
    pthread_mutex_unlock(&lock);
}

void* subordinate(void* arg) {
    sleep(SPREADING);
    printf("[subordinate] as you wish\n");
    for(int i = 0; i < 3; i++) {
        spread_words();
    }
    //hint3: Finish up the threads
    pthread_exit(NULL);
}

void* king(void* arg) {
    pthread_t tid;
    int status;
    printf("spread the words ");

    //hint4: You should start your subordinate here
    status = pthread_create(&tid, NULL, subordinate, NULL);
    if (status != 0) {
        printf("error");
        pthread_exit(NULL);
    }

    pthread_detach(tid);

    printf("that I am king!\n");
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    pthread_t tid;
    int status;
    //hint5: Initialize the mutex and condition variable before use.
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    status = pthread_create(&tid, NULL, king, NULL);
    if (status != 0) {
        printf("error");
        return -1;
    }
    pthread_join(tid, NULL);

    //hint6: Eliminate busy waiting by using a mutex and condition variable.
    //hint6: This solution should be an upgraded version of thread_05.c.
    pthread_mutex_lock(&lock);
    while (cnt_task > 0) {
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    //hint7: You have to clean up mutex and cond
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);

    printf("The words have been spread...\n");
    return 0;
}

/*
Expected output:

spread the words that I am king!
[subordinate] as you wish
[subordinate] spreading words...
[subordinate] spreading words...
[subordinate] spreading words...
The words have been spread...

*/
