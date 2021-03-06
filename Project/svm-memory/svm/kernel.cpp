#include "kernel.h"

#include <iostream>
#include <algorithm>

namespace svm
{
    Kernel::Kernel(
                Scheduler scheduler,
                std::vector<Memory::ram_type> executables_paths
            )
        : board(),
          processes(),
          priorities(),
          scheduler(scheduler),
          _last_issued_process_id(0),
          _last_ram_position(0),
          _cycles_passed_after_preemption(0),
          _current_process_index(0)
    {

        // Memory Management

        /*
         *   TODO:
         *
         *     Initialize data structures for methods `AllocateMemory` and
         *       `FreeMemory`
         */
	_free_memory_index = 0;    

	board.memory.ram[0] = _free_memory_index;
	board.memory.ram[1] = board.memory.ram.size()-2;



        // Process page faults (find empty frames)
        board.pic.isr_4 = [&]() {
            std::cout << "Kernel: page fault." << std::endl;
		
	    Memory::page_entry_type page = board.cpu.registers.a;
	    Memory::page_entry_type frame = board.memory.AcquireFrame();

	    if(frame != Memory::INVALID_PAGE) {
	    	(*(board.memory.page_table))[page] = frame;
	    } else {
	    	board.Stop();
	    }

            /*
             *  TODO:
             *
             *    Get the faulting page index from the register 'a'
             *
             *    Try to acquire a new frame from the MMU by calling
             *      `AcquireFrame`
             *    Check if the frame is valid
             *        If valid
             *            Write the frame to the current faulting page in the
             *              MMU page table (at index from register 'a')
             *            or else if invalid
             *              Notify the process or stop the board (out of
             *              physical memory)
             */
        };

        // Process Management

        std::for_each(
            executables_paths.begin(),
            executables_paths.end(),
            [&](Memory::ram_type &executable) {
                CreateProcess(executable);
            }
        );

        /*
         *  TODO:
         *
         *    Switch to the first process on the CPU
         *    Switch the page table in the MMU to the table of the current
         *      process
         *    Set a proper state for the first process
         */

        /*
         *  TODO:
         *
         *    Each scheduler should get the page table switching step
         *    Each exit call handler in `isr_3` should start using `FreeMemory`
         *      to release data in RAM or virtual memory
         */
	
	if(!processes.empty() && scheduler != Priority ) {
	
		std::cout << "The process: " << _current_process_index <<", has been set for execution." << std::endl;

		board.cpu.registers = processes[_current_process_index].registers;
		board.memory.page_table = processes[_current_process_index].page_table;

		processes[_current_process_index].state = Process::States::Running;
	
	} else if (!priorities.empty() && scheduler == Priority) {
	
		std::cout << "The process: " << priorities[_current_process_index].id << ", has been set for execution." << std::endl;

		board.cpu.registers = priorities[_current_process_index].registers;
		board.memory.page_table = priorities[_current_process_index].page_table;

		priorities[_current_process_index].state = Process::States::Running;
	
	}


        if (scheduler == FirstComeFirstServed) {
            board.pic.isr_0 = [&]() {
                // ToDo: Process the timer interrupt for the FCFS
		std::cout << "Skipping the timer interrupt handler for FCFS" << std::endl;
		return;
            };

            board.pic.isr_3 = [&]() {
                // ToDo: Process the first software interrupt for the FCFS
                std::cout << "The software interrupt handler for FCFS was called" << std::endl;
				std::cout << "Number of Processes left = " 
				<< processes.size() << std::endl;
				
				std::cout << "Exit call on the current process. Unload the current process. Process id = " 
				<< processes[_current_process_index].id
				<< std::endl;
				
				std::cout << "FreeMemory was called for the current process." << std:: endl;
				FreeMemory(processes[_current_process_index].memory_start_position);
				processes.erase(processes.begin() + _current_process_index);
				std::cout << std::endl;

				// Unload the current process
				if (processes.empty()) {
					std::cout << "There are no processes to perform. Board has been stopped." << std::endl;
					board.Stop();					
				}
				else {
					board.cpu.registers = processes[_current_process_index].registers;
					board.memory.page_table = processes[_current_process_index].page_table;
					processes[_current_process_index].state = Process::States::Running;
					std::cout << "Process State: Running" << std::endl;
					std::cout << std::endl;
				}
            };
        } else if (scheduler == ShortestJob) {
            board.pic.isr_0 = [&]() {
                // ToDo: Process the timer interrupt for the Shortest
                std::cout << "Skipping the timer interrupt handler for SJF" << std::endl;
		return;
				//  Job scheduler	
            };

            board.pic.isr_3 = [&]() {
                // ToDo: Process the first software interrupt for the Shortest
				std::cout << "The software interrupt handler for SJF was called" << std::endl;
				std::cout << "Number of Processes left = " 
				<< processes.size() << std::endl;
			

				std::cout <<"Process id: " << processes[_current_process_index].id << ", with time = "	
					  <<processes[_current_process_index].sequential_instruction_count << std::endl;	


				std::cout << "FreeMemory was called for the current process." << std:: endl;
				FreeMemory(processes[_current_process_index].memory_start_position);
				std::cout << "Exit call on the current process. Unload the current process." << std::endl;
				processes.erase(processes.begin() + _current_process_index);
				
				
				if (processes.empty()) {
					std::cout << "There are no processes to perform. Board has been stopped." << std::endl;
					board.Stop();					
				}
                		//  Job scheduler

               	 		// Unload the current process
				else {
					board.cpu.registers = processes[_current_process_index].registers;
					board.memory.page_table = processes[_current_process_index].page_table;
					processes[_current_process_index].state = Process::States::Running;
					std::cout << "Process State: Running" << std::endl;
					std::cout << std::endl;
				}
            };
        } else if (scheduler == RoundRobin) {
            board.pic.isr_0 = [&]() {
                // ToDo: Process the timer interrupt for the Round Robin
                //  scheduler
				std::cout << "The timer interrupt handler for RR was called" << std::endl;
				
				++_cycles_passed_after_preemption;		
				std::cout << "The current cycle count " << _cycles_passed_after_preemption << std::endl;
				std::cout << "Allowing the current process with index: " << _current_process_index << ", to run"
				<< std::endl
				<< std::endl;

				if(_cycles_passed_after_preemption > _MAX_CYCLES_BEFORE_PREEMPTION){
					std::cout << "Max number of Cycles has been reached" << std::endl;
					process_list_type::size_type next_process_index = 
					(_current_process_index + 1)% processes.size();
					
					std::cout << "The current process with index: " << _current_process_index 
							  << ", has consumed it time slice: " << _MAX_CYCLES_BEFORE_PREEMPTION
							  << std::endl
							  << "Switching the context to the next process with index: " << next_process_index
							  << std::endl;
					//switch the context
					
					std::cout 
					<< "Saving all the registers from the CPU to the PCB of the previous process" 
					<< std::endl;
					
					processes[_current_process_index].registers = board.cpu.registers;
					processes[_current_process_index].state = Process::States::Ready;
					
					std::cout 
					<< "Restoring all the registers from the PCB of the next process to the CPU" 
					<< std::endl;
					
					_current_process_index = next_process_index;
					std::cout << "Current Process Index has been incremented" << std::endl;
					
					board.cpu.registers = processes[_current_process_index].registers;
					board.memory.page_table = processes[_current_process_index].page_table;
					processes[_current_process_index].state = Process::States::Running;
					std::cout << "Process State: Running" << std::endl;
					
					std::cout << "Restoring the time slice counter" << std::endl;
					_cycles_passed_after_preemption = 0;
					std::cout << std::endl;
				}
            };

            board.pic.isr_3 = [&]() {
                // ToDo: Process the first software interrupt for the
				std::cout << "The software interrupt handler for RR was called" << std::endl;
				std::cout << "Number of Processes left = " 
				<< processes.size() << std::endl;
				
				// exit() and Unload the current process
				std::cout << "Exit call on the current process. Unload the current process and context switch." << std::endl;
				std::cout << "FreeMemory was called for the current process." << std:: endl;
				FreeMemory(processes[_current_process_index].memory_start_position);
				processes.erase(processes.begin() + _current_process_index);

				if (processes.empty()) {
					std::cout << "There are no processes to perform. Board has been stopped." << std::endl;
					board.Stop();					
				}
				else {
					if (_current_process_index >= processes.size()) {
						_current_process_index = 0;
					}
					board.cpu.registers = processes[_current_process_index].registers;
					board.memory.page_table = processes[_current_process_index].page_table;
					processes[_current_process_index].state = Process::States::Running;
					std::cout << "Process State: Running" << std::endl;
					std::cout << std::endl;
				}
				
            };
        } else if (scheduler == Priority) {
            board.pic.isr_0 = [&]() {
                // ToDo: Process the timer interrupt for the Priority Queue, using priorities data structure
				std::cout << "The timer interrupt handler for Priority Queue was called" << std::endl;				
			
				// using 1st method for Priority scheduling of decreasing the priority of current process at each clock tick
				// until its' priority gets below the priority of the next process, then context switch occurs 
				//  scheduler

				for (unsigned int i = 1; i < priorities.size(); ++i){
					++priorities[i].priority;
				}


				process_list_type::size_type next_process_index = 
				(_current_process_index + 1)% priorities.size();
				//switch the context

				if (priorities[_current_process_index].priority < priorities[next_process_index].priority) {
					
					std::cout 
					<< "The priority of the current process with id: " << priorities[_current_process_index].id 
					<< ", with priority: "
					<< priorities[_current_process_index].priority
					<< ", has dropped below that of the next highest process with id: " << priorities[next_process_index].id
					<< ", with priority: "
					<< priorities[next_process_index].priority
					<< std::endl
					<< "Switching the context to the next process"
					<< std::endl;
				
					std::cout 
					<< "Saving all the registers from the CPU to the PCB of the previous process" 
					<< std::endl;
					
					priorities[_current_process_index].registers = board.cpu.registers;
					priorities[_current_process_index].state = Process::States::Ready;
					
					std::cout 
					<< "Restoring all the registers from the PCB of the next process to the CPU" 
					<< std::endl;
					// find a way to sort 
					//sort(priorities.begin(), priorities.end());
					//
					//
					for(unsigned int i = 1; i < priorities.size(); ++i){
						int j = i - 1;
						Process _temp = priorities[i];
						while(j >=0 && priorities[j].priority < _temp.priority){
							priorities[j+1] = priorities[j];
							j--;
						}	
						priorities[j+1] = _temp;
					}	
					std::cout << "Current Process Index was changed based on new priorities" << std::endl;
					
					board.cpu.registers = priorities[_current_process_index].registers;
					board.memory.page_table = priorities[_current_process_index].page_table;
					priorities[_current_process_index].state = Process::States::Running;
					std::cout << "Process State: Running" << std::endl;
					
					_cycles_passed_after_preemption = 0;
					std::cout << std::endl;

				}
			
				
            };

            board.pic.isr_3 = [&]() {
        		        // ToDo: Process the first software interrupt for the Priority
				std::cout << "The software interrupt handler for Priority Queue was called" << std::endl;
               			//  Queue scheduler
				std::cout << "Number of Processes left = " 
				<< priorities.size() << std::endl;
				
				// exit() and Unload the current process
				std::cout << "Exit call on the current process: " << priorities[_current_process_index].id
				<< ", with priority: "
				<< priorities[_current_process_index].priority
				<< ". Unload the current process and context switch." 
				<< std::endl;
				std::cout << "FreeMemory was called for the current process." << std:: endl;
				FreeMemory(priorities.front().memory_start_position);
				priorities.pop_front();

				if (priorities.empty()) {
					std::cout << "There are no processes to perform. Board has been stopped." << std::endl;
					board.Stop();					
				}
				else {
					if (_current_process_index >= priorities.size()) {
						_current_process_index = 0;
					}
					board.cpu.registers = priorities[_current_process_index].registers;
					board.memory.page_table = priorities[_current_process_index].page_table;
					priorities[_current_process_index].state = Process::States::Running;
					std::cout << "Process State: Running" << std::endl;
					std::cout << std::endl;
				}
            };
        }


        board.Start();
    }

    Kernel::~Kernel() { }

    void Kernel::CreateProcess(Memory::ram_type &executable)
    {
        Memory::ram_size_type
            new_memory_position = AllocateMemory(executable.size()); // TODO:
                                      //   allocate memory for the process
                                      //   with `AllocateMemory`
        if (new_memory_position == -1) {
            std::cerr << "Kernel: failed to allocate memory."
                      << std::endl;
        } else {
            // Assume that the executable image size can not be greater than
            //   a page size
            std::copy(
                executable.begin(),
                executable.end(),
                board.memory.ram.begin() + new_memory_position
            );

            Process process(
                _last_issued_process_id++,
                new_memory_position,
                new_memory_position + executable.size()
            );

            // Old sequential allocation
            //
            //   std::copy(
            //       executable.begin(),
            //       executable.end(),
            //       board.memory.ram.begin() + _last_ram_position
            //   );
            //
            //   Process process(
            //       _last_issued_process_id++,
            //       _last_ram_position,
            //       _last_ram_position + executable.size()
            //   );
            //
            //   _last_ram_position +=
            //       executable.size();

            // ToDo: add the new process to an appropriate data structure
            //generate random priority level for process
	srand (time(NULL));
	process.priority = rand() % (processes.size() + 1) + 1;

	// ToDo: add the new process to an appropriate data structure
	
	if (scheduler == Priority) {			
		priorities.push_back(process);
		std::sort (priorities.begin(), priorities.end());
	}
	else {
		processes.push_back(process);
	}
	// sorting processes by time, using insertion sort for simplicity		
	if (scheduler == ShortestJob) {			
		for(int i = 1; i < processes.size(); ++i){
			int j = i - 1;
			Process _temp = processes[i];
			while(j >=0 && processes[j].sequential_instruction_count > _temp.sequential_instruction_count){
				processes[j+1] = processes[j];
				j--;
			}	
			processes[j+1] = _temp;
		}	
	}	

            // ToDo: process the data structure (e.g., sort if necessary)

            // ---
        }
    }

    Memory::ram_size_type Kernel::AllocateMemory(
                                      Memory::ram_size_type units
                                  )
    {
        /*
         *  TODO:
         *
         *    Task 1: allocate physical memory by using a free list with the
         *      next fit approach.
         *
         *    You can adapt the algorithm from the book The C Programming
         *      Language (Second Edition) by Brian W. Kernighan and Dennis M.
         *      Ritchie (8.7 Example - A Storage Allocator).
         *
         *    Task 2: adapt the algorithm to work with your virtual memory
         *      subsystem.
         */
	Memory::ram_size_type p, prevp = _free_memory_index;

	for (p = board.memory.ram[prevp]; ; prevp = p, p = board.memory.ram[p]) {
	
		if(board.memory.ram[p+1] >= units){ //size >= units
			if(board.memory.ram[p+1] == units) {//size == units
				board.memory.ram[prevp] = board.memory.ram[p];
			}
			else {
				board.memory.ram[p + 1] -= units + 2;
				p += board.memory.ram[p + 1]; 
				board.memory.ram[p+1] = units;
			}
			
			_free_memory_index = prevp;
			return  p+2;  
		}
		if(p == _free_memory_index) return -1;
	}
        return -1;
    }

    void Kernel::FreeMemory(
                     Memory::ram_size_type physical_address
                 )
    {
        /*
         *  TODO:
         *
         *    Task 1: free physical memory
         *
         *    You can adapt the algorithm from the book The C Programming
         *      Language (Second Edition) by Brian W. Kernighan and Dennis M.
         *      Ritchie (8.7 Example - A Storage Allocator).
         *
         *    Task 2: adapt the algorithm to work with your virtual memory
         *      subsystem
         */
	Memory::ram_size_type p = _free_memory_index;
	Memory::ram_size_type bp = physical_address - 2; 
	
	
	for(; !(bp > p && bp < board.memory.ram[p]); p = board.memory.ram[p])
		if(p >= board.memory.ram[p] && (bp > p || bp < board.memory.ram[p]))
			break;
	if(bp + board.memory.ram[bp+1] == board.memory.ram[p]) {
		board.memory.ram[bp+1] += board.memory.ram[board.memory.ram[p+1]];
		board.memory.ram[bp] = board.memory.ram[board.memory.ram[p]];
	} else
		board.memory.ram[bp] = board.memory.ram[p];
	if(p + board.memory.ram[p+1] == bp) {
		board.memory.ram[p+1] += board.memory.ram[bp+1];
		board.memory.ram[p] = board.memory.ram[bp];
	} else 
		board.memory.ram[p] = bp;
	
	_free_memory_index = p;

    }
}
