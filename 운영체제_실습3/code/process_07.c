#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#define SLEEP_TIME 2

int main(int argc, char* argv[]) {
    pid_t pid;
    int status;
    int parent_pid, child_pid, favorite_number;
    char favorite_fruit[] = "Apple";

    printf("Final Question!\n");

    pid = fork();

    switch (pid) {
    case -1:
        printf("fork failed\n");
        return -1;
        
    default: 
        // hint: Which case handles the parent process based on the return value of fork?
        // hint: How can the parent process retrieve its own unique PID?
        parent_pid = getpid();
        printf("[%d] I am a parent\n", parent_pid);

        // hint: How can the parent block its execution until the child finishes?
        wait(&status);

        printf("[%d] ....and my child's favorite number is %d\n", parent_pid, WEXITSTATUS(status));
        break;
        
    case 0: 
        // hint: Which case handles the child process?
        // hint: To ensure the parent prints its message first, what should the child do?
        sleep(SLEEP_TIME); 

        favorite_number = 5;

        // hint: How can the child identifying its own PID? 
        child_pid = getpid();

        // hint: How can the child identify its parent's PID?
        parent_pid = getppid();

        printf("[%d] and I am a child! And my parent's PID is [%d]\n", child_pid, parent_pid);
        printf("[%d] my parent's favorite fruit is %s but he doesn't know that my favorite number is %d\n", child_pid, favorite_fruit, favorite_number);

        // hint: How does the child terminate and pass its favorite number to the parent?
        exit(favorite_number);
        break;
    }

    return 0;
}

/*
Expected output:

Final Question!
[xxxx] I am a parent
[yyyy] and I am a child! And my parent's PID is [xxxx]
[yyyy] my parent's favorite fruit is Apple but he doesn't know that my favorite number is 5
[xxxx] ....and my child's favorite number is 5

***xxxx and yyyy are process IDs***
*/