#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>


const int CART_SIZE = 10; // Maximum number of balloon figures in the cart
bool stopFlag = false;  // flag for program termination

int animalCart[CART_SIZE];  // Bounded buffer for animal balloons
int producedAnimals = 0;  // keep track of produced animals
std::mutex animalMtx;
std::condition_variable cvAnimalCart;

int houseCart[CART_SIZE];  // Bounded buffer for house balloons
int producedHouses = 0;  // keep track of produced houses
std::mutex houseMtx;
std::condition_variable cvHouseCart;


// Producer: Balloon Bob
void produceAnimalBalloons() {
    while (!stopFlag) {
        // Sleep a random amount of time (1 - 10 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 10 + 1));

        // Mutex is automatically unlocked when unique_lock goes out of scope (at end of while iteration)
        std::unique_lock<std::mutex> lock(animalMtx);

        // Wait if the cart is full (automatically releases the mutex and blocks the thread)
        cvAnimalCart.wait(lock, [] { return producedAnimals < CART_SIZE; });

        // Produce an animal balloon and print the steps
        std::cout << "\nAnimal producer is producing...\n";
        for (int i = 0; i < CART_SIZE; i++) {
            if (animalCart[i] == 0) {
                std::cout << "Full = " << producedAnimals << " ; Empty = " << CART_SIZE - producedAnimals << std::endl;
                animalCart[i] = 1;
                producedAnimals++;

                std::cout << "Animal producer has produced...\n";
                std::cout << "Updated Full = " << producedAnimals << " ; Updated Empty = " << CART_SIZE - producedAnimals << std::endl;
                break;
            }
        }

        cvAnimalCart.notify_all(); // Notify consumers that an animal has been added
    }
}


// Producer: Hellium Harry
void produceHouseBalloons() {
    while (!stopFlag) {
        // Sleep for a random time (1 - 10 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 10 + 1));

        // Mutex is automatically unlocked when unique_lock goes out of scope (at end of while iteration)
        std::unique_lock<std::mutex> lock(houseMtx);

        // Wait if the cart is full (automatically releases the mutex and blocks the thread)
        cvHouseCart.wait(lock, [] { return producedHouses < CART_SIZE; });

        // Produce a house balloon and print the steps
        std::cout << "\nHouse producer is producing...\n";
        for (int i = 0; i < CART_SIZE; i++) {
            if (houseCart[i] == 0) {
                std::cout << "Full = " << producedHouses << " ; Empty = " << CART_SIZE - producedHouses << std::endl;
                houseCart[i] = 1;
                producedHouses++;

                std::cout << "House producer has produced...\n";
                std::cout << "Updated Full = " << producedHouses << " ; Updated Empty = " << CART_SIZE - producedHouses << std::endl;
                break;
            }
        }

        cvHouseCart.notify_all(); // Notify consumers that a house has been added
    }
}


// Consumer: customers wanting only animal balloons
void consumeAnimalBalloons() {
    while (!stopFlag) {
        // Mutex is automatically unlocked when unique_lock goes out of scope (at end of while iteration)
        std::unique_lock<std::mutex> lock(animalMtx);

        // Wait if the cart is empty
        cvAnimalCart.wait(lock, [] { return producedAnimals > 0; });

        // Consume an animal balloon and print the steps
        std::cout << "\nAnimal consumer is consuming...\n";
        for (int i = 0; i < CART_SIZE; i++) {
            if (animalCart[i] == 1) {
                std::cout << "Full = " << producedAnimals << " ; Empty = " << CART_SIZE - producedAnimals << std::endl;
                animalCart[i] = 0;
                producedAnimals--;

                std::cout << "Animal consumer has consumed...\n";
                std::cout << "Updated Full = " << producedAnimals << " ; Updated Empty = " << CART_SIZE - producedAnimals << std::endl;
                break;
            }
        }

        cvAnimalCart.notify_all(); // Notify animal producer that space is available

        // Free mutex before sleeping
        lock.unlock();

        // Sleep for a random amount of time (1- 15 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 11 + 5));
    }
}


// Consumer: customers wanting only house balloons
void consumeHouseBalloons() {
    while (!stopFlag) {
        // Mutex is automatically unlocked when unique_lock goes out of scope (at end of while iteration)
        std::unique_lock<std::mutex> lock(houseMtx);

        // Wait if the cart is empty
        cvHouseCart.wait(lock, [] { return producedHouses > 0; });

        // Consume a house balloon and print the steps
        std::cout << "\nHouse consumer is consuming...\n";
        for (int i = 0; i < CART_SIZE; i++) {
            if (houseCart[i] == 1) {
                std::cout << "Full = " << producedHouses << " ; Empty = " << CART_SIZE - producedHouses << std::endl;
                houseCart[i] = 0;
                producedHouses--;

                std::cout << "House consumer has consumed...\n";
                std::cout << "Updated Full = " << producedHouses << " ; Updated Empty = " << CART_SIZE - producedHouses << std::endl;
                break;
            }
        }

        cvHouseCart.notify_all(); // Notify house producer that space is available

        // Free mutex before sleeping
        lock.unlock();

        // Sleep a random amount of time (1- 15 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 11 + 5));
    }
}


// Consumer: cusotmers wanting both balloon types
void consumeBothBalloons() {
    while (!stopFlag) {
        // Mutexes are automatically unlocked when they go out of scope (at end of while iteration)
        std::unique_lock<std::mutex> animalLock(animalMtx);
        std::unique_lock<std::mutex> houseLock(houseMtx);

        // Wait if either cart is empty
        cvAnimalCart.wait(animalLock, [] { return producedAnimals > 0; });
        cvHouseCart.wait(houseLock, [] { return producedHouses > 0; });

        // Consume both types of balloons and print the steps
        bool animalsDone = false;
        bool housesDone = false;
        std::cout << "\nAnimal & House consumer is consuming...\n";
        for (int i = 0; i < CART_SIZE; i++) {
            if (animalCart[i] == 1 && !animalsDone) {
                std::cout << "Animals: Full = " << producedAnimals << " ; Empty = " << CART_SIZE - producedAnimals << std::endl;
                animalCart[i] = 0;
                producedAnimals--;

                std::cout << "Animal & House consumer consumed an animal...\n";
                std::cout << "Animals: Updated Full = " << producedAnimals << " ; Updated Empty = " << CART_SIZE - producedAnimals << std::endl;
                animalsDone = true;
            }
            if (houseCart[i] == 1 && !housesDone) {
                std::cout << "Houses: Full = " << producedHouses << " ; Empty = " << CART_SIZE - producedHouses << std::endl;
                houseCart[i] = 0;
                producedHouses--;

                std::cout << "Animal & House consumer consumed a house...\n";
                std::cout << "Houses: Updated Full = " << producedHouses << " ; Updated Empty = " << CART_SIZE - producedHouses << std::endl;
                housesDone = true;
            }
            if (animalsDone && housesDone) break;
        }

        // Notify producers that space is available
        cvAnimalCart.notify_all();
        cvHouseCart.notify_all();

        // Free both mutexes before sleeping
        animalLock.unlock();
        houseLock.unlock();

        // Sleep a random amount of time (5 - 15 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 11 + 5));
    }
}


void outputBufferInfo() {
    while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::seconds(10));  // output with ~10 seconds in between

        // Print Animal buffer first to unlock animalMtx
        std::unique_lock<std::mutex> animalLock(animalMtx);

        std::cout << "\nAnimal buffer: ";
        for (int i = 0; i < CART_SIZE; i++)
            std::cout << '[' << i << "] -> " << animalCart[i] << " ;;; ";
        std::cout << "FULL SLOTS = " << producedAnimals << " AND EMPTY SLOTS = " << CART_SIZE - producedAnimals << " ;;; MUTEX IS AVAILABLE\n";

        animalMtx.unlock();

        // Print House buffer
        std::unique_lock<std::mutex> houseLock(houseMtx);

        std::cout << "\nHouse buffer: ";
        for (int i = 0; i < CART_SIZE; i++)
            std::cout << '[' << i << "] -> " << houseCart[i] << " ;;; ";
        std::cout << "FULL SLOTS = " << producedHouses << " AND EMPTY SLOTS = " << CART_SIZE - producedHouses << " ;;; MUTEX IS AVAILABLE\n";
    }
}


int main() {
    srand(time(nullptr));  // generate a random seed to be used by rand()

    // Create producer and consumer threads
    std::thread balloonBob(produceAnimalBalloons);
    std::thread helliumHarry(produceHouseBalloons);
    std::thread animalConsumer(consumeAnimalBalloons);
    std::thread houseConsumer(consumeHouseBalloons);
    std::thread bothConsumer(consumeBothBalloons);
    std::thread bufferInfo(outputBufferInfo);

    // Sleep for 45 seconds
    std::this_thread::sleep_for(std::chrono::seconds(45));

    // Signal threads to stop
    stopFlag = true;

    // Ensure the main thread waits for all threads before terminating the program
    balloonBob.join();
    helliumHarry.join();
    animalConsumer.join();
    houseConsumer.join();
    bothConsumer.join();
    bufferInfo.join();

    std::cout << "\n45 seconds have passed, exiting program\n\n";
}
