#include "ftl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Logical Page Address Management
typedef struct {
    unsigned int logicalPageAddress;
    unsigned int physicalPageAddress;
} LogicalPageMappingTable;

// Physical Page Address Mapping to Logical Page Address and User Data
typedef struct {
    unsigned int physicalPageAddress;
    unsigned int logicalPageAddress;
    unsigned char *data;
} PhysicalPageMappingTable;

// Virtual Block Status: Record Virtual Block's valid Pages and erase count.
typedef struct {
    unsigned int validPages;
    unsigned int eraseCount;
} VirtualBlockStatus;

/*
一般 FTL 有三張表:
Table 1: FTL mapping table 紀錄每個 Logical Page Address 對應的 Physical Page Address.
Table 2: FTL mapping table 紀錄每個 Physical Page Address 對應的 Logical Page Address 與 Data.
Options: VPBM (Valid Page Bit Map) Table, 紀錄每個 physical block 上哪一個 page 是有效數據 (移除 VPBM, 改為 P2L 方式確認是否為 Valid Page)
    ---> 實現在 GC 時只讀有效數據 Valid data.
Table 3: VPC (Valid page count) Table, 紀錄每個 physical block 的有效 page count.
    ---> 通常 GC 會排序此表回收最少 valid page 的 flash block
*/
// Table 1: L2P Table (Logical Page Address to Physical Page Address Table)
LogicalPageMappingTable L2P_Table[TOTAL_BLOCKS * PAGES_PER_BLOCK];
// // Options: P2L Table (Physical Page Address Table to Logical Page Address)
PhysicalPageMappingTable P2L_Table[TOTAL_BLOCKS * PAGES_PER_BLOCK];
// Table 2: VPBM (Valid Page Bit Map)
// PhysicalValidTable VPBM[BLOCKS_PER_DIE][TOTAL_DIES * PAGES_PER_BLOCK];
// Table 3: VPC (Valid Page Count), use Block Status struct.
VirtualBlockStatus VPC[BLOCKS_PER_DIE];

// 目前寫入的 Virtual Block 的 Free Page Count (每個 VB 上限為 TOTAL_DIES * PAGES_PER_BLOCK)
unsigned int FreePagesCount;
// 空的 Virtual Block 數量 (VB 上限為 TOTAL_DIES)
unsigned int FreeVirtualBlockCount;
// 當下目標寫入的 Virtual Block
unsigned int WriteTargetVB;
// Total Erase Count
unsigned int TotalEraseCount;

//初始化 FTL 相關使用的 Table
void initializeFTL() 
{
    //Initial L2P, P2L, VPBM tables.
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++)
    {
        //L2P
        L2P_Table[i].logicalPageAddress = i;
        L2P_Table[i].physicalPageAddress = INVALID;
        
        //P2L        
        P2L_Table[i].physicalPageAddress = i;
        P2L_Table[i].logicalPageAddress = INVALID;
        P2L_Table[i].data = NULL;
    }

    // Initial VPC (Valid Page Count), Block Status.
    // BlockStatus PhysicalBlockStatus[BLOCKS_PER_DIE][TOTAL_DIES * PAGES_PER_BLOCK]; //書中範例的二維陣列 Block Map
    // BlockStatus VPC[BLOCKS_PER_DIE];
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) 
    {
        VPC[i].validPages = 0;
        VPC[i].eraseCount = 0;
    }
        
    // 目前寫入的 Virtual Block 的 Free Page Count (每個 VB 上限為 TOTAL_DIES * PAGES_PER_BLOCK)
    FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;
    // 空的 Virtual Block 數量 (VB 上限為 TOTAL_DIES)
    FreeVirtualBlockCount = BLOCKS_PER_DIE;
    // 當下目標寫入的 Virtual Block
    WriteTargetVB = 0;    
    // 總 Erase Count，只要有做過 VB Erase 時都加 1
    TotalEraseCount = 0;
}

// Update L2P Mapping Table
int updateL2PTable(unsigned int logicalPage, unsigned int physicalPage)
{    
    L2P_Table[logicalPage].physicalPageAddress = physicalPage; 

    // for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++) {
    //     if (L2P_Table[i].logicalPageAddress == logicalPage) {
    //         L2P_Table[i].physicalPageAddress = physicalPage; 
    //     }
    // }

    return 0;
}

// Update P2L Table (physical page address and logical page address and user data)
int updateP2LTable(unsigned int physicalPage, unsigned int logicalPage, unsigned char *data)
{
    P2L_Table[physicalPage].logicalPageAddress = logicalPage;
    P2L_Table[physicalPage].data = data;

    // for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++) {
    //     if (P2L_Table[i].physicalPageAddress == physicalPage) {
    //         P2L_Table[i].logicalPageAddress = logicalPage;
    //         P2L_Table[i].data = data;
    //     }
    // }

    return 0;
}

// Write Data to Flash
int writeDataToFlash(unsigned int logicalPage, unsigned int length, unsigned char *data)
{        
    //判斷當下的 Current Block 的 Free Pages 數是否為 0
    if (FreePagesCount == 0)
    {
        //判斷 Free Virtual Block 的數量 (Criteria 判斷 OP Size 比例的)
        if (FreeVirtualBlockCount > (BLOCKS_PER_DIE - (BLOCKS_PER_DIE * OP_SIZE))) {
            FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;
            WriteTargetVB ++;
        }
        else {
            //經過 GC 流程後會在 Function 中重新設定 WriteTargetVB 與 FreePagesCount.
            garbageCollect();
        }
    }

    // 更新 L2P Table, 對應的 logical page 與 physical page
    // 宣告目標 Physical Page 為 正在寫入的 VB + 剩餘的 Free Page Count.
    unsigned int targetPhysicalPage = (WriteTargetVB * BLOCKS_PER_DIE) + (TOTAL_DIES * PAGES_PER_BLOCK - FreePagesCount);
    if (updateL2PTable(logicalPage, targetPhysicalPage) != 0) {
        printf("");
    }

    // 更新 P2L Table
    if (updateP2LTable(targetPhysicalPage, logicalPage, data)) {
        printf("");
    }

    // 更新 VPC Table, Write Data 只對對應的 valid page + 1
    VPC[WriteTargetVB].validPages++;
    
    // 使用的 Pages 數量陸續增加
    FreePagesCount--;    

    return 0;
}

int writeP2LTableToCSV(const char *filename) {    
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }
    
    // 寫入 CSV 標題
    fprintf(file, "PhysicalPageAddress,LogicalPageAddress,Data\n");

    // 寫入每一行數據
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++)
    {
        fprintf(file, "%u,%u,%02X", P2L_Table[i].physicalPageAddress, P2L_Table[i].logicalPageAddress, P2L_Table[i].data);
        fprintf(file, "\n");
    }
    
    fclose(file);
}

// Read Data from Flash
unsigned char* readDataFromFlash(unsigned int logicalPage, unsigned int length)
{
    unsigned int ppa = L2P_Table[logicalPage].physicalPageAddress;            
    return P2L_Table[ppa].data;
}

// Get physical block address from L2P Table.
unsigned int logicalToPhysical(unsigned int logicalPage) 
{    
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++) 
    {
        if (L2P_Table[i].logicalPageAddress == logicalPage) {
            return L2P_Table[i].physicalPageAddress;
        }
    }
    return INVALID;
}

/*
可用的 Free Flash Block 小於某個閥值. (Physical)
0. 觸發點 -> FreePagesCount <= TOTAL_BLOCKS * PAGES_PER_BLOCK
1. 挑選 Oringinal Flash Block -> find min valid count (貪婪算法) and 較小的 Erase Count 的 block
2. 從 Oringinal Flash Block 中找到有效數據
3. 把有效數據寫入目標 Flash Block, 目標 block = 搜尋條件為 erase count 較高的 block.
*/
void garbageCollect()
{
    // pthread_mutex_lock(&lock);
    // IsGCActive = 1;

    // 尋找空的 VB, moveToVB
    // 尋找最少的 valid count 對應的 VB, toEraseVB -> new FreeVB    

    // 尋找最小的 Valid Count 的 block
    unsigned int minValidPages = TOTAL_DIES * PAGES_PER_BLOCK + 1; // 36 + 1
    // 尋找小於或等於平均某除次數的的 block
    unsigned int averageEraseCount = TotalEraseCount / BLOCKS_PER_DIE + 1;
    // 尋找 Erase Count 最小的 block
    unsigned int minEraseCounts = INVALID;

    // 尋找到需要 GC 的 target block
    unsigned int toEraseVictimBlock = INVALID; //target block to erase.
    // 尋找到的 GC Target Block
    unsigned int moveToVictimBlock = INVALID; //target block to move on.

    // ---------------------------------------------------------------------------------------------
    // 1. 挑選 Oringinal Flash Block -> find min valid count (貪婪算法) and 較小的 Erase Count 的 block
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) 
    {
        if (VPC[i].validPages < minValidPages && VPC[i].validPages >= 0 && VPC[i].eraseCount <= averageEraseCount)
        {
            minValidPages = VPC[i].validPages;
            toEraseVictimBlock = i;
        }        
    }    
    
    // Can't find target block
    if (toEraseVictimBlock == INVALID) 
    {
        //IsGCActive = 0;
        // pthread_mutex_unlock(&lock);
        printf("Can't find target block !!!");
        return;
    }

    // 顯示要做 GC 的 Block, 與需搬移的 Pages 數.
    // 要做 Erase 的 VB.
    printf("[GC Flow] Target Block for Erase: %u\n", toEraseVictimBlock);    
    // 需要移動的 Valid Pages.
    printf("[GC Flow] Move Pages Number %u:\n", minValidPages);
    // ---------------------------------------------------------------------------------------------


    // 尋找適合的 Target Block (Erase Count 最高且有 Valid page 尚未用完的 Block 開始搜尋.)
    // 並移動
    while (minValidPages > 0)
    {
        for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) 
        {
            // 若是要做 Erase 的 VB 則跳出.
            if (i == toEraseVictimBlock)
            {
                break;
            }
            
            if (VPC[i].eraseCount < minEraseCounts && VPC[i].validPages < TOTAL_DIES * PAGES_PER_BLOCK)
            {
                minEraseCounts = VPC[i].eraseCount;
                moveToVictimBlock = i;
            }
        }

        printf("[GC Flow] Target Physical Block to Move: %u\n", moveToVictimBlock);
        
        // 針對此 block 可以移動過去的 page 數
        unsigned int movePageCount = TOTAL_DIES * PAGES_PER_BLOCK - VPC[moveToVictimBlock].validPages;
        minValidPages = minValidPages - movePageCount;

        // 將對應的 Valid Page 寫入對應的 moveToVictimBlock.
        for (unsigned int i = 0; i < TOTAL_DIES * PAGES_PER_BLOCK; i++)
        {
                                          
            // 更新 VPC, Block Status, total page valid count +1.
            VPC[moveToVictimBlock].validPages++;
            
        }
    }    

    // Erase block and update VPC block info.
    VPC[toEraseVictimBlock].validPages = 0;
    VPC[toEraseVictimBlock].eraseCount++;
    TotalEraseCount++;

    // 設定目標的 Erase VB.
    WriteTargetVB = toEraseVictimBlock;
    // 回復 Free Pages 的數量
    FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;

    // IsGCActive = 0;    
    //pthread_mutex_unlock(&lock);
}

/*
Wear-Leveling
1. 將 valid count 較多的且 Erase count 較少的 block 上的數據搬移到 Erase Count 較多的 block 中
*/
void wearLeveling()
{

}

// 顯示 Virtual Block 的 Valid Count 與 Erase Count 資訊
void printStatus() 
{    
    printf("Virtual Block Status:\n");
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) 
    {
        printf("VB: %u, Valid Pages = %u, Erase Count = %u\n", i, VPC[i].validPages, VPC[i].eraseCount);
    }
}

//-------------------------------------------------------------------------------------------------
// Backup Source Code (Not Used)
//-------------------------------------------------------------------------------------------------
// Update VBPM. 將 Physical address 位置設定成帶入的參數 0 or 1.
// unsigned int updateVBPM(unsigned int physicalPage, unsigned char valid)
// {
    // int empty = 1;
    // for (unsigned int i = 0; i <  BLOCKS_PER_DIE; i++)
    // {
    //     for (unsigned int j = 0; j < TOTAL_DIES * PAGES_PER_BLOCK; j++)
    //     {
    //         if (VPBM[i][j].physicalPageAddress == physicalPage) 
    //         {
    //             VPBM[i][j].isValid = valid;
    //             empty = 0;
    //         }
    //     }                
    // }
    // if (empty)
    // {
    //     VPBM[physicalPage/BLOCKS_PER_DIE][physicalPage%BLOCKS_PER_DIE].physicalPageAddress = physicalPage;
    //     VPBM[physicalPage/BLOCKS_PER_DIE][physicalPage%BLOCKS_PER_DIE].isValid = valid;
    // }
//     return 0;
// }

// Get logical block address from P2L Table.
// unsigned int physicalToLogical(unsigned int physicalPage)
// {
//     for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; ++i) 
//     {
//         if (P2L_Table[i].physicalPageAddress == physicalPage) {
//             return P2L_Table[i].logicalPageAddress;
//         }
//     }
//     return INVALID;
// }

// GC Update Physical Table.
// unsigned int updateMappingTableforGC(unsigned int oriPhysicalPage, unsigned int targetPhysicalPage)
// {
//     // VPBM[oriPhysicalPage].isValid = 0;
//     // VPBM[targetPhysicalPage].isValid = 1;
//     // unsigned int oriLogicalPage = INVALID;
//     // unsigned int targetLogicalPage = INVALID;
//     // for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++)
//     // {
//     //     if (L2P_Table[i].physicalPageAddress == oriPhysicalPage && VPBM[i].isValid == 1) 
//     //     {
//     //         oriLogicalPage = L2P_Table[i].logicalPageAddress;
//     //     }
//     // }
//     //updateMappingTable()
// }

// Write Data Flow
// void writeData(unsigned int logicalPage, unsigned char *data)
// {       
//     // --------------------------------------------------------------------------------------------
//     // 判斷是否需要做 GC
//     // 做完 GC 之後原設定新的 FreeTargetBlockPage, FreePagesCount 會回復成 36
//     // 設定 1 個是否已寫過 5 個 VB 的 flag.
//     if (FreePagesCount == 0)
//     {
//         unsigned int writedBlockPage = 0;
//         for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++)
//         {
//             if (VPC[i].validPages == 0)
//             {
//                 writedBlockPage++;
//                 FreeVirtualBlockCount=i;                
//             }
//         }
        
//         // Free Block Page 數量已不足, 做 GC. 閥值設為 4
//         if (writedBlockPage > 4)
//         {
//             garbageCollect();    
//         }
//         else
//         {
//             FreePagesCount = 36;
//             //FreeTargetBlockPage++;
//         }            
//     }
//     // --------------------------------------------------------------------------------------------

//     // 更新原先對應的 Physical page 之 Valid count.
//     unsigned int oldPhysicalPage = logicalToPhysical(logicalPage);
//     if (oldPhysicalPage != INVALID)
//     {
//         // 更新 VPBM, 將 vaild flag 設為 0
//         updateVBPM(oldPhysicalPage, 0);        

//         // 更新 VPC, Block Status, 將 total page valid count -1
//         VPC[oldPhysicalPage / BLOCKS_PER_DIE].validPages--;
//     }

//     // 尋找對應的 free physical block/page
//     unsigned int targetBlock = INVALID; // not used.
//     unsigned int targetPhysicalPage = INVALID;

//     if (FreeVirtualBlockCount == 0)
//         targetPhysicalPage = TOTAL_DIES * PAGES_PER_BLOCK - FreePagesCount;
//     else
//         targetPhysicalPage = (FreeVirtualBlockCount * BLOCKS_PER_DIE) + (TOTAL_DIES * PAGES_PER_BLOCK - FreePagesCount);
    
//     FreePagesCount--;
//     // 更新 logical page address 對應的 physical page address.
//     updateL2PTable(logicalPage, targetPhysicalPage);        
//     // 將 VPBM 對應的 physical page 設為 1
//     updateVBPM(targetPhysicalPage, 1);
//     // 更新 VPC, Block Status, total page valid count +1.
//     VPC[targetPhysicalPage / BLOCKS_PER_DIE].validPages++;
        
//     // pthread_mutex_unlock(&lock);
// }