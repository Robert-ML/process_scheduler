Name: Lica Robert-Mihai
Class: 331

#
#  University project - Thread Scheduler
#

Project description
-

I implemented a shared library with a simulated thread scheduler for a single core processor.

Project structure
-
* `Makefile` - linux makefile for building the `libscheduler.so` library
* `scheduler.h` - header file for the scheduler implementation
* `so_scheduler.h` - header file for the library's interface (to be used by the user)
* `so_scheduler.c` - implementation of the scheduler
* `sch_thread.h` - header file for the information about threads hold by the scheduler
* `sch_thread.c` - implementation of the header file
* `pq.h`- header file for the generic priority queue
* `pq.c`- implementation of a generic priority queue
* `debug.h` - header with helpfull debug macros
* `utils.h` - header with error handling macros

Implementation
-

The generic priority queue must receive as parameters function pointers to implementations of the copying and comparing of the provided data type. The time complexity is O(log N) for inserting and pop-ing the top and for peek-ing the top is O(1). The space complexity is O(n).

To sinchronize between threads and for one to signal another that he can start his work, I use semaphores as sincronization devices because unlocking mutexes from other threads than the one that locked it first [results](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html) in undefined behaviour or error. (I had to change the implementation when observed this detail)

The planner is quite efficient as it keeps track of what thread was running before hand and makes the context switch only if needed, thus not needing to insert an element in the priority queue and another one to be taken out all the time.

Miscellaneous
-
* GitHub: [Profile](https://github.com/Robert-ML)
 <!-- | [Repo](https://github.com/Robert-ML/stdio.h_shared_library) -->
* Geeksforgeeks page about the [priority queue](https://www.geeksforgeeks.org/priority-queue-using-binary-heap/) and inspiration for the code
