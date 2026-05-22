#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdint.h>
#define NUM_TASKS 3
#define SPREADING 2

static _Atomic int cnt_task = NUM_TASKS;

void spread_words() {
    sleep(SPREADING);
    printf("[subordinate] spreading words...\n");
    cnt_task--;
}

void* subordinate(void* arg) {
    sleep(SPREADING);
    printf("[subordinate] as you wish\n");
    for(int i = 0; i < 3; i++) {
        spread_words();
    }

    pthread_exit(NULL); //hint1: Finish up the threads
}

void* king(void* arg) {
    pthread_t tid;
    int status;
    printf("spread the words ");

    //hint2: You should start your subordinate here
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

    status = pthread_create(&tid, NULL, king, NULL);
    if (status != 0) {
        printf("error");
        return -1;
    }
    pthread_join(tid, NULL);

    //hint3: Make the main thread wait for the subordinate to finish
    //hint3: But cannot use pthread_join because subordinate thread is detached
    //hint3: Think busy waiting
    while (cnt_task > 0) {
        // busy waiting
    }
   
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
