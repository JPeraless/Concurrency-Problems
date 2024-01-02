#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>


const int NUM_ASTRONOMERS = 10;
const int NUM_ASYMMETRIC = 4;
const int NUM_GREEDY = 3;
const int AVG_EAT_TIME = 2;  // center of the normal distribution for eating times
const int MAX_WAIT_TIME = 2;

bool stopFlag = false;  // flag for program termination

std::vector<std::timed_mutex> chopstickMutexes(NUM_ASTRONOMERS);  // mutexes for each chopstick
std::vector<int> eatCount(NUM_ASTRONOMERS, 0);  // tracks no. of times each astronomer has eaten
std::mutex eatCountMutex;  // mutex to protect access to eatCount

// Enums for possible states
enum class AstronomerState { CONTEMPLATING, CONSUMING };
enum class ChopstickState { AVAILABLE, IN_USE };

// Vectors to track states
std::vector<AstronomerState> astronomerStates(NUM_ASTRONOMERS, AstronomerState::CONTEMPLATING);
std::vector<ChopstickState> chopstickStates(NUM_ASTRONOMERS, ChopstickState::AVAILABLE);
std::mutex stateMutex;  // mutex for state updates


int generate_random_eating_time() {
    // thread-local random number generator
    static thread_local std::mt19937 gen(std::random_device{}());

    // Normal distribution centered on AVG_EAT_TIME with standard deviation of 1
    std::normal_distribution<> dis(AVG_EAT_TIME, 1.0);

    // random eating time of at least 1 second
    int eatTime = std::max(1, static_cast<int>(std::round(dis(gen))));
    return eatTime;
}


bool shouldYieldTurn(int astronomerId) {
    // Evaluated before allowing an astronomer to eat to prevent starvation

    std::lock_guard<std::mutex> lock(eatCountMutex);

    // IDs of left and right astronomers
    int left = (astronomerId - 1 + NUM_ASTRONOMERS) % NUM_ASTRONOMERS;
    int right = (astronomerId + 1) % NUM_ASTRONOMERS;

    // Since greedy astronomers eat twice at a time, the maximum difference
    // between a greedy astronomer and its neighbours is THRESHOLD + 1

    const int THRESHOLD = 3;  // max allowed difference in times eaten between astronomers sharing a chopstick
    if ((eatCount[astronomerId] - eatCount[left] >= THRESHOLD) || (eatCount[astronomerId] - eatCount[right] >= THRESHOLD))
        return true;
    return false;
}


void eat(int astronomerId, bool greedy) {
    // IDs of left and right chopsticks
    int left = astronomerId;
    int right = (astronomerId + 1) % NUM_ASTRONOMERS;

    // Chopsticks being used
    {
        std::lock_guard<std::mutex> stateLock(stateMutex);
        chopstickStates[left] = ChopstickState::IN_USE;
        chopstickStates[right] = ChopstickState::IN_USE;
    }

    // Check if astronomer should yield his turn
    if (shouldYieldTurn(astronomerId)) {
        // Chopsticks now available
        std::lock_guard<std::mutex> stateLock(stateMutex);
        chopstickStates[left] = ChopstickState::AVAILABLE;
        chopstickStates[right] = ChopstickState::AVAILABLE;
        return;
    }

    // Astronomer consuming
    {
        std::lock_guard<std::mutex> stateLock(stateMutex);
        astronomerStates[astronomerId] = AstronomerState::CONSUMING;
    }

    // Eating
    int eatingTime = generate_random_eating_time();
    if (greedy)
        eatingTime *= 2;

    std::this_thread::sleep_for(std::chrono::seconds(eatingTime));

    // Astronomer contemplating
    {
        std::lock_guard<std::mutex> stateLock(stateMutex);
        astronomerStates[astronomerId] = AstronomerState::CONTEMPLATING;
    }

    // Chopsticks now available
    {
        std::lock_guard<std::mutex> stateLock(stateMutex);
        chopstickStates[left] = ChopstickState::AVAILABLE;
        chopstickStates[right] = ChopstickState::AVAILABLE;
    }

    // Update eat count
    {
        std::lock_guard<std::mutex> lock(eatCountMutex);
        if (greedy)
            eatCount[astronomerId]++;
        eatCount[astronomerId]++;
    }
}


void symAstronomer(int astronomerId) {
    while (!stopFlag) {
        // IDs of left and right chopsticks
        int left = astronomerId;
        int right = (astronomerId + 1) % NUM_ASTRONOMERS;

        // Construct locks but don't attempt to acquire them
        std::unique_lock<std::timed_mutex> leftLock(chopstickMutexes[left], std::defer_lock);
        std::unique_lock<std::timed_mutex> rightLock(chopstickMutexes[right], std::defer_lock);

        // Attempt to acquire both chopsticks without waiting
        bool rightAcquired = rightLock.try_lock();
        bool leftAcquired = leftLock.try_lock();

        // If neither lock is acquired try again in the next iteration
        if (!rightAcquired && !leftAcquired) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // If only one chopstick is acquired, try to get the other within 0.5 seconds
        std::chrono::milliseconds timeout(500);
        if (rightAcquired && !leftAcquired) {
            // Update right chopstick state
            {
                std::lock_guard<std::mutex> stateLock(stateMutex);
                chopstickStates[right] = ChopstickState::IN_USE;
            }

            if (!leftLock.try_lock_for(timeout)) {
                rightLock.unlock();  // if left one wasn't acquired in time

                // Update right chopstick state
                {
                    std::lock_guard<std::mutex> stateLock(stateMutex);
                    chopstickStates[right] = ChopstickState::AVAILABLE;
                }
            }
        } else if (leftAcquired && !rightAcquired) {
            // Update left chopstick state
            {
                std::lock_guard<std::mutex> stateLock(stateMutex);
                chopstickStates[left] = ChopstickState::IN_USE;
            }

            if (!rightLock.try_lock_for(timeout)) {
                leftLock.unlock();  // if right one wasn't acquired in time

                // Update left chopstick state
                {
                    std::lock_guard<std::mutex> stateLock(stateMutex);
                    chopstickStates[left] = ChopstickState::AVAILABLE;
                }
            }
        }

        // If both chopsticks are acquired, try to eat
        if (rightLock.owns_lock() && leftLock.owns_lock()) {
            eat(astronomerId, false);
            leftLock.unlock();
            rightLock.unlock();
        }

        // Allow other astronomers to grab chopsticks before attempting to eat again
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


void asymAstronomer(int astronomerId) {
    while (!stopFlag) {
        // IDs of left and right chopsticks
        int left = astronomerId;
        int right = (astronomerId + 1) % NUM_ASTRONOMERS;

        std::unique_lock<std::timed_mutex> rightLock(chopstickMutexes[right], std::defer_lock);

        // Attempt to acquire the right chopstick
        if (rightLock.try_lock()) {
            // Update right chopstick state as it will be locked before eating due to delay
            std::unique_lock<std::mutex> stateLock(stateMutex);
            chopstickStates[right] = ChopstickState::IN_USE;
            stateLock.unlock();

            std::this_thread::sleep_for(std::chrono::seconds(1));  // wait for at least 1 second

            std::unique_lock<std::timed_mutex> leftLock(chopstickMutexes[left], std::defer_lock);
            // Attempt to acquire left chopstick within 2 seconds
            if (leftLock.try_lock_for(std::chrono::seconds(2))) {
                eat(astronomerId, false);
                leftLock.unlock();
            }

            rightLock.unlock();
        }

        // Allow other astronomers to grab the chopsticks before attempting to eat again
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


void greedyAstronomer(int astronomerId) {
    while (!stopFlag) {
        // IDs of left and right chopsticks
        int left = astronomerId;
        int right = (astronomerId + 1) % NUM_ASTRONOMERS;

        // Construct locks but don't attempt to acquire them
        std::unique_lock<std::timed_mutex> leftLock(chopstickMutexes[left], std::defer_lock);
        std::unique_lock<std::timed_mutex> rightLock(chopstickMutexes[right], std::defer_lock);

        // Attempt to acquire each lock for secs
        std::chrono::seconds timeout(2);
        if (leftLock.try_lock_for(timeout) && rightLock.try_lock_for(timeout)) {
            eat(astronomerId, true);

            // In a multithreaded context, true simultaneity at the instruction level for releasing various
            // locks isn't achievable due to the way threads are scheduled and executed. Unlocking one right
            // after the other is a standard and practical way to release them in a multithreaded context
            leftLock.unlock();
            rightLock.unlock();
        }

        // If a lock was acquired, unlock it
        if (leftLock.owns_lock())
            leftLock.unlock();
        else if (rightLock.owns_lock())
            rightLock.unlock();

        // Allow other astronomers to grab the chopsticks before attempting to eat again
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


std::vector<int> placeAstronomers(){
    // place astronomer types randomly
    // asymmetric == 0, symmetric == 1, greedy == 2

    std::vector<int> order;
    for (int i = 0; i < NUM_ASYMMETRIC; i++)
        order.emplace_back(0);

    for (int i = 0; i < NUM_GREEDY; i++)
        order.emplace_back(1);

    int NUM_SYM = NUM_ASTRONOMERS - NUM_ASYMMETRIC - NUM_GREEDY;
    for (int i = 0; i < NUM_SYM; i++)
        order.emplace_back(2);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(order.begin(), order.end(), gen);

    return order;
}


void outputInfo(std::vector<char> astrInitials) {
    // Visualize astronomer and chopstick states periodically
    while (!stopFlag) {
        std::unique_lock<std::mutex> stateLock(stateMutex);
        std::unique_lock<std::mutex> eatLock(eatCountMutex);

        // Astronomer initials (A for asymmetric, S for symmetric, G for greedy)
        std::cout << std::endl << std::endl << "             ";
        for (char initial : astrInitials)
            std::cout << initial << ' ';

        // Eating count of each astronomer
        std::cout << std::endl << "Times eaten: ";
        for (int count : eatCount)
            std::cout << count << ' ';
        std::cout << std::endl << std::endl;

        // Astronomer and chopstick states
        for (int i = 0; i < NUM_ASTRONOMERS; i++) {
            std::cout << "    Chopstick " << i << " is " <<
                (chopstickStates[i] == ChopstickState::AVAILABLE ? "AVAILABLE" : "IN USE") << '\n';
            std::cout << "(" << astrInitials[i] << ") Astronomer " << i << " is " <<
                (astronomerStates[i] == AstronomerState::CONTEMPLATING ? "CONTEMPLATING" : "CONSUMING") << '\n';
        }
        std::cout << std::endl;

        stateLock.unlock();
        eatLock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // update interval
    }
}


int main() {
    // Generate astronomers
    std::vector<int> astronomers = placeAstronomers();
    std::vector<std::thread> threads;
    std::vector<char> astrInitials;

    // Initialize astronomer threads
    for (int i = 0; i < NUM_ASTRONOMERS; i++) {
        if (astronomers[i] == 0) {
            threads.emplace_back(asymAstronomer, i);  // initialize asymAstronomer
            astrInitials.emplace_back('A');
        } else if (astronomers[i] == 1) {
            threads.emplace_back(symAstronomer, i);  // initialize symAstronomer
            astrInitials.emplace_back('S');
        } else {
            threads.emplace_back(greedyAstronomer, i);  // initialize greedyAstronomer
            astrInitials.emplace_back('G');
        }
    }

    std::thread visualizeStates(outputInfo, astrInitials);  // state visualization thread

    std::this_thread::sleep_for(std::chrono::seconds(45));  // sleep 45 seconds
    stopFlag = true;  // signal threads to stop

    // Join all threads before terminating
    for (int i = 0; i < NUM_ASTRONOMERS; i++)
        threads[i].join();
    visualizeStates.join();

    std::cout << "\n45 seconds have passed, exiting program\n\n";
}
