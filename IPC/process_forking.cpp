#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


int main() {
    pid_t pid;  // process ID

    pid = fork();  // create child process

    if (pid < 0) {
        perror("Error: fork failed");
        return 1;
    }
    else if (pid == 0) {  // child
        printf("\n--- Child Process ---\n");
        printf("Child Process: PID == %d, parent's PID == %d\n", getpid(), getppid());
    }
    else {  // parent
        // pid == PID of newly created child process
        printf("\n--- Parent Process ---\n");
        printf("Parent Process: PID == %d, child process's PID == %d\n", getpid(), pid);
    }

    // Both processes execute this
    printf("This message is from process with PID %d", getpid());
    printf("\n\n");
}
