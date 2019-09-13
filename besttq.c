#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* CITS2002 Project 1 2019
   Name(s):             Frinze Lapuz, Thye Shan Ng
   Student number(s):   22711649, 22727425
 */

//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c

//BASIC MATH DEFINITION

#define __max(a, b) (((a) > (b)) ? (a) : (b))
#define __min(a, b) (((a) < (b)) ? (a) : (b))

//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES 4
#define MAX_DEVICE_NAME 20
#define MAX_PROCESSES 50
#define MAX_EVENTS_PER_PROCESS 100
#define MAX_BLOCKED_PROCESSES 100 * 50

#define TIME_CONTEXT_SWITCH 5
#define TIME_ACQUIRE_BUS 5

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum = 0;
int total_process_completion_time = 99999999;

//Parse File Device Details Storage
char device_names[MAX_DEVICES][MAX_DEVICE_NAME];
int transfer_rates[MAX_DEVICES];
int no_of_devices = 0; // this is also the index of current device being parsed
int devicePriority[MAX_DEVICES];

//Pase File Process Details Storage
int process_start[MAX_PROCESSES];
int process_name[MAX_PROCESSES];
int process_end[MAX_PROCESSES];
int process_rank[MAX_PROCESSES];
int no_of_process = 0;

//Parse File Events Detail Storage
int event_start[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
char event_device[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS][MAX_DEVICE_NAME];
int transfer_size[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int no_of_events[MAX_PROCESSES];
int IO_runtime[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS]; //in usec

// Program Flow
bool skip_IO_wait = false; //DIS/ALLOWS IO System Tick By 1 Print
int system_clock = 0;
int nexit = 0;              //Number of exited process
int no_of_RQelements = 0;   //Facilitates count of READY<->RUNNING
int process_entered_RQ = 0; //facilitates NEW->READY
int RQ[MAX_PROCESSES];      //holds the processes in ready queue
int running = -1;           //holds the process in running state

int BQ_Pindex[MAX_BLOCKED_PROCESSES];
int BQ_Eindex[MAX_BLOCKED_PROCESSES];
int BQ_RemainingTime[MAX_BLOCKED_PROCESSES];
int no_of_BQelements = 0;
int IO_entered_BQ[MAX_PROCESSES];

int process_remaining_runtime[MAX_PROCESSES];
int process_running_time[MAX_PROCESSES];

//  ----------------------------------------------------------------------

#define CHAR_COMMENT '#'
#define MAXWORD 20
#define HR "\n----------------------------------------------------------------------\n"

//PRINT TERMINALS
#define PRINT_IN_RED printf("\033[0;31m")
#define PRINT_IN_USUAL printf("\033[0m")

//----------------------------------------------------------------------
void duplicateArrayInt(int destination[], int source[], int size)
{
    //INTEGER ARRAY DUPLICATION

    for (int i = 0; i < size; i++)
    {
        destination[i] = source[i];
    }
}

void program_flow_reset(void)
{
    //RESETS ALL OF THE GLOBAL VARIABLES TO 0, ASIDE FROM THE 'STORAGE' VARIABLES
    //In preparation for next job mix

    //Process Flow Array Resets (Block,Ready, General Processes)
    memset(BQ_Pindex, 0, sizeof(BQ_Pindex));
    memset(BQ_Eindex, 0, sizeof(BQ_Eindex));
    memset(BQ_RemainingTime, 0, sizeof(BQ_RemainingTime));
    memset(IO_entered_BQ, 0, sizeof(IO_entered_BQ));
    memset(process_running_time, 0, sizeof(process_running_time));
    memset(process_remaining_runtime, 0, sizeof(process_remaining_runtime));
    duplicateArrayInt(process_remaining_runtime, process_end, no_of_process);
    memset(RQ, 0, sizeof(RQ));

    //Process Flow Variable
    skip_IO_wait = false;
    system_clock = 0;
    process_entered_RQ = 0;
    nexit = 0;
    no_of_RQelements = 0;
    running = -1;
    no_of_BQelements = 0;
}

int myceil(double x)
{
    //besttq.c:(.text+0x73): undefined reference to `ceil'
    //Alternative to ceil
    int whole = x;
    double decimal = x - whole;
    int add = (decimal > 0) ? 1 : 0;

    return whole + add;
}

int runtime(int process_index, int event_index, int transfer_rate)
{
    //CALCULATES THE AMOUNT OF TIME TAKEN (USEC) FOR FILE TRANSFER
    /*
        Units:
        transfer-size -> bytes
        transfer-size -> bytes/sec
        total-runtime -> usec
    */
    float total = transfer_size[process_index][event_index] * pow(10, 6) / transfer_rate;
    return myceil(total);
}

void sort_process(void)
{
    //RANKS PROCESSES ACCORDINGLY BY THEIR STARTING TIME

    int length = no_of_process;
    int rank = length;
    int max = -1;
    int process_dup[length];
    int temp;
    int process_temp_rank[length];

    //Duplicates process time quantum so that it could be altered
    //Without altering process_start
    for (int i = 0; i < length; i++)
    {
        process_dup[i] = process_start[i];
    }

    //Ranks the process without altering the index
    /* 
        eg. [500, 200, 100, 400] -> [4, 2, 1, 3]
    */
    for (int i = 0; i < length; i++)
    {
        for (int j = 0; j < length; j++)
        {
            if (process_dup[j] > max)
            {
                max = process_dup[j];
                temp = j;
            }
        }

        for (int j = 0; j < length; j++)
        {
            if (process_dup[j] == max)
            {
                process_temp_rank[j] = rank;
                rank--;
            }
        }
        max = -1;
        process_dup[temp] = -1;
    }

    //Orders process name in ascending order via the loop above
    /*
        eg.     Process_name [1,2,3,4]      -> [3,2,4,1]
                Process_temp_rank [4,2,1,3] -> [1,2,3,4]
    */
    int rank_index = 0;
    for (int i = 0; i < length + 1; i++)
    {
        for (int j = 0; j < length; j++)
        {
            if (process_temp_rank[j] == i)
            {
                process_rank[rank_index] = process_name[j];
                rank_index = rank_index + 1;
            }
        }
    }
}

int sort_comp(const void *elem1, const void *elem2)
{
    //QSORT pointer comparator
    //Compares two pointer elements and returns the order adjustment of index assignment

    int n1 = *((int *)elem1);
    int n2 = *((int *)elem2);

    if (n1 > n2)
    {
        return 1;
    }
    if (n1 < n2)
    {
        return -1;
    }
    return 0;
}

void createDevicePriority(void)
{
    /*CREATES RANKING ORDER OF THE DEVICE PRIORITIES BASED ON TRANSFER RATE
    -> Done using comparison of original array to the sorted array to create rank
    By Definition of MAX_DEVICES = 4 -> Array is of length 4
    eg. transferRate = {0,400,300,0} (0 is no device assigned)
        Duplicate = {0,0,300,400} (QSORT COMPARISON SORTER)
        rank = {0,3,2,0} (result)

        Higher Numerical Rank = Higher Device Priority
    */

    int device_rate_dup[MAX_DEVICES];

    //DUPLICATE ARRAY
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        device_rate_dup[i] = transfer_rates[i];
    }
    qsort(device_rate_dup, sizeof(device_rate_dup) / sizeof(int), sizeof(int), sort_comp);

    //CREATE RANK ORDER
    for (int i = 0; i < no_of_devices; i++)
    {
        //LOOP FOR INDIVIDUAL DEVICE RATE
        for (int j = 0; j < MAX_DEVICES; j++)
        {
            //LOOP FOR FINDING THE INDEX IN THE SORTED LIST
            if (transfer_rates[i] == device_rate_dup[j])
            {
                devicePriority[i] = j; //RECORD INDEX OF ORDER
                break;                 //BREAKS OUT OF THE INNER LOOP FOR NEXT RANK
            }
        }
    }
}

int getProcessIndexByName(int name)
{
    //FINDS THE PROCESS INDEX BY PROCESS NAME

    for (int i = 0; i < no_of_process; i++)
    {
        if (name == process_name[i])
        {
            return i;
        }
    }

    printf("\n\n ERROR Process Name: %i not found\n\n", name);
    return -1; //Returns negative index for failure
}

int getDeviceIndex(char device_name[])
{
    //FIND INDEX OF DEVICE NAME

    for (int i = 0; i < no_of_devices; i++)
    {
        //COMPARE device name selected to list
        if (strcmp(device_name, device_names[i]) == 0)
        {
            return i;
        }
    }

    printf("ERROR: DEVICE NOT FOUND\n");
    return -1; //Returns negative index for failure
}

int getDeviceTransferRate(char device_name[])
{
    //GETS THE TRANSFER RATE OF THE DEVICE

    for (int i = 0; i < no_of_devices; i++)
    {
        //COMPARE device name selected to list
        if (strcmp(device_name, device_names[i]) == 0)
        {
            return transfer_rates[i];
        }
    }

    printf("ERROR: DEVICE NOT FOUND\n");
    return -1;
}

void print_device_details(int index)
{
    printf("Device No: %3d | Device Name: %20s | Transfer Rate: %10d Bytes/Second \n",
           index, device_names[index], transfer_rates[index]);
}

void print_event_details(int process_index, int event_index)
{
    printf("Event Start: %10d | Device Name: %20s | Transfer Size: %10d | Runtime IO: %5d usec \n",
           event_index, event_device[process_index][event_index], transfer_size[process_index][event_index],
           IO_runtime[process_index][event_index]);
}

void print_arr_int(char name[], int list[], int size)
{ //Print INT in a easily-readable format
    printf("%s:[", name);
    for (int i = 0; i < size; i++)
    {
        printf("%i ,", list[i]);
    }
    printf("]\n");
}
void print_BQ(void)
{
    //Prints Details about the Blocked Queue

    printf("#BQElement:%i\n", no_of_BQelements);
    print_arr_int("BQ_Pindex", BQ_Pindex, no_of_BQelements);
    print_arr_int("BQ_Eindex", BQ_Eindex, no_of_BQelements);
    print_arr_int("BQ_RemainingTime", BQ_RemainingTime, no_of_BQelements);
    printf("\n\n");
}

void print_RQ(void)
{
    print_arr_int("RQ", RQ, no_of_process);
}

void print_process_execution_order(void)
{
    print_arr_int("Process Execution Order", process_rank, no_of_process);
}

void print_device_priority(void)
{
    print_arr_int("Device Priority", devicePriority, no_of_devices);
}

void parse_tracefile(char program[], char tracefile[])
{
    //  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp = fopen(tracefile, "r");

    if (fp == NULL)
    {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int lc = 0; //LINE Counter

    //  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while (fgets(line, sizeof line, fp) != NULL)
    {
        ++lc;

        //  COMMENT LINES ARE SIMPLY SKIPPED
        if (line[0] == CHAR_COMMENT)
        {
            continue;
        }

        //  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

        //      printf("%i = %s", nwords, line);

        //  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if (nwords <= 0)
        {
            continue;
        }
        //  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENTS
        if (nwords == 4 && strcmp(word0, "device") == 0)
        {

            strcpy(device_names[no_of_devices], word1);
            transfer_rates[no_of_devices] = atoi(word2);
            print_device_details(no_of_devices);

            no_of_devices++;
            // FOUND A DEVICE DEFINITION, WE'LL NEED TO STORE THIS SOMEWHERE
        }

        else if (nwords == 1 && strcmp(word0, "reboot") == 0)
        {
            // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
            printf("Total Devices: %d \n", no_of_devices);
        }

        else if (nwords == 4 && strcmp(word0, "process") == 0)
        {
            process_name[no_of_process] = atoi(word1);
            process_start[no_of_process] = atoi(word2);

            printf(HR);
            printf("Start Process No %2d: %d \n", no_of_process, process_start[no_of_process]);
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
        }

        else if (nwords == 4 && strcmp(word0, "i/o") == 0)
        {
            int process_index_of_event = no_of_events[no_of_process];

            event_start[no_of_process][process_index_of_event] = atoi(word1);
            strcpy(event_device[no_of_process][process_index_of_event], word2);
            transfer_size[no_of_process][process_index_of_event] = atoi(word3);

            int transfer_rate;
            transfer_rate = getDeviceTransferRate(event_device[no_of_process][process_index_of_event]);

            IO_runtime[no_of_process][process_index_of_event] =
                runtime(no_of_process, process_index_of_event, transfer_rate);

            print_event_details(no_of_process, no_of_events[no_of_process]);
            no_of_events[no_of_process]++;
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
        }

        else if (nwords == 2 && strcmp(word0, "exit") == 0)
        {
            process_end[no_of_process] = atoi(word1);
            process_remaining_runtime[no_of_process] = process_end[no_of_process];
            //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
            printf("End: %d \n", process_end[no_of_process]);
        }

        else if (nwords == 1 && strcmp(word0, "}") == 0)
        {
            no_of_process++;
            //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
        }
        else
        {
            printf("%s: line %i of '%s' is unrecognized",
                   program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
}

#undef MAXWORD
#undef CHAR_COMMENT

//  ----------------------------------------------------------------------

int popArrayInt(int arr[])
{
    //RETURNS THE FIRST ELEMENT AND SHIFTS ALL OTHER ELEMENT ONE MOVE TO THE LEFT
    //Assumption: Mid-element cannot be equal to 0

    int popped = arr[0];

    int i = 0;

    //Moves element until encounter 0 to the left
    while (arr[i] != 0)
    {
        arr[i] = arr[i + 1];
        i++;
    }
    return popped;
}

int firstZeroRQ(void)
{
    //FINDS THE INDEX OF THE FIRST ZERO IN RQ

    int i = 0;
    while (RQ[i] != 0)
    {
        i++;
    }
    return i;
}

void delBQFirst(int time)
{
    //DELETES THE FIRST ELEMENT AND SHIFTS ALL OTHER ELEMENT ONE MOVE TO THE LEFT
    //Additional Note: If there is a change in the bus owner, add TIME_ACQUIRE_BUS

    int i = 0;
    int name = process_name[BQ_Pindex[0] - 1];

    printf("SC: %10d  nexit=%3d Process P%i-%i IO FINISHED\n-----------\n", system_clock + time, nexit, name, BQ_Eindex[0]);
    printf("SC: %10d  nexit=%3d Process P%i BLOCKED->READY (IO)\n", system_clock, nexit, name);
    RQ[firstZeroRQ()] = name;

    //SHIFT ELEMENTS TO THE LEFT
    while (BQ_Pindex[i] != 0)
    {
        BQ_Pindex[i] = BQ_Pindex[i + 1];
        BQ_Eindex[i] = BQ_Eindex[i + 1];
        BQ_RemainingTime[i] = BQ_RemainingTime[i + 1];
        i++;
    }

    if (BQ_Pindex[0] != 0) // Happens only if deletion still has other elements
    {
        //CHANGE OF BUS OWNER
        BQ_RemainingTime[0] += TIME_ACQUIRE_BUS;
    }
    no_of_BQelements--;
}

void SubtractTimeInBQ(int time)
{
    /*SUBTRACTS TIME FROM THE FIRST ELEMENT
    Additional Notes: If First < Time subtraction, carryover subtraction to next element
    */
    if (time >= 0)
    {
        do
        {
            if (no_of_BQelements != 0) //SUBTRACT ONLY IF THERE IS A BQ ELEMENT
            {
                if (BQ_RemainingTime[0] > time)
                {
                    BQ_RemainingTime[0] -= time;
                    time = 0;
                }
                else if (BQ_RemainingTime[0] == time)
                { //First element is UP, delete it
                    delBQFirst(time);
                    time = 0;
                }
                else
                {
                    // TIME SUBTRACTION CARRY OVER
                    //CANNOT CARRYOVER IF NO_OF_BQELEMENTS = 0
                    time -= BQ_RemainingTime[0];
                    delBQFirst(BQ_RemainingTime[0]);
                }
            }
            else //No BQ element
            {
                time = 0;
            }

        } while (time != 0); //Ends loop at completion of time subtraction and carryovers
    }
    else
    {
        /*negative time is used to adjust IO remaining time
        Note: IO is blocked in function seekNextIO()
        System Clock tick happens at the time consumed including the time before IO is found
        Hence the time before IO is found does not count in the blocked queue for the IO that is
        recently inserted, but counts for all other IO
        */

        printf("-----TIME ADJUSTER DETECTED %i---\n", time);
        BQ_RemainingTime[0] -= time;
        print_BQ();
        printf("------------------------------\n");
    }
}

void system_clock_tick(int time)
{
    // Simulates system clock tick by certain amount
    // Additional Note: Allows simultaneous IO and CPU time processing
    system_clock += time;
    SubtractTimeInBQ(time);
}

void seekNextProcess(void)
{
    //FINDS THE NEXT PROCESS TO BE PUT IN READY QUEUE (SKIPPING METHOD)

    int deviation = process_start[getProcessIndexByName(process_rank[process_entered_RQ])] - system_clock;
    system_clock_tick(deviation);
    RQ[0] = process_rank[process_entered_RQ];
    printf("SC: %10d  nexit=%3d Process P%i NEW->READY (SKIP)\n", system_clock, nexit, RQ[0]);

    process_entered_RQ++;
    no_of_RQelements++;
}

void seekNextReadyProcess(int start, int expectedTimeConsume)
{
    //FIND PROCESSES TO GO TO RQ IN STARTING AND ENDING TIME (time interval)
    //process_start[getProcessIndexByName(process_rank[process_entered_RQ])]
    //-> Finds the start process of the next process in the rank(by name)
    //-> The rank will yield the name of the process

    if (process_entered_RQ < no_of_process) //INDEX CHECKER
    {
        int process_index = getProcessIndexByName(process_rank[process_entered_RQ]);

        while ((start + expectedTimeConsume >= process_start[process_index]) && (process_entered_RQ < no_of_process))
        { // Repeats for all processes that has not entered RQ that is within the time interval
            process_index = getProcessIndexByName(process_rank[process_entered_RQ]);

            printf("SC: %10d  nexit=%3d Process P%i NEW->READY\n", process_start[process_index],
                   nexit, process_rank[process_entered_RQ]);

            RQ[firstZeroRQ()] = process_rank[process_entered_RQ]; //INSERTS TO FIRST ZERO ELEMENT
            process_entered_RQ++;
            no_of_RQelements++;
        }
    }
}

int getPriorityDevice(int device_index)
{
    return devicePriority[device_index];
}

void insertToBQ(int position, int process_index, int event_index, int remaining_time)
{
    //Insert Values to Blocked BQ on specific position
    for (int i = no_of_BQelements - 1; i >= position; i--)
    {
        //MOVE ITEMS TO THE RIGHT
        BQ_Pindex[i + 1] = BQ_Pindex[i];
        BQ_Eindex[i + 1] = BQ_Eindex[i];
        BQ_RemainingTime[i + 1] = BQ_RemainingTime[i];
    }

    //INSERT TO SPECIFIC INDEX
    BQ_Pindex[position] = process_index + 1; //DIFFERENTIATE index 0 and invalid index
    BQ_Eindex[position] = event_index;
    BQ_RemainingTime[position] = remaining_time;
}

void insertIOtoBQ(int process_index, int event_index)
{
    //FINDS THE POSITION AT WHICH IO SHOULD BE PLACED IN BQ BY PRIORITY ORDER

    int runtime = IO_runtime[process_index][event_index];
    int InsertDeviceIndex = getDeviceIndex(event_device[process_index][event_index]);
    int InsertDevicePriority = getPriorityDevice(InsertDeviceIndex);

    for (int i = 0; i <= no_of_BQelements; i++)
    {

        if (BQ_Pindex[i] != 0) // VALID INDEX
        {
            int iDeviceIndex = getDeviceIndex(event_device[BQ_Pindex[i] - 1][BQ_Eindex[i]]); //DEVICE INDEX OF i'th element
            int iDevicePriority = getPriorityDevice(iDeviceIndex);
            if (InsertDevicePriority > iDevicePriority) //HIGHER DEVICE PRIORITY
            {
                insertToBQ(i, process_index, event_index, runtime);
                break;
            }
            else
            { // LOWER DEVICE PRIORITY BY RANK AND TIME -> FIND WHERE IT IS OF HIGHER PRIORITY
                continue;
            }
        }
        else
        { //FOUND A 0
            insertToBQ(i, process_index, event_index, runtime);
            break;
        }
    }

    no_of_BQelements++;
    print_BQ();
}

int seekNextIO(int start, int expectedTimeConsume, int process_index)
{
    //Finds IO in expected time range of process
    //returns Time Consumed Before IO STARTS
    //returns -1 if there is no IO

    int event_count = no_of_events[process_index];
    int IO_start = event_start[process_index][IO_entered_BQ[process_index]];
    //IF the Expected Ending Time is more than the IO Event Starting (then it will start)

    if ((start + expectedTimeConsume >= IO_start) && (IO_entered_BQ[process_index] < event_count))
    {
        int expected_system_clock = system_clock + IO_start - start;
        insertIOtoBQ(process_index, IO_entered_BQ[process_index]);

        printf("SC: %10d  nexit=%3d Process P%i-%i REQUEST DATA BUS\n", expected_system_clock, nexit, running,
               IO_entered_BQ[process_index]);

        print_BQ();
        IO_entered_BQ[process_index]++; //NEXT IO TO BE FOUND
        return IO_start - start;
    }
    else
    { // NO IO to start
        return -1;
    }
}

int RunProcessForTQ(int TQ, int process_name)
{
    //RUN PROCESS UNTIL PROCESS IS EXIT/EXPIRE/BLOCKED
    running = process_name;
    int process_index = getProcessIndexByName(process_name);
    int time_adjuster = 0;

    int timeConsumed = 0;
    bool DidExit = false;
    bool DidBlocked = false;

    //Returns the time consumed (EXPECTED - W/O IO) to run process
    if (process_remaining_runtime[process_index] <= TQ)
    {
        //Process Will Finished/Exited
        DidExit = true; //IF BLOCKED, doesn't matter
        timeConsumed = process_remaining_runtime[process_index];
    }
    else
    {
        timeConsumed = TQ;
    }

    //Finds NextIO To run if any
    int timeBeforeIOStarts = seekNextIO(process_running_time[process_index], timeConsumed, process_index);

    if (timeBeforeIOStarts != -1) // FOUND
    {
        // THERE IS AN IO TO BE RUN
        timeConsumed = timeBeforeIOStarts;
        DidBlocked = true;

        if (no_of_BQelements == 1)
        { // FIRST ELEMENT NEEDS TO ADJUST TIME
            time_adjuster = timeConsumed + TIME_ACQUIRE_BUS;
        }
    }

    //Finds Processes that can be put in RQ Before Process Ends/Expire/Blocked
    seekNextReadyProcess(system_clock, timeConsumed);

    //Exiting/Blocked/Readying Process
    if (DidBlocked)
    { //Record Process Index and Event Index in seekNextIO
        printf("SC: %10d  nexit=%3d Process P%i RUNNING->BLOCKED\n-----------\n", system_clock + timeConsumed, nexit, running);
    }
    else if (DidExit)
    {
        //EXITED PROCESS COUNTER
        nexit++;
        printf("SC: %10d  nexit=%3d Process P%i RUNNING -> EXIT\n-----------\n", system_clock + timeConsumed, nexit, running);
        running = 0; //INDICATOR THAT PROCESS EXITED
        print_BQ();
    }
    else
    {
        //Process Not Exiting will go back To Queue
        printf("SC: %10d  nexit=%3d Process P%i RUNNING -> READY\n-----------\n", system_clock + timeConsumed, nexit, running);
        no_of_RQelements++;
        RQ[firstZeroRQ()] = running;
    }
    SubtractTimeInBQ(-time_adjuster);
    process_running_time[process_index] += timeConsumed;
    process_remaining_runtime[process_index] -= timeConsumed;
    return timeConsumed;
}

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
    program_flow_reset();
    printf(HR);
    printf("running simulate_job_mix( time_quantum = %i usecs )\n",
           time_quantum);

    //First Starting Process
    //[System Clock] = [First Starting Process]
    seekNextProcess();
    int starting_time = system_clock;
    running = -1;

    //RUN JOBMIX UNTIL NO OTHER PROCESS IS REMAINING TO BE EXECUTED
    while (nexit < no_of_process)
    {
        if ((running != RQ[0]) && ((RQ[0] != 0)))
        {
            //SWITCH PROCESS BECAUSE SOMEONE IS WAITING
            seekNextReadyProcess(system_clock, TIME_CONTEXT_SWITCH); //SEEK Process that NEW->READY during TIME_CONTEXT_SWITCH
            system_clock_tick(TIME_CONTEXT_SWITCH);
            printf("SC: %10d  nexit=%3d Process P%i READY->RUNNING\n", system_clock, nexit, RQ[0]);
            no_of_RQelements--;
            system_clock_tick(RunProcessForTQ(time_quantum, popArrayInt(RQ)));
        }
        else if (RQ[0] == 0)
        {
            // NO PROCESS IN READY QUEUE FIND NEXT ONE
            if (!skip_IO_wait)
            {
                printf("SC: %10d  nexit=%3d NO OTHER PROCESS READY TO RUN\n", system_clock, nexit);
                print_BQ();
                print_RQ();
            }

            //SEEK NEXT PROCESS TO RUN
            if (process_entered_RQ < no_of_process)
            {
                seekNextProcess();
                system_clock_tick(TIME_CONTEXT_SWITCH);
                printf("SC: %10d  nexit=%3d Process P%i READY->RUNNING\n", system_clock, nexit, RQ[0]);
                no_of_RQelements--;
                system_clock_tick(RunProcessForTQ(time_quantum, popArrayInt(RQ)));
            }
            else
            { // ALL PROCESS IS BLOCKED -> NEED TO SKIP TIME
                running = 0;
                skip_IO_wait = true;
                system_clock_tick(1);
            }
        }
        else
        { // NO OTHER PROCESS IS RUNNING so keep running
            printf("SC: %10d  nexit=%3d Process P%i CONTINUE RUNNING\n", system_clock, nexit, running);
            system_clock_tick(RunProcessForTQ(time_quantum, popArrayInt(RQ)));
        }
    }

    int process_completion = system_clock - starting_time;
    print_RQ();
    print_BQ();
    printf("SC: %10d END -> Total Process Completion Time: %i\n\n", system_clock, process_completion);

    //DETERMINING IF THIS IS THE BEST TQ
    total_process_completion_time = __min(total_process_completion_time, process_completion);
    if (total_process_completion_time == process_completion)
    {
        optimal_time_quantum = time_quantum;
    }
}

//  ----------------------------------------------------------------------

void usage(char program[])
{
    printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    int TQ0 = 0, TQfinal = 0, TQinc = 0;

    //  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND THREE TIME VALUES
    if (argcount == 5)
    {
        TQ0 = atoi(argvalue[2]);
        TQfinal = atoi(argvalue[3]);
        TQinc = atoi(argvalue[4]);

        if (TQ0 < 1 || TQfinal < TQ0 || TQinc < 1)
        {
            usage(argvalue[0]);
        }
    }
    //  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND ONE TIME VALUE
    else if (argcount == 3)
    {
        TQ0 = atoi(argvalue[2]);
        if (TQ0 < 1)
        {
            usage(argvalue[0]);
        }
        TQfinal = TQ0;
        TQinc = 1;
    }
    //  CALLED INCORRECTLY, REPORT THE ERROR AND TERMINATE
    else
    {
        usage(argvalue[0]);
    }

    //  READ THE JOB-MIX FROM THE TRACEFILE, STORING INFORMATION IN DATA-STRUCTURES
    parse_tracefile(argvalue[0], argvalue[1]);

    //THE ORDER OF PROCESS IS DETERMINED BY THE STARTING TIME OF PROCESS
    sort_process();

    //Create Device Priority Array based on Transfer Rate
    createDevicePriority();

    print_process_execution_order();
    print_device_priority();

    //  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
    //  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
    //  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED
    for (int time_quantum = TQ0; time_quantum <= TQfinal; time_quantum += TQinc)
    {
        simulate_job_mix(time_quantum);
    }

    //  PRINT THE PROGRAM'S RESULT
    printf("best %i %i\n", optimal_time_quantum, total_process_completion_time);

    exit(EXIT_SUCCESS);
}
