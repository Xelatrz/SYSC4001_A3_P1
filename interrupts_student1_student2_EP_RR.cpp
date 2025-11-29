/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue

        static std::vector<std::pair<int, unsigned int>> io_time;
        static std::vector<std::pair<int, unsigned int>> cpu_since_io;

        auto get_io_remaining = [&](int pid) -> unsigned int {
            for (auto &p : io_time) {
                if (p.first == pid) {
                    return p.second;
                }
            }
            return 0;
        };
        auto set_io_remaining = [&](int pid, unsigned int v) {
            for (auto &p : io_time) {
                if (p.first == pid) {
                    p.second = v;
                    return;
                }
            }
            io_time.push_back({pid, v});
        };
        auto erase_io = [&](int pid) {
            for (size_t i = 0; i < io_time.size(); ++i) {
                if (io_time[i].first == pid) {
                    io_time.erase(io_time.begin() + i);
                    return;
                }
            }
        };
        auto get_cpu_since = [&](int pid) -> unsigned int {
            for (auto &p : cpu_since_io) {
                if (p.first == pid) {
                    return p.second;
                }
            }
            return 0;
        };
        auto set_cpu_since = [&](int pid, unsigned int v) {
            for (auto &p : cpu_since_io) {
                if (p.first == pid) {
                    p.second = v;
                    return;
                }
            }
            cpu_since_io.push_back({pid, v});
        };
        auto erase_cpu_since = [&](int pid, unsigned int v) {
            for (size_t i = 0; i < cpu_since_io.size(); ++i) {
                if (cpu_since_io[i].first == pid) {
                    cpu_since_io.erase(cpu_since_io.begin() + i);
                    return;
                }
            }
        };

        for (size_t i = 0; i < wait_queue.size(); ) {
            PCB &p = wait_queue[i];
            if (get_io_remaining(p.PID) == 0) {
                set_io_remaining(p.PID, p.io_duration);
            }

            unsigned int remaining get_io_remaining(p.PID);
            if (remaining > 0) {
                remaining -= 1;
            }
            set_io_remaining(p.PID, remaining);
            if (remaining == 0) {

                PCB finished = p;
                finished.state = READY;
                sync_queue(job_list, finished);
                ready_queue.push_back(finished);
                execution_status += print_exec_status(current_time, finished.PID, WAITING, READY);

                erase_io(finished.PID);
                erase_cpu_since(finished.PID);
                wait_queue.erase(wait_queue.begin() + i);

            } else {
                ++i;
            }
        }

        for (auto &job : job_list) {
            if (job.state == NOT_ASSIGNED) {
                PCB temp = job;
                if (assign_memory(temp)) {
                    temp.state = READY;
                    sync_queue(job_list, temp);
                    ready_queue.push_back(temp);
                    execution_status += print_exec_status(current_time, temp.PID, NOT_ASSIGNED, READY);
                }
            }
        }

        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        
        const unsigned int QUANTUM = 100;
        static unsigned int quantum_used = 0

        auto best_index_ready = [&]() -> int {
            if (ready_queue.empty()) {
                return -1;
            }
            int best = 0;
            for (size_t i = 1; i < ready_queue.size(); ++i) {
                if (ready_queue[i].PID < ready_queue[best].PID) {
                    best = (int)i;
                }
            }
            return best;
        };

        if (running.PID != -1 && !ready_queue.empty()) {
            int best_index = best_index_ready();
            if (best_index >= 0 && ready_queue[best_index].PID < running.PID) {
                running.state = READY;
                sync_queue(job_list, running);
                ready_queue.push_back(running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                running = PCB();
                idle_CPU(running);
                quantum_used = 0;
            }
        }

        if (running.PID == -1) {
            int best_index = best_index_ready();
            if (best_index >= 0) {
                running = ready_queue[best_index];
                ready_queue.erase(ready_queue.begin() + best_index);
                if (running.start_time == -1) {
                    running.start_time = current_time;
                }
                running.state = RUNNING;
                quantum_used = 0;
                sync_queue(job_list, running);
                execution_status += print_exec_status(currennt_time, running.PID, READY, RUNNING);
            }
        }

        if (running.PID != -1) {
            if (running.remaining_time > 0) {
                running.remaining_time -= 1;
            }
            quantum_used++;

            static std::vector<std::pair<int, unsigned int> cpu_since_io_local;
            
            auto get_cpu_local = [&](int pid) -> unsigned int {
                for (auto &pr : cpu_since_io_local) {
                    if (pr.first == pid) {
                        return pr.second;
                    }
                }
                return 0;
            };
            auto set_cpu_local = [&](int pid, unsigned int v) {
                for (auto &pr : cpu_since_io_local) {
                    if (pi.first == pid) {
                        pr.second = v;
                        return;
                    }
                }
                cpu_since_io_local.push_back({pid, v});
            };
            auto erase_cpu_local = [&](int pid) {
                for (size_t i = 0; i < cpu_since_io_local.size(); ++i) [
                    if (cpu_since_io_local[i].first == pid) {
                        cpu_since_io_local.erase(cpu_since_io_local.begin() + i);
                        return;
                    }
                ]
            };

            unsigned int cs = get_cpu_local(running.PID);
            cs++;
            set_cpu_local(running.PID, cs);

            if (running.io_freq > 0 && cs >= running.io_freq && running.remaining_time > 0) {
                PCB t = running;
                t.state = WAITING;
                wait_queue.push_back(t);
                execution_status += print_exec_status(current_time + 1, t.PID, RUNNING, WAITING);
                running = PCB();
                idle_CPU(running);
                quantum_used = 0;
                erase_cpu_local(t.PID);
            } else if (running.remaining_time == 0) {
                execution += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                unsigned int pid_cleanup = running.PID;
                running = PCB();
                idle_CPU(running);
                quantum_used = 0;
                erase_cpu_local(running.PID);
            } else if (quantum_used >= QUANTUM) {
                running.state = READY;
                sync_queue(job_list, running);
                ready_queue.push_back(running);
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, READY);
                running = PCB();
                idle_CPU(running);
                quantum_used = 0;
            } else {
                sync_queue(job_list, running);
            }
        }
        current_time++;

        /////////////////////////////////////////////////////////////////

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}