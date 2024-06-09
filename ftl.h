#ifndef FTL_H
#define FTL_H
#include <pthread.h>

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

// 深入淺出 SSD 書本的案例
#define TOTAL_DIES 4
#define BLOCKS_PER_DIE 6
#define PAGES_PER_BLOCK 9
#define PAGE_SIZE 4096 //Bytes
#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE) //Bytes

#define TOTAL_BLOCKS (TOTAL_DIES * BLOCKS_PER_DIE)
#define TOTAL_PAGES (TOTAL_BLOCKS * PAGES_PER_BLOCK)

#define OP_SIZE 0.83
#define INVALID 0xFFFFFFFF

// int IsGCActive = 0;
// unsigned int TotalEraseCount;

// initial FTL
void initializeFTL();
// Write Data to Flash
int writeDataToFlash(unsigned int logicalPage, unsigned int length, unsigned char *data);

//L2P Table
int updateL2PTable(unsigned int logicalPage, unsigned int physicalPage);
//P2L Table
int updateP2LTable(unsigned int physicalPage, unsigned int logicalPage, unsigned char *data);

// save L2P Table to csv
extern int writeP2LTableToCSV();
// Read Data from Flash
extern unsigned char* readDataFromFlash(unsigned int logicalPage, unsigned int length);

void garbageCollect();
void wearLeveling();

// void* writeThread(void* arg);
// void* printStatusThread(void* arg);

// //L2P Table
// unsigned int logicalToPhysical(unsigned int logicalPage);
// //P2L Table
// unsigned int physicalToLogical(unsigned int physicalPage);

//Trim 對 invalid page 做處理
void trim();

void displayValidCountTable();
void displayEraseCountTable();

void printStatus();

// extern pthread_mutex_t lock;
// extern int isGCActive;

#endif //FTL_H