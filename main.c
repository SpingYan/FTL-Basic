#include <stdio.h>
#include <stdlib.h>
//#include <pthread.h>
#include <time.h>
#include "ftl.h"

/* B58R TLC
#define CE_NUM 8
#define PLANE_NUM 6
#define BLOCK_NUM 567
#define PAGE_NUM 2784
#define PAGE_SIZE 18352 //Bytes
*/

/*
Flash Geomerty B17A
2CE 474 Block 2304 Page 4 Plane
#define CE_NUM 2
#define PLANE_NUM 4
#define BLOCK_NUM 466
#define PAGE_NUM 2304
*/

// Sample for Simulation
// #define CE_NUM 2
// #define PLANE_NUM 2
// #define BLOCK_NUM 1980
// #define PAGE_NUM 1152
// #define PAGE_SIZE 16384 //Bytes
// // VB size = block
// #define VB_SIZE

/*
    Create thread for write test flow
    0. table init -> pre-condition -> Y: Seq write 100%
    1. Thread 1: "Write" LBA, length -> prepare cmd -> excute cmd.
        -> trim flow and write flow
        -> Erase Count +1 -> Y: Gen Table Log
    2. Get Valid Count Table
       Get Erase Count Table
       program page -> erase block
*/

#define NUM_WRITES 10000

int main(int argc, char **argv)
{       
    // Initial FTL and related flow.
    initializeFTL();
    srand(time(NULL));

    // pthread_t write_thread;
    // pthread_t status_thread;

    unsigned int writeLBARange = TOTAL_BLOCKS * PAGES_PER_BLOCK * OP_SIZE;

    for (unsigned int i = 0; i < NUM_WRITES; i++)
    {
        WriteCommand cmd;        
        cmd.logicalPage = rand() % (writeLBARange);
        writeData(cmd.logicalPage);

        // unsigned char* data = (unsigned char*)malloc(PAGE_SIZE);
        // for (unsigned int j = 0; j < PAGE_SIZE; j++) {
        //     data[j] = rand() % 256;
        // }
        //cmd.data = data;
        //writeData(cmd.logicalPage, cmd.data);
        

        // 每100次print一次狀態
        if (i % 100 == 0) {
            printStatus();
        }

        //free(data);
    }
    
    // if (pthread_create(&write_thread, NULL, writeThread, &command) != 0) {
    //     fprintf(stderr, "Error creating write thread\n");
    //     return -100;
    // }
    
    // if (pthread_create(&status_thread, NULL, printStatusThread, NULL) != 0) {
    //     fprintf(stderr, "Error creating status thread\n");
    //     return -200;
    // }

    // if (pthread_join(write_thread, NULL) != 0) {
    //     fprintf(stderr, "Error joining write thread\n");
    //     return -101;
    // }

    // unsigned char readBuffer[PAGE_SIZE];
    // readData(command.logicalPage, readBuffer);
    // printf("Read data: %s\n", readBuffer);



    // if (pthread_cancel(status_thread) != 0) {
    //     fprintf(stderr, "Error cancelling status thread\n");
    //     return 3;
    // }

    printStatus();

    //pthread_mutex_destroy(&lock);

    return 0;

    // printf("CHAR_BIT    :   %d\n", CHAR_BIT);
    // printf("CHAR_MAX    :   %d\n", CHAR_MAX);
    // printf("CHAR_MIN    :   %d\n", CHAR_MIN);
    // printf("INT_MAX     :   %d\n", INT_MAX);
    // printf("INT_MIN     :   %d\n", INT_MIN);
    // printf("LONG_MAX    :   %ld\n", (long)LONG_MAX);
    // printf("LONG_MIN    :   %ld\n", (long)LONG_MIN);
    // printf("SCHAR_MAX   :   %d\n", SCHAR_MAX);
    // printf("SCHAR_MIN   :   %d\n", SCHAR_MIN);
    // printf("SHRT_MAX    :   %d\n", SHRT_MAX);
    // printf("SHRT_MIN    :   %d\n", SHRT_MIN);
    // printf("UCHAR_MAX   :   %d\n", UCHAR_MAX);
    // printf("UINT_MAX    :   %u\n", (unsigned int)UINT_MAX);
    // printf("ULONG_MAX   :   %lu\n", (unsigned long)ULONG_MAX);
    // printf("USHRT_MAX   :   %d\n", (unsigned short)USHRT_MAX);
}