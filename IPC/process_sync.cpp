#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <cstring>


const int NUM_CHILDREN = 4;
const int NUM_MESSAGES = 5;


// Shared memory struct
struct SharedMemory {
    char messages[NUM_CHILDREN][256];
    int status[NUM_CHILDREN];
};

// Semaphore P (wait)
void P(int semid, int semnum) {
    struct sembuf p_op = {static_cast<short unsigned int>(semnum), -1, 0};
    semop(semid, &p_op, 1);
}

// Semaphore V (signal)
void V(int semid, int semnum) {
    struct sembuf v_op = {static_cast<short unsigned int>(semnum), 1, 0};
    semop(semid, &v_op, 1);
}


int main() {
    int shmId;
    int semId;
    key_t key = 1234;  // hardcoded key for shared memory and semaphore
    SharedMemory* shmPtr;

    // Shared memory
    shmId = shmget(key, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmId < 0) {
        std::cerr << "Error creating the shared memory\n";
        return 1;
    }

    // Shared memory attachment
    shmPtr = (SharedMemory*)shmat(shmId, NULL, 0);
    if ((long)shmPtr == -1) {
        std::cerr << "Error attaching the shared memory\n";
        return 1;
    }

    // Shared memory initialization
    memset(shmPtr->messages, 0, sizeof(shmPtr->messages));
    for (int i = 0; i < NUM_CHILDREN; ++i)
        shmPtr->status[i] = 0;

    // Semaphore creation
    semId = semget(key, 2, IPC_CREAT | 0666);
    if (semId < 0) {
        std::cerr << "Error creating semaphores\n";
        return 1;
    }

    // Initialize semaphores
    semctl(semId, 0, SETVAL, 1);
    semctl(semId, 1, SETVAL, 0);

    // Fork child processes
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        if (fork() == 0) {
            for (int j = 0; j < NUM_MESSAGES; ++j) {
                P(semId, 0);  // wait for write access

                sprintf(shmPtr->messages[i], "Message: %d, from child: %d", j, i);  // write message
                shmPtr->status[i] = 1;  // indicate pending message

                V(semId, 1);  // signal that a message is available
                sleep(1);  // wait before sending the following message
            }
            return 0;
        }
    }

    // Parent process
    for (int i = 0; i < NUM_CHILDREN * 5; ++i) {
        P(semId, 1);  // wait for message

        // Scan for messages
        for (int j = 0; j < NUM_CHILDREN; ++j) {
            if (shmPtr->status[j] == 1) {  // check for new message
                std::cout << "Received: " << shmPtr->messages[j] << std::endl;
                shmPtr->status[j] = 0;

                V(semId, 0);  // signal message processed
                break;
            }
        }
    }

    shmdt((void *) shmPtr);  // detach shared memory

    // Remove shared memory and semaphores
    shmctl(shmId, IPC_RMID, NULL);
    semctl(semId, 0, IPC_RMID);
}
