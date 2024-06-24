#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ftl.h"
#include <unistd.h>
#include <windows.h>
#include "freeVB.h"

// Write Command
typedef struct {
    unsigned int logicalPage;   //LCA
    unsigned short data;        //Write Data
    unsigned int length;        //length in LCA
} WriteCommand;

// Write Data Info Array
typedef struct {
    unsigned int logicalPage;
    unsigned short data;
} WriteData;

#define NUM_WRITES 2000
#define NUM_LOOPS 100
#define LCA_RANGE 180 //(TOTAL_BLOCKS * PAGES_PER_BLOCK * OP_SIZE)
// #define DATA_SIZE 4 // every page's user data is 4 bytes.
// #define MAX_LBA_LENGTH 4 // every command length LBA Size is 4 pages.

unsigned int generate_complex_seed() {
    unsigned int seed = (unsigned int)time(NULL);
    seed ^= (unsigned int)clock();
    seed ^= (unsigned int)rand();
    seed ^= (unsigned int)getpid();
        
    return seed;
}

// Initialize Golden Write Data. (data is logical page)
void initializeWriteGoldenData(WriteData *glodenWriteData, unsigned int lcaRange) {
   // unsigned int length = sizeof(glodenWriteData);
   for (unsigned int i = 0; i < lcaRange; i++) {
        glodenWriteData[i].logicalPage = i;
        glodenWriteData[i].data = i;
   }
}

// Write Pattern Cases
// Add Sequence Write's Pattern，write 0 ~ 179 LCA.
// Add position Write's Pattern，Pre-condition write 0 ~ 179 after 0 ~ 179. after full range,
// Direct to write single LCA 0 address.
// Modify Random write's Pattern，Before write, do Pre-Condition first.
// ------------------------------------------------------------------------------------------------
// [Pre-Condition] Add a Sequence Write pattern that writes 0 to 179 LCA in order for loops.
int sequenceWrite_PreCondition(WriteData *glodenWriteData, unsigned int start, unsigned int end) {    
    for (unsigned int i = start; i < end; i++) {
        writeDataToFlash(i, 1, glodenWriteData[i].data);        
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
// [Write Test] Add a Sequence Write pattern that writes 0 to 179 LCA in order for loops.
int sequenceWrite_Test(WriteData *glodenWriteData, unsigned int start, unsigned int end) {
    for (unsigned int loop = 0; loop < NUM_LOOPS; loop++) {
        for (unsigned int i = start; i < end; i++) {
            writeDataToFlash(i, 1, glodenWriteData[i].data);            
        }
        printf("[Loop: %u] Sequence Write Test Done.\n", loop);
    }
    return 0;
}

// ------------------------------------------------------------------------------------------------
// [Write Test] Perform a pre-condition by writing all positions from 0 to 179 first in order (SEQ Write 100%),
// then Random Write for loop.
int randomWrite_Test(WriteData *glodenWriteData, unsigned int start, unsigned int end) {
    // random seed.
    srand(generate_complex_seed());

    // Determine start and end parameters is reasonable. 
    if (start > end) {
        printf("[Fail] The number START is greater than the number END !!!\n");
        return -1;
    } else if ( start == end) {
        printf("[Fail] The number START is equal to the number END !!!\n");
        return -2;
    }

    unsigned int lcaRange = end - start;

    for (unsigned int loop = 0; loop < NUM_WRITES; loop++) {
        unsigned int lca = start + (rand() % (lcaRange));
        writeDataToFlash(lca, 1, glodenWriteData[lca].data);        
    }
    printf("[Loops: 1000] Random Write LCA: %u Data Done.\n");

    return 0;
}

// ------------------------------------------------------------------------------------------------
// [Write Test] Add a 0 position write pattern. First, perform a pre-condition by writing all positions from 0 to 179 in order, 
// then continuously write to LCA position 0 for loops.
int pointWrite_Test(WriteData *glodenWriteData, unsigned int logicalPageAddress) {
    for (unsigned int loop = 0; loop < NUM_WRITES; loop++) {
        writeDataToFlash(logicalPageAddress, 1, glodenWriteData[logicalPageAddress].data);        
    }
    printf("[Loop: 1000] Position Write LCA: %u Data Done.\n", logicalPageAddress);
    return 0;
}

int main(int argc, char **argv) {
    // --------------------------------------------------------------------------------------------
    // Select Test Pattern.
    printf("=======================================================================================\n");
    printf("                             Start to Basic FTL Test\n");
    printf("=======================================================================================\n");
    printf("Please Select Test Pattern:\n");
    printf("    1. Sequence Write. (Add a Sequence Write pattern that writes 0 to 179 LCA in order for loops)\n");
    printf("\n");
    printf("    2. Random Write. (Perform a pre-condition by writing all positions from 0 to 179),\n");
    printf("       first in order (SEQ Write 100%%), then Random Write for loop.\n");    
    printf("\n");
    printf("    3. 0 Position Write. (Add a single point write pattern.\n");
    printf("       First, perform a pre-condition by writing all positions from 0 to 179 in order,\n");
    printf("       then continuously write to LCA position 0 for loops.)\n");
    printf("\n");
    printf(": ");

    int select = 0;
    unsigned int testCase = 0;
    scanf("%d", &select);

    switch (select)
    {
    case 1:        
        printf("You select the [Sequence Write] Pattertn.\n");
        testCase = 1;
        break;
    case 2:
        printf("You select the [Random Write] Pattertn.\n");
        testCase = 2;
        break;
    case 3:
        printf("You select the [0 Position Write] Pattertn.\n");
        testCase = 3;
        break;
    default:
        printf("YOU DON'T select any pattern, it will test [[Sequence Write] Pattern !!!\n");
        testCase = 1;
        break;
    }


    //---------------------------------------------------------------
    // Initial FTL and related flow.
    initializeFTL();
    printf("[Done] Initial FTL Table and Parameters Done.\n");
    // Present Virtual Block Status
    printVBStatus();
    
    //---------------------------------------------------------------
    // Fixed LCA Length for Golden Write Data.
    WriteData glodenWriteData[LCA_RANGE];
    initializeWriteGoldenData(glodenWriteData, LCA_RANGE);

    //---------------------------------------------------------------
    // Pre-Condition
    sequenceWrite_PreCondition(glodenWriteData, 0, LCA_RANGE);
    printf("[Done] Sequence Write Pre-Condition Done.\n");
    // Present Virtual Block Status
    printVBStatus();
    // system("PAUSE");
    
    //---------------------------------------------------------------
    // Do Write Test
    int result = 0;
    switch (testCase)
    {
    case 1:
        result = sequenceWrite_Test(glodenWriteData, 0, LCA_RANGE);
        if (result != 0) {
            printf("[Fail] Sequence Write Flow had Error !!!\n");
        }
        break;
    case 2:
        result = randomWrite_Test(glodenWriteData, 0, LCA_RANGE);
        if (result != 0) {
            printf("[Fail] Random Write Flow had Error !!!\n");
        }        
        break;
    case 3:
        result = pointWrite_Test(glodenWriteData, 0);
        if (result != 0) {
            printf("[Fail] Point Write Flow had Error !!!\n");
        }
        break;
    default:
        printf("[Info] There are NO Test have to Excute !!!\n");
        break;
    }    
    
    printf("[Finished] Write Test Done.\n");    
    // Write P2L Table data to csv.
    writeP2LTableToCSV();    
    // Present Virtual Block Status    
    printVBStatus();        
    // Read Flash Data to compare Golden Write data.
    // allocate a temp array for rtead P2L data.
    unsigned short* tempReadData = (unsigned short*)malloc(LCA_RANGE * sizeof(unsigned short));
    unsigned int i = 0;    
    
    // --------------------------------------------------------------------------------------------
    // Read data from flash to temp buffer.
    for (i; i < LCA_RANGE; i++) {
        tempReadData[i] = readDataFromFlash(i, 1);
    } 

    // --------------------------------------------------------------------------------------------
    // Compare to golden buffer table.
    for (i; i < LCA_RANGE; i++) {
        if  (tempReadData[i] != glodenWriteData[i].data) {
            printf("[Fail] Data Compare Fail in LCA: %u, Data Read: %hu, Golden Data: %hu\n", i, tempReadData[i], glodenWriteData[i].data);
            result = -1;
        }
    }

    if (result == 0) {
        printf("[Sucess] All data is match\n");
    } else if (result == -1) {
        printf("[Fail] Data mismatch after write test !!!\n");
    }    
    // Free Momery
    free(tempReadData);
    
    system("PAUSE");

    return 0;
}