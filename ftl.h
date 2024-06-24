#ifndef FTL_H
#define FTL_H

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
// #define TOTAL_DIES 4
// #define BLOCKS_PER_DIE 6
// #define PAGES_PER_BLOCK 9
// #define PAGE_SIZE 4096 //Bytes

#define TOTAL_DIES 4 // Total Dies
#define BLOCKS_PER_DIE 7 //7
#define PAGES_PER_BLOCK 9
#define PAGE_SIZE 16384 //Bytes

#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE) //Bytes

#define TOTAL_BLOCKS (TOTAL_DIES * BLOCKS_PER_DIE)
#define TOTAL_PAGES (TOTAL_BLOCKS * PAGES_PER_BLOCK)

#define OP_SIZE 0.8
#define INVALID 0xFFFFFFFF
#define INVALID_CHAR 0xFF
#define INVALID_SHORT 0xFFFF

// initial FTL
void initializeFTL();
// Write Data to Flash
int writeDataToFlash(unsigned int logicalPage, unsigned int length, unsigned short data);
// Read Data from Flash for single LCA.
unsigned short readDataFromFlash(unsigned int logicalPage, unsigned int length);
// Save L2P Table to csv
int writeP2LTableToCSV();
// Update L2P Table
int updateL2PTable(unsigned int logicalPage, unsigned int physicalPage);
// Update P2L Table
int updateP2LTable(unsigned int physicalPage, unsigned int logicalPage, unsigned short data);
// Do Garbage Collect Flow (wear-leveling = 0: Common GC, 1: Wear-Leveling)
int garbageCollect(unsigned int wearLeveling);
// Print Virtual Block Status
void printVBStatus();

#endif //FTL_H