#include "ftl.h"
#include "freeVB.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

//=====================================================================================================================
// Normal FTL tables:
// Table 1: FTL mapping table: Logical Page Address mapping to Physical Page Address.
// Table 2: FTL mapping table: Physical Page Address mapping to Logical Page Address, User Data and write counts.
// Table 3: VBStatus (Virtual Block Status) Table, record every Virtual Block's valid page count and erase count.
//=====================================================================================================================

// Logical Page Address Mapping to Physical Page Address Management
typedef struct {
    unsigned int logicalPageAddress;
    unsigned int physicalPageAddress;
} LogicalPageMappingTable;

// Physical Page Address Mapping to Logical Page Address and User Data and Write Count
typedef struct {
    unsigned int physicalPageAddress;
    unsigned int logicalPageAddress;
    unsigned short data;
    unsigned short count; //Write this physical page counts.
} PhysicalPageMappingTable;

// Virtual Block Status: Record Virtual Block's valid Pages and erase count.
typedef struct {
    unsigned int validCount;
    unsigned int eraseCount;
} VirtualBlockStatus;

// Table 1: L2P Table (Logical Page Address to Physical Page Address Table)
LogicalPageMappingTable L2P_Table[TOTAL_BLOCKS * PAGES_PER_BLOCK];

// Table 2: P2L Table (Physical Page Address Table to Logical Page Address)
PhysicalPageMappingTable P2L_Table[TOTAL_BLOCKS * PAGES_PER_BLOCK];

// Table 3: VBStatus (Valid Page Count), use Block Status struct.
VirtualBlockStatus VBStatus[BLOCKS_PER_DIE];

//=====================================================================================================================
// Global Varible
//=====================================================================================================================

// Total Erase Count
unsigned int TotalEraseCount;
// Free VB List
FreeVBList FreeVirtualBlockList;
// Current write target VB index.
unsigned int WriteTargetVB;
// Current GC target VB index.
unsigned int GCTargetVB;
// record how many pages can be program (write) on target VB.
unsigned int FreePagesCount;

// Initialize FTL Table and Parameters.
void initializeFTL() 
{
    //Initial L2P, P2L, VBStatus tables.
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++) {
        //L2P
        L2P_Table[i].logicalPageAddress = i;
        L2P_Table[i].physicalPageAddress = INVALID;
        
        //P2L        
        P2L_Table[i].physicalPageAddress = i;
        P2L_Table[i].logicalPageAddress = INVALID;        
        P2L_Table[i].data = INVALID_SHORT;
        P2L_Table[i].count = 0;
    }

    // Initial VBStatus's Valid Count and Erase Count.
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        VBStatus[i].validCount = 0;
        VBStatus[i].eraseCount = 0;
    }

    // Initial Free VB List
    initializeFreeVBList(&FreeVirtualBlockList);
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        insertFreeVB(&FreeVirtualBlockList, i, VBStatus[i].eraseCount);
    }

    // record how many pages can be program (write) on target VB, initial is total die and pages per block.
    FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;           
    // Set Current Write Target VB = index 0.
    WriteTargetVB = 0;
    // Delete Free VB index 0.
    deleteFreeVB(&FreeVirtualBlockList, WriteTargetVB);
    // Record Total Erase Count for VB.
    TotalEraseCount = 0;
}

// Update L2P Mapping Table
int updateL2PTable(unsigned int logicalPage, unsigned int physicalPage)
{        
    if (L2P_Table[logicalPage].logicalPageAddress != logicalPage) {
        printf(" L2P Table logical Address initial fail.\n");
        return -1;
    }

    // Determine physical page have value or not, if had VBStatus[].ValidCount--
    if (L2P_Table[logicalPage].physicalPageAddress != INVALID) {
        unsigned int tempVB = L2P_Table[logicalPage].physicalPageAddress / (TOTAL_DIES * PAGES_PER_BLOCK);
        VBStatus[tempVB].validCount--;
    }
    
    L2P_Table[logicalPage].physicalPageAddress = physicalPage;

    return 0;
}

// Update P2L Table (physical page address and logical page address and user data)
int updateP2LTable(unsigned int physicalPage, unsigned int logicalPage, unsigned short data)
{
    if (P2L_Table[physicalPage].physicalPageAddress != physicalPage) {
        printf(" P2L Table Physical Address initial fail.\n");
        return -1;
    }

    P2L_Table[physicalPage].logicalPageAddress = logicalPage;
    P2L_Table[physicalPage].data = data;
    P2L_Table[physicalPage].count++;

    return 0;
}

int scanValidTablesToErase()
{
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        if (VBStatus[i].validCount == 0) {
            // Simulate to Erase this VB.
            VBStatus[i].eraseCount++;
            for (unsigned int j = 0; j < TOTAL_DIES * PAGES_PER_BLOCK; j++) {
                P2L_Table[i * TOTAL_DIES * PAGES_PER_BLOCK + j].logicalPageAddress = INVALID;
                P2L_Table[i * TOTAL_DIES * PAGES_PER_BLOCK + j].data = INVALID_SHORT;
            }
            insertFreeVB(&FreeVirtualBlockList, i, VBStatus[i].eraseCount);
        }    
    }

    unsigned int avgEraseCount = TotalEraseCount / BLOCKS_PER_DIE;
    unsigned int maxEraseCount = 0;
    
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        if (VBStatus[i].eraseCount > maxEraseCount) {
            maxEraseCount = VBStatus[i].eraseCount;
        }        
    }
    
    if (maxEraseCount > avgEraseCount + 10) {
        garbageCollect(1);
    }
    
    return 0;
}

// Write Data to Flash
int writeDataToFlash(unsigned int logicalPage, unsigned int length, unsigned short data)
{        
    if (FreePagesCount == 0) {
        printf("[!!! Warning !!!] Free Pages Count is empty 0, Do find free VB ......\n");
        // If Free VB Counts < 3, do GC flow.
        if (countFreeVBNodes(&FreeVirtualBlockList) < 2) {// < 3 
            garbageCollect(0); // find min erase count VB.
        } else {
            // int result = getFreeVBNode();
            int getVB = getFreeVB(&FreeVirtualBlockList);
            if (getVB == -1) {
                scanValidTablesToErase();
                getVB = getFreeVB(&FreeVirtualBlockList);
                if (getVB == -1) {
                    printf("[Fail] Can't Find Any Free VB in List to Write !!!\n");
                    //system("PAUSE");
                    return -1;
                }            
            }
            
            WriteTargetVB = getVB;
            // deleteFreeVB(&FreeVirtualBlockList, WriteTargetVB);
            printf("[Done] Find New Free VB is: %u\n", WriteTargetVB);
            // Current Write VB's Free Page Count.
            FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;
        }
    }

    // Avoid Sequence Write for errase count is zero.
    if (FreePagesCount == 0)
    {
        int getVB = getFreeVB(&FreeVirtualBlockList);
            if (getVB == -1) {
                scanValidTablesToErase();
                getVB = getFreeVB(&FreeVirtualBlockList);
                if (getVB == -1) {
                    printf("[Fail] Can't Find Any Free VB in List to Write !!!\n");
                    //system("PAUSE");
                    return -1;
                }            
            }
            
            WriteTargetVB = getVB;
            // deleteFreeVB(&FreeVirtualBlockList, WriteTargetVB);
            printf("[Done] Find New Free VB is: %u\n", WriteTargetVB);
            // Current Write VB's Free Page Count.
            FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;
    }
    
    // Update L2P Table
    unsigned int targetPhysicalPage = (WriteTargetVB * TOTAL_DIES * PAGES_PER_BLOCK) + (TOTAL_DIES * PAGES_PER_BLOCK - FreePagesCount);
    if (updateL2PTable(logicalPage, targetPhysicalPage) != 0) {
        printf("updateL2PTable fail !!!");
    }

    // Update P2L Table
    if (updateP2LTable(targetPhysicalPage, logicalPage, data)) {
        printf("updateP2LTable fail !!!");
    }

    // Update VBStatus Table, Write Data and valid page + 1
    VBStatus[WriteTargetVB].validCount++;
    
    // Calc remaining Write Pages.
    FreePagesCount = FreePagesCount - length;

    return 0;
}

// GC Purpose: Find Next Free VB to write.
// 1. Find Erase target VB (Source VB): Minimal Valid Count.
// 2. target VB -> OP space or VBStatus[].validPage is 0  VB.
// 3. Write OP Space (Target New VB) -> this VB is newest Free Target VB.
// 4. Erase Source VB -> Erase Count + 1.
int garbageCollect(unsigned int wearLeveling)
{
    // find Minimal Valid Count on VB
    unsigned int minValidCount = TOTAL_DIES * PAGES_PER_BLOCK;
    // Min Erase count
    unsigned int minEraseCount = INVALID;
    // Source VB index, find needs GC's source block
    unsigned int sourceToEraseVB = INVALID; //target block to erase.
    // Find moveing on table.
    unsigned int targetToMoveVB = INVALID; //target block to move on (empty).

    // 1: [Wear Leveling] select Source VB -> find Minimal Erase Count VB.    
    if (wearLeveling == 1) {
        for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++)
        {
            if (VBStatus[i].eraseCount < minEraseCount)
            {
                minEraseCount = VBStatus[i].eraseCount;
                sourceToEraseVB = i;
            }
        }        
    }
    // 0: [Common GC] select Source VB -> find Minimal Valid Count VB.
    else 
    { // Common GC FG Flow: find min erase count
        for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
            if (VBStatus[i].validCount != 0 && VBStatus[i].validCount <= minValidCount)
            {
                minValidCount = VBStatus[i].validCount;
                sourceToEraseVB = i;
            }
        }             
    }

    // search first Free VB, and delete from Free VB List.
    int getVB = getFreeVB(&FreeVirtualBlockList);
    if (getVB == -1) {
	    scanValidTablesToErase();
	    getVB = getFreeVB(&FreeVirtualBlockList);
	    if (getVB == -1) {
		    printf("[Fail] Can't Find Any Free VB in List to Write !!!\n");
		    //system("PAUSE");
		    return -1;
	    }
    }

    targetToMoveVB = getVB;
    // deleteFreeVB(&FreeVirtualBlockList, targetToMoveVB);
            
    unsigned int tempTargetPage, tempLogical, tempPhysical;
    unsigned int tempSourceInitialPage = sourceToEraseVB * TOTAL_DIES * PAGES_PER_BLOCK;
    unsigned int validToMovePages = 0;

    // Calc Current physical page to write.
    tempTargetPage = targetToMoveVB * TOTAL_DIES * PAGES_PER_BLOCK;

    // 3. source VB's valid page and data write to Free VB.
    // Determine page is valid or not -> According P2L's logical address is same to L2P's Logical page and physical's value.
    for (unsigned int i = 0; i < TOTAL_DIES * PAGES_PER_BLOCK; i++) {
        tempLogical = P2L_Table[tempSourceInitialPage + i].logicalPageAddress;
        tempPhysical = L2P_Table[tempLogical].physicalPageAddress;
        
        // Detrermine is valid date or not，P2L mapping's Logical Addres is eual L2P's Physical or not.
        if (tempPhysical == (tempSourceInitialPage + i))
        {
            // Load physical data (Source)，Move to target's VB page.
            P2L_Table[tempTargetPage].logicalPageAddress = tempLogical;
            P2L_Table[tempTargetPage].data = P2L_Table[tempPhysical].data;
            P2L_Table[tempTargetPage].count++;

            // Update L2P Table to new Physical Page Address.
            L2P_Table[tempLogical].physicalPageAddress = tempTargetPage;

            // update VBStatus Table, Write Data for valid page + 1
            VBStatus[targetToMoveVB].validCount++;
            tempTargetPage++;
            validToMovePages++;
        }        
    }

    // 4. Erase this VB.
    VBStatus[sourceToEraseVB].eraseCount++;
    VBStatus[sourceToEraseVB].validCount = 0;
    for (unsigned int i = 0; i < TOTAL_DIES * PAGES_PER_BLOCK; i++) {
        P2L_Table[sourceToEraseVB * TOTAL_DIES * PAGES_PER_BLOCK + i].logicalPageAddress = INVALID;
        P2L_Table[sourceToEraseVB * TOTAL_DIES * PAGES_PER_BLOCK + i].data = INVALID_SHORT;
    }

    // Insert erased VB into Free VB List.
    insertFreeVB(&FreeVirtualBlockList, sourceToEraseVB, VBStatus[sourceToEraseVB].eraseCount);

    // Increase Total Erase Count.
    TotalEraseCount++;
    
    // Update write target DB and Free Page Count.
    WriteTargetVB = targetToMoveVB;
    FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK - validToMovePages;

    return 0;
}

// Read Data from Flash
unsigned short readDataFromFlash(unsigned int logicalPage, unsigned int length) {
    unsigned int ppa = L2P_Table[logicalPage].physicalPageAddress;
    return P2L_Table[ppa].data;
}

// Read Count from Flash
unsigned short readCountFromFlash(unsigned int logicalPage) {
    unsigned int ppa = L2P_Table[logicalPage].physicalPageAddress;
    return P2L_Table[ppa].count;
}

// Display Virtual Block's Valid Count and Erase Count.
void printVBStatus() 
{    
    printf("--------------------------------------------------------------\n");
    printf("                   Virtual Block Status\n");
    printf("--------------------------------------------------------------\n");

    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        printf("VB: %u, Valid Count = %u, Erase Count = %u\n", i, VBStatus[i].validCount, VBStatus[i].eraseCount);
    }
    
    printf("--------------------------------------------------------------\n");
}

int writeP2LTableToCSV() {
    time_t now = time(NULL); // Get current time
    struct tm *t = localtime(&now); // localization time

    char filename[64];
    // Format file name.
    snprintf(filename, sizeof(filename), "./P2L-Table_%04d%02d%02d_%02d%02d%02d.csv",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }
    
    // Write excel title.
    fprintf(file, "PhysicalPageAddress,LogicalPageAddress,Data\n");

    // Write down row data.
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++)
    {
        fprintf(file, "%u,%u,%hu,%hu", P2L_Table[i].physicalPageAddress, P2L_Table[i].logicalPageAddress, P2L_Table[i].data, P2L_Table[i].count);                    
        fprintf(file, "\n");
    }
    
    fclose(file);

    return 0;
}