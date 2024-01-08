#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <cctype>


// Convert string to uppercase
void to_uppercase(char *str) {
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}


int main() {
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];
    pid_t pid;
    char buffer[300];

    // Create pipes
    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        std::cerr << "Pipe creation failed\n";
        return 1;
    }

    // Fork child process
    pid = fork();
    if (pid < 0) {
        std::cerr << "Fork creation failed\n";
        return 1;
    }

    if (pid == 0) {  // child
        close(pipe_parent_to_child[1]);  // close unused write end
        close(pipe_child_to_parent[0]);  // close unused read end

        while (true) {
            ssize_t numRead = read(pipe_parent_to_child[0], buffer, sizeof(buffer));

            // If parent closed the pipe
            if (numRead <= 0)
                break;
            
            buffer[numRead] = '\0';

            std::cout << "\tChild received: " << buffer << std::endl;
            to_uppercase(buffer);
            std::cout << "\tChild sending: " << buffer << std::endl;

            // Send uppercase message back
            write(pipe_child_to_parent[1], buffer, strlen(buffer) + 1);
        }

        close(pipe_parent_to_child[0]);  // close read end
        close(pipe_child_to_parent[1]);  // close write end
    }
    else {  // parent
        close(pipe_parent_to_child[0]);  // close unused read end
        close(pipe_child_to_parent[1]);  // close unused write end

        std::string input;
        while (true) {
            std::cout << std::endl << "Enter a message (\"exit\" to quit): ";
            std::getline(std::cin, input);
            
            if (input == "exit") {
                break;
            }

            // Send message to child
            std::cout << "\tParent sending: " << input << std::endl;
            write(pipe_parent_to_child[1], input.c_str(), input.size() + 1);

            // Read response from child
            read(pipe_child_to_parent[0], buffer, sizeof(buffer));
            std::cout << "\tParent received: " << buffer << std::endl;
        }

        close(pipe_parent_to_child[1]);  // close write end
        close(pipe_child_to_parent[0]);  // close read end

        wait(NULL);  // wait for child to exit
    }
}
