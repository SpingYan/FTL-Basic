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
    unsigned char data; //Write Data
    unsigned int length; // length in LBA
} WriteCommand;

#define NUM_WRITES 1000
// #define DATA_SIZE 4 // 每一個 page 存放的 user data 為 4 bytes
// #define MAX_LBA_LENGTH 4 // 每次 command 傳送的 LBA 大小是 4 個 page

unsigned int generate_complex_seed() {
    unsigned int seed = (unsigned int)time(NULL);
    seed ^= (unsigned int)clock();
    seed ^= (unsigned int)rand();  // 使用隨機數生成器本身的輸出
    seed ^= (unsigned int)getpid();
        
    return seed;
}

// 隨機產生擁有 length 長度的 Byte 資料
void generateRandomDataforCmd(unsigned char data, unsigned char length) {
    
    // for (unsigned int i = 0; i < length; i++) {
    //     srand(generate_complex_seed());
    //     data[i] = rand() % INVALID_CHAR;
    // }
    return;
}

int main(int argc, char **argv) {       
    printf("=============================================\n");
    printf("           Start to Basic FTL Test\n");
    printf("=============================================\n");
    
    // Initial FTL and related flow.
    initializeFTL();    
    printf("Initial FTL Parameters Done.\n");

    // 顯示 Virtual Block Status
    printStatus();
 
    // 以 OP Size 來限制 LBA 寫入的 Range.
    unsigned int writeLogicalPageAddressRange = TOTAL_BLOCKS * PAGES_PER_BLOCK * OP_SIZE;
    printf("OP Ratio is = %f, and Logical Page Range = 0 ~ %d\n", OP_SIZE, writeLogicalPageAddressRange);
    // --------------------------------------------------------------------------------------------
    // 寫入 NUM_WRITES 次數
    // 1. 每次寫完後 read data 後 compare (L2P -> P2L -> check P2L's lba -> check P2L's data)
    // 2. 每 100 次寫 P2L Table 的 csv 檔案，並 print 一次 VPC 狀態
    // --------------------------------------------------------------------------------------------    
    for (unsigned int i = 0; i < NUM_WRITES; i++) {
        WriteCommand cmd;
        Sleep(100);
        srand(generate_complex_seed());
        cmd.logicalPage = rand() % (writeLogicalPageAddressRange);
        cmd.length = 1; // rand() % (MAX_LBA_LENGTH) + 1;                
        cmd.data = rand() % INVALID_CHAR;

        // cmd.data = (unsigned char *)malloc(cmd.length);
        // generateRandomDataforCmd(cmd.data, cmd.length);
        // printf("Generate Write CMD - Random Data Done. (generateRandomDataforCmd)\n");
        printf("Write Command Info: cmd.logicalPage is [%u], cmd.data is [%02X]\n", cmd.logicalPage, cmd.data);

        // Write Data to Flash
        if (writeDataToFlash(cmd.logicalPage, cmd.length, cmd.data) != 0) {
            printf("Write Data to Flash Fail !!! (writeDataToFlash)\n");
        }            

        // Read Data from Flash
        // unsigned char *readData = (unsigned char *)malloc(cmd.length);
        // readData = readDataFromFlash(cmd.logicalPage, cmd.length);
        unsigned char readData = readDataFromFlash(cmd.logicalPage, cmd.length);
        // if (readData == NULL) {
        //     printf("Can't Get Data from Flash !!!");
        // }
        
        // Compare Data
        printf("Compare Data: Write Data is [%02X], Read Data is [%02X]\n", cmd.data, readData);
        // if (memcmp(cmd.data, &readData, cmd.length) != 0) {
        if (cmd.data != readData) {
            printf("Data mismatch at Logical Page Address %u, length %u\n", cmd.logicalPage);
        }         

        // Release Memory.
        //free(cmd.data);
        //free(readData);
        // --------------------------------------------------------------------------------------------

        // Write P2L Table to file and Print P2L and VPC Table Status.
        // const char *filename = "./P2L_Table";
        if (i != 0 && i % 100 == 0)
        {
            // Save P2L table data to file.
            if(writeP2LTableToCSV()!=0) {
                printf("Write P2L Table to csv File Fail !!!\n");
            }
            printf("Write P2L Table to csv File Success !!!\n");
            // Print VPC Status.
            printStatus();
            Sleep(1000);
        }        
    }

    // Display VB Status.
    printStatus();

    // Free 
    // freeTables();

    //pthread_mutex_destroy(&lock);
    system("PAUSE");

    return 0;
}