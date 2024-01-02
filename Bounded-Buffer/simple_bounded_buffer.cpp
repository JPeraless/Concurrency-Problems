#include <iostream>
#include <mutex>


const int CART_SIZE = 10; // Maximum number of balloon figures in the cart
int cart[CART_SIZE];  // Bounded buffer for balloon figures
int producedAnimals = 0;  // keep track of produced animals
std::mutex mtx;


// Producer: Balloon Bob
void produce() {
    // Mutex is automatically unlocked when unique_lock goes out of scope
    std::unique_lock<std::mutex> lock(mtx);

    std::cout << "Producer is producing...\n";
    if (producedAnimals < 10) {
        for (int i = 0; i < CART_SIZE; i++) {
            if (cart[i] == 0) {
                std::cout << "Full = " << producedAnimals << " ; Empty = " << CART_SIZE - producedAnimals << std::endl;
                cart[i] = 1;
                producedAnimals++;

                std::cout << "Producer has produced...\n";
                std::cout << "Updated Full = " << producedAnimals << " ; Updated Empty = " << CART_SIZE - producedAnimals << std::endl;
                break;  // Break once the first 0 has been switched to a 1
            }
        }
    } else
        std::cout << "Cannot produce balloons, cart is full.\n";
}


// Consumer: Customer line
void consume() {
    // Mutex is automatically unlocked when unique_lock goes out of scope
    std::unique_lock<std::mutex> lock(mtx);

    std::cout << "Consumer is consuming...\n";
    if (producedAnimals > 0) {
        for (int i = 0; i < CART_SIZE; i++) {
            if (cart[i] == 1) {
                std::cout << "Full = " << producedAnimals << " ; Empty = " << CART_SIZE - producedAnimals << std::endl;
                cart[i] = 0;
                producedAnimals--;

                std::cout << "Consumer has consumed...\n";
                std::cout << "Updated Full = " << producedAnimals << " ; Updated Empty = " << CART_SIZE - producedAnimals << std::endl;
                break;  // Break once the first 1 has been switched to a 0
            }
        }
    } else
        std::cout << "Cannot consume balloons, cart is empty.\n";
}


void outputBufferInfo() {
    std::unique_lock<std::mutex> lock(mtx);  // Lock mutex

    // Print buffer information
    std::cout << "Buffer: ";
    for (int i = 0; i < CART_SIZE; i++)
        std::cout << '[' << i << "] -> " << cart[i] << " ;;; ";
    std::cout << "FULL SLOTS = " << producedAnimals << " AND EMPTY SLOTS = " << CART_SIZE - producedAnimals << " ;;; MUTEX IS AVAILABLE\n";
}


int main() {
    short user_choice;

    std::cout << std::endl;
    std::cout << "1. Press 1 for Producer\n";
    std::cout << "2. Press 2 for Consumer\n";
    std::cout << "3. Press 3 for Information\n";
    std::cout << "4. Press 4 to Exit\n";

    do {
        std::cout << "\nEnter your choice: ";
        if (!(std::cin >> user_choice)) {  // Input validation
            std::cin.clear(); // Clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard input
            std::cout << "Please enter a valid option (1 - 4)\n";
            continue;
        }
        switch (user_choice) {
            case 1:
                produce();
                break;
            case 2:
                consume();
                break;
            case 3:
                outputBufferInfo();
                break;
            case 4:
                std::cout << "\nExiting program\n\n";
                break;
            default:
                std::cout << "Please enter a valid option (1 - 4)\n";
                break;
        }
    } while (user_choice != 4);
}
