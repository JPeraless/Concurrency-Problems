#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cctype>
#include <sys/wait.h>


// Shared data struct
struct SharedData {
    volatile int flag;  // simple lock; 0 == unlocked, 1 == locked
    char message[1024];  // message buffer
};


// Convert string to uppercase
void to_uppercase(char *str) {
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}


int main() {
    key_t shm_key = IPC_PRIVATE;  // use hardcoded key

    // Create shared memory
    int shm_id = shmget(shm_key, sizeof(SharedData), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget error");
        return 1;
    }

    // Attach to shared memory
    SharedData *shared_data = static_cast<SharedData*>(shmat(shm_id, nullptr, 0));
    if (reinterpret_cast<intptr_t>(shared_data) == -1) {
        perror("shmat error");
        shmctl(shm_id, IPC_RMID, nullptr);
        return 1;
    }

    shared_data->flag = 0;  // initialize shared data unlocked

    // Forking a child
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        shmdt(shared_data);
        shmctl(shm_id, IPC_RMID, nullptr);
        return 1;
    }

    if (pid == 0) {  // child
        while (true) {
            // Wait until flag is set by the parent
            while (shared_data->flag != 1) {
                usleep(100);
            }

            // Output and process message
            std::cout << "\tChild received: " << shared_data->message << std::endl;
            if (strcmp(shared_data->message, "exit") == 0) {
                shared_data->flag = 0;
                break;
            }
            to_uppercase(shared_data->message);
            std::cout << "\tChild sending: " << shared_data->message << std::endl;

            shared_data->flag = 0;  // reset flag to indicate processing is done
        }

        // Detach from shared memory
        shmdt(shared_data);
        _exit(0);
    }
    else {  // parent
        std::string input;
        do {
            std::cout << std:: endl << "Enter a message (\"exit\" to quit): ";
            getline(std::cin, input);

            std::cout << "\tParent sending: " << input << std::endl;

            // Copy input to shared memory
            strncpy(shared_data->message, input.c_str(), sizeof(shared_data->message));
            shared_data->message[sizeof(shared_data->message) - 1] = '\0';  //  null-termination

            shared_data->flag = 1;  // indicate that message is ready

            // Wait for child to reset flag
            while (shared_data->flag != 0) {
                usleep(100);
            }

            // Output uppercase message from child
            if (input != "exit")
                std::cout << "\tParent received: " << shared_data->message << std::endl;

        } while (input != "exit");

        // Give child a chance to process "exit" message
        while (shared_data->flag != 0) {
            usleep(100);
        }

        wait(nullptr);  // wait for child to exit

        // Detach and remove shared memory
        shmdt(shared_data);
        shmctl(shm_id, IPC_RMID, nullptr);
    }
}
