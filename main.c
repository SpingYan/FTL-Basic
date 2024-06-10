#include <stdio.h>
#include <stdlib.h>
//#include <pthread.h>
#include <time.h>
#include <string.h>
#include "ftl.h"
#include <unistd.h>
#include <windows.h>

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

typedef struct {
    unsigned int logicalPage; //LBA
    unsigned char *data; //Write Data
    unsigned int length; // length in LBA
} WriteCommand;

#define NUM_WRITES 1000
// #define DATA_SIZE 4 // 每一個 page 存放的 user data 為 4 bytes
// #define MAX_LBA_LENGTH 4 // 每次 command 傳送的 LBA 大小是 4 個 page

// 隨機產生擁有 length 長度的 Byte 資料
void generateRandomDataforCmd(unsigned char *data, unsigned char length) {
    
    for (unsigned int i = 0; i < length; i++) {
        srand(time(NULL) * time(NULL) * getpid() * clock());
        data[i] = rand() % 0xFF;
    }
    return;
}

int main(int argc, char **argv) {       
    // Initial FTL and related flow.
    initializeFTL();
    
    // pthread_t write_thread;
    // pthread_t status_thread;

    // 以 OP Size 來限制 LBA 寫入的 Range
    unsigned int writeLogicalPageAddressRange = TOTAL_BLOCKS * PAGES_PER_BLOCK * OP_SIZE;

    // 寫入 NUM_WRITES 次數
    // 1. 每 100 次 print 一次狀態，並寫 P2L Table 的檔案
    // 2. 每次寫完後 read data (L2P -> P2L -> check P2L's lba -> check P2L's data)
    for (unsigned int i = 0; i < NUM_WRITES; i++) {
        WriteCommand cmd;
        srand(time(NULL) * time(NULL) * getpid() * clock());
        cmd.logicalPage = rand() % (writeLogicalPageAddressRange);
        cmd.length = 1; // rand() % (MAX_LBA_LENGTH) + 1;
        cmd.data = (unsigned char *)malloc(cmd.length);
        generateRandomDataforCmd(cmd.data, cmd.length);
        
        // Write Data to Flash
        writeDataToFlash(cmd.logicalPage, cmd.length, cmd.data);

        // Read Data from Flash
        unsigned char *readData = (unsigned char *)malloc(cmd.length);
        readData = readDataFromFlash(cmd.logicalPage, cmd.length);
        if (readData == NULL) {
            printf("Can't Get Data from Flash!!!");
        }
        
        // Compare Data
        if (memcmp(cmd.data, readData, cmd.length) != 0) {
            printf("Data mismatch at Logical Page Address %u, length %u\n", cmd.logicalPage, cmd.length);
        } 
        else {
            printf("Data match at Logical Page Address %u, length %u\n", cmd.logicalPage, cmd.length);
        }

        // Release Data
        free(cmd.data);
        free(readData);

        // Write P2L Table to file and Print P2L and VPC Table Status.
        //const char *filename = "./P2L_Table";
        if (i != 0 && i % 100 == 0)
        {
            // Save P2L table data to file.
            writeP2LTableToCSV();
            printStatus();
            Sleep(1000);
        }        
    }

    printStatus();
    //pthread_mutex_destroy(&lock);
    system("PAUSE");

    return 0;
}