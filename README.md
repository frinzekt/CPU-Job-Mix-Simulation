# CITS2002 (Systems Programmng) - Project 1 (CPU Job Mix Simulation)
 
## Task
Given a tracefile (history of devices processses), find the optimum time quantum (time limit at which processes are alternated) to be set in each process in order to achieve concurrency and the minimum amount of time to complete all the IO operations and events of each processes using the five state model.

### Five State Model
![alt text](https://fennypotter.files.wordpress.com/2010/10/process.jpeg)

## Approach

### Pre-simulation
1. Determine device priorities
2. Initialize/Reset variables for each and every simulation

### Process Alternation
1. Reading the file and recording data structures
2. Arranging the initial sequence of the processes to be put from `NOT READY-> READY`
3. Feed the pool when processes becomes ready `NOT READY -> READY` via the `RQ` (ready queue)
4. Do a state change from `READY -> RUNNING`
5. Switch Process `RUNNING -> READY` if time quantum expires or process finishes (`RUNNING -> EXIT`)

### IO Handling
1. During the process `RUNNING`, when an IO is encountered switch from `RUNNING -> BLOCKED` via the `BQ` (blocked queue)
2. Insertion to blocked queue is evaluated using the predetermined device priority and wait time wherein IO operations are put infront of the lesser prioritized IO operations.
3. Tick the IO operation of the process taking the data bus. If the IO operation finishes, shift the next IO using a data bus context switch. The IO operation that finishes will also use a CPU context switch of `BLOCKED -> READY`


## Evaluation and Improvement
1. Implementation of bundling of data - data structures
2. Modularization - division to multiple c files with headers
3. Use of `#define` more often
4. Git Workflow for better communication, enumeration and prioritization of task such as issues and pull request
