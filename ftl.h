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
#define TOTAL_BLOCKS (TOTAL_DIES * BLOCKS_PER_DIE)
#define PAGES_PER_BLOCK 9
#define PAGE_SIZE 4096 //Bytes
#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE) //Bytes
#define OP_SIZE 0.83
#define INVALID 0xFFFFFFFF

typedef struct {
    unsigned int logicalPage; //LBA
    //const unsigned char *data; //Write Data
} WriteCommand;

// initial FTL
void initializeFTL();

// void* writeThread(void* arg);
// void* printStatusThread(void* arg);
//void writeData(unsigned int logicalPage, const unsigned char* data);
void writeData(unsigned int logicalPage);
//void readData(unsigned int logicalPage, unsigned char* data);

//L2P Table
unsigned int logicalToPhysical(unsigned int logicalPage);
//P2L Table
unsigned int physicalToLogical(unsigned int physicalPage);

void garbageCollect();
void displayValidCountTable();
void displayEraseCountTable();
void printStatus();

extern pthread_mutex_t lock;
extern int isGCActive;
extern int totalEraseCount;

#endif //FTL_H