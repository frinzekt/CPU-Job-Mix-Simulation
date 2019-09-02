#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* CITS2002 Project 1 2019
   Name(s):             Frinze Lapuz, Thye Shan Ng
   Student number(s):   22711649, 22727425
 */

//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c

//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES 4
#define MAX_DEVICE_NAME 20
#define MAX_PROCESSES 50
#define MAX_EVENTS_PER_PROCESS 100

#define TIME_CONTEXT_SWITCH 5
#define TIME_ACQUIRE_BUS 5

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum = 0;
int total_process_completion_time = 0;

/* TraceFile Sample
device    usb2[Device Name]      60000000 [Transfer Rate] bytes/sec
device    kb         10 bytes/sec
device    ssd        240000000 bytes/sec
device    hd         80000000 bytes/sec
device    wifi       6750000 bytes/sec
device    screen     200000 bytes/sec
reboot

process  1  200 [Process Start] {
  i/o      100 [Event Start]     hd      1600 [transfer_size]
  i/o      110     usb2    1600
  i/o      180     hd      1000
  i/o      190     usb2    1000
  exit     400 [Process End]
}
process  2  480 {
  i/o      8000    screen  40
  exit     8005
}
*/
//Parse File Device Details Storage
char *device_names[MAX_DEVICE_NAME];
char *device_check[MAX_DEVICE_NAME];
int transfer_rates[MAX_DEVICE_NAME];
int no_of_devices = 0; // this is also the index of current device being parsed
int pointer_amount = 0;

//Pase File Process Details Storage
int process_start[MAX_PROCESSES];
int process_end[MAX_PROCESSES];
int no_of_process = 0;

//Parse File Events Detail Storage
int event_start[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
char *event_device[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int transfer_size[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int no_of_events[MAX_PROCESSES];
int IO_runtime[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS]; //in usec

//  ----------------------------------------------------------------------

#define CHAR_COMMENT '#'
#define MAXWORD 20
#define HR "\n----------------------------------------------------------------------\n"

//Calculations-----------------------------------------------------------
//CALCULATES THE AMOUNT OF TIME TAKEN (USEC) FOR FILE TRANSFER
/*void runtime(int process_index, int event_index)
{
    int trans_rate;
    int i;
    float total;
    for (i = 0; i< MAX_DEVICE_NAME; i++)
    {
        if (strcmp(device_check[i],event_device[process_index][event_index]) == 0)
        {
            trans_rate = transfer_rates[i];
        }
    }
    printf("%s \n",event_device[process_index][event_index]);
    total = transfer_size[no_of_process][no_of_events[no_of_process]] / trans_rate * pow(10, 6);
    printf("%f \n", total);
} */

//----------------------------------------------------------------------

/*int getDeviceTransferRate (char device_name[]){
    int index = -1;
    //FIND INDEX OF DEVICE NAME

    for(int i = 0; i<no_of_devices; i++){
        //COMPARE device name selected to list
        if(strcm)
    }

    //INDEX -1 -> NOT FOUND DEVICE
    if(index==-1){
        printf("ERROR: DEVICE NOT FOUND IN THE LIST")
    }
    return transfer_rates[index];
}*/
void print_device_details(int index)
{
    printf("Device No: %3d | Device Name: %20s | Transfer Rate: %10d Bytes/Second \n",
           index, device_names[index], transfer_rates[index]);
}
void print_event_details(int process_index, int event_index)
{
    printf("Event Start: %10d | Device Name: %20s | Transfer Size: %10d  \n",
           event_index, event_device[process_index][event_index], transfer_size[process_index][event_index]);
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
            device_names[no_of_devices] = word1;
            transfer_rates[no_of_devices] = atoi(word2);
            device_check[pointer_amount] = word1;
            pointer_amount++;
            print_device_details(no_of_devices);
            
            no_of_devices++;
            // FOUND A DEVICE DEFINITION, WE'LL NEED TO STORE THIS SOMEWHERE
        }

        else if (nwords == 1 && strcmp(word0, "reboot") == 0)
        {
            printf("Total Devices: %d \n", no_of_devices); // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }

        else if (nwords == 4 && strcmp(word0, "process") == 0)
        {
            process_start[no_of_process] = atoi(word2);

            printf(HR);
            printf("Start Process No %2d: %d \n", no_of_process, process_start[no_of_process]); // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
        }

        else if (nwords == 4 && strcmp(word0, "i/o") == 0)
        {
            event_start[no_of_process][no_of_events[no_of_process]] = atoi(word1);
            event_device[no_of_process][no_of_events[no_of_process]] = word2;
            transfer_size[no_of_process][no_of_events[no_of_process]] = atoi(word3);
            int i;
            for (i=0;i<no_of_devices; i++)
            {
                
                print_device_details(i);
            }
            //runtime(no_of_process,no_of_events[no_of_process]);
            //IO_runtime[no_of_process][no_of_events[no_of_process]] = runtime();
            /* 
            TODO : 
                runtime is in usec
                MATH: runtime = (transfer_size/transfer_rate) * 10^6
                runtime = (transfer_size/GetTransferRate(event_device)) * 10^6
                IO_runtime[no_of_process][no_of_events[no_of_process]] = runtime;


            */

            print_event_details(no_of_process, no_of_events[no_of_process]);
            no_of_events[no_of_process]++;
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
        }

        else if (nwords == 2 && strcmp(word0, "exit") == 0)
        {
            process_end[no_of_process] = atoi(word1);
            printf("End: %d \n", process_end[no_of_process]); //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
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

int RunForTQ(int TQ, int process_index)
{

    return 0;
}

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
/*    int process_queue[MAX_PROCESSES];
    int device_io_queue[MAX_EVENTS_PER_PROCESS];
    int system_time = 0;*/

    // TODO Simulations
    printf("running simulate_job_mix( time_quantum = %i usecs )\n",
           time_quantum);
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

//  vim: ts=8 sw=4
