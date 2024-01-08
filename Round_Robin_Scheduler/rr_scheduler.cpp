#include "scheduler.h"
#include <queue>
#include <iostream>
#include <limits>


void simulate_rr(int64_t quantum, int64_t max_seq_len, std::vector<Process> & processes, std::vector<int> & seq) {
    seq.clear();

    std::queue<int> ready_queue;
    std::queue<int> job_queue;
    std::vector<int64_t> remaining_bursts(processes.size());

    // Initialize job_queue and remaining_bursts
    for (size_t i = 0; i < processes.size(); i++) {
        job_queue.emplace(i);
        remaining_bursts[i] = processes[i].burst;
    }

    int64_t current_time = 0;

    // Continue until no processes are left
    while (!ready_queue.empty() || !job_queue.empty()) {
        if (ready_queue.empty()) {
            ready_queue.emplace(job_queue.front());
            current_time = processes[job_queue.front()].arrival_time;

            if (seq.back() != -1 && current_time != 0)
                seq.emplace_back(-1);

            job_queue.pop();
        }
        else {
            int curr_process = ready_queue.front();  // index of process being executed

            // Set start_time if process's first time being executed
            if (processes[curr_process].start_time == -1)
                processes[curr_process].start_time = current_time;


            // Optimization
            // ================================================================================
            // ================================================================================

            bool process_not_started = false;

            // Get minimum burst of processes in ready_queue
            int64_t min_burst = std::numeric_limits<int>::max();
            for (size_t i = 0; i < ready_queue.size(); i++) {
                int process = ready_queue.front();
                ready_queue.pop();
                ready_queue.push(process);
                min_burst = std::min(min_burst, remaining_bursts[process]);

                // Check if there's a process in rq that hasn't started
                if (processes[process].start_time == -1)
                    process_not_started = true;
            }

            // Only execute if all processes in rq have been started
            if (!process_not_started) {
                int64_t next_arrival = processes[job_queue.front()].arrival_time;  // arrival of next process in jq

                int n = 0;
                // Get maximum possible n ensuring that all processes' bursts are greater than the update time and
                // that no process arrives while updating the ones already in ready_queue
                while (min_burst > quantum && (current_time + (static_cast<int64_t>(ready_queue.size()) * quantum) < next_arrival))
                    n++;

                min_burst -= quantum * n;
                current_time += static_cast<int64_t>(ready_queue.size()) * quantum * n;

                if (n > 0) {
                    if (ready_queue.size() == 1) {
                        if (seq.back() != ready_queue.front() || seq.empty())
                            seq.emplace_back(ready_queue.front());
                        remaining_bursts[ready_queue.front()] -= quantum * n;
                    }
                    else {
                        // Make a vector copy of ready_queue
                        std::vector<int> rq_copy(ready_queue.size());
                        for (size_t i = 0; i < ready_queue.size(); i++) {
                                // std::cout << "Vector copy" << std::endl;
                                int process = ready_queue.front();
                                ready_queue.pop();
                                ready_queue.push(process);
                                rq_copy[i] = process;
                        }

                        // Append sequence of execution of the processes
                        if (seq.back() == rq_copy.front()) {  // handles first process being repeated
                            seq.insert(seq.end(), rq_copy.begin() + 1, rq_copy.end());

                            for (int i = 0; i < n - 1; i++) {
                                if (seq.size() > static_cast<size_t>(max_seq_len))
                                    break;
                                seq.insert(seq.end(), rq_copy.begin(), rq_copy.end());
                            }
                        } else {
                            for (int i = 0; i < n; i++) {
                                if (seq.size() > static_cast<size_t>(max_seq_len))
                                        break;
                                seq.insert(seq.end(), rq_copy.begin(), rq_copy.end());
                            }
                        }

                        // Update remaining bursts of all updated processes
                        for (const int &process : rq_copy)
                            remaining_bursts[process] -= quantum * n;
                    }
                }
            }

            // ================================================================================
            // ================================================================================

            ready_queue.pop();

            int64_t update_time = std::min(remaining_bursts[curr_process], quantum);  // max possible update time
            remaining_bursts[curr_process] -= update_time;  // update process remaining time
            current_time += update_time;  // update current time
            processes[curr_process].finish_time = current_time;  // update process finish_time

            // Add processes that arrived during execution
            while (!job_queue.empty() && processes[job_queue.front()].arrival_time < current_time) {
                ready_queue.emplace(job_queue.front());
                job_queue.pop();
            }

            // Push to queue if process isn't done
            if (remaining_bursts[curr_process] > 0)
                ready_queue.emplace(curr_process);

            // Add process id to seq if it's not equal to the last one
            if (seq.back() != curr_process || seq.empty())
                seq.emplace_back(curr_process);
        }
    }

    // Trim seq to max_seq_len
    if (seq.size() > static_cast<size_t>(max_seq_len))
        seq.resize(max_seq_len);
}
