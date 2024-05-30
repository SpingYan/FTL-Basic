#include "ftl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #define TOTAL_DIES 4
// #define BLOCKS_PER_DIE 6
// #define TOTAL_BLOCKS (TOTAL_DIES * BLOCKS_PER_DIE)
// #define BLOCK_SIZE 4096 //Bytes
// #define PAGES_PER_BLOCK 9
// #define PAGE_SIZE (BLOCK_SIZE / PAGES_PER_BLOCK)
// #define INVALID 0xFFFFFFFF

//pthread_mutex_t lock;
int isGCActive = 0;
int totalEraseCount = 0;

// Page Mapping Table related struct, display logical and physical address.
typedef struct {
    unsigned int logicalPageNumber; //書上矩陣圖的數值
    unsigned int physicalPageNumber; //書上矩陣圖的實體位置
    unsigned char isValid; //是否為有效資料
} PageMappingEntry;

// Block Status: Record Block's valid Pages and erase count.
typedef struct {
    unsigned int validPages;
    unsigned int eraseCount;
} BlockStatus;

PageMappingEntry pageMapping[TOTAL_BLOCKS * PAGES_PER_BLOCK];
BlockStatus blockStatus[TOTAL_DIES][BLOCKS_PER_DIE]; //書中範例的二維陣列 Block Map
//unsigned char flashMemory[TOTAL_DIES][BLOCKS_PER_DIE][BLOCK_SIZE]; //實際上寫入 Flash 的資料空間大小 (Byte)

unsigned int physicalTargetBlocks[TOTAL_BLOCKS]; //管理空閒的 Block (實體)
// unsigned int freeBlockCount;

//初始化 FTL 相關使用的 Table
void initializeFTL() {
    for (unsigned int i = 0; i < TOTAL_DIES; ++i) {
        for (unsigned int j = 0; j < BLOCKS_PER_DIE; ++j) {
            blockStatus[i][j].validPages = 0;
            blockStatus[i][j].eraseCount = 0;            
            physicalTargetBlocks[i * BLOCKS_PER_DIE + j] = i * BLOCKS_PER_DIE + j; //依照 freeBlock 的位置填寫 Value.
        }
    }
    
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; ++i) {
        pageMapping[i].logicalPageNumber = INVALID;
        pageMapping[i].physicalPageNumber = INVALID;
        pageMapping[i].isValid = 0;
    }

    //freeBlockCount = TOTAL_BLOCKS;
    //pthread_mutex_init(&lock, NULL);
}

unsigned int logicalToPhysical(unsigned int logicalPage) 
{    
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; ++i) {
        if (pageMapping[i].logicalPageNumber == logicalPage) {
            return pageMapping[i].physicalPageNumber;
        }
    }
    return INVALID;
}

unsigned int physicalToLogical(unsigned int physicalPage) {
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; ++i) {
        if (pageMapping[i].physicalPageNumber == physicalPage) {
            return pageMapping[i].logicalPageNumber;
        }
    }
    return INVALID;
}

// 更新 Mapping Table 流程
void updateMapping(unsigned int logicalPage, unsigned int physicalPage) {
    unsigned int block = physicalPage / PAGES_PER_BLOCK; // 計算是在哪一個 block
    if (pageMapping[logicalPage].physicalPageNumber != INVALID) 
    {
        unsigned int oldPhysicalPage = pageMapping[logicalPage].physicalPageNumber; //將 oringinal physical page 轉移到 old physical page.
        unsigned int oldBlock = oldPhysicalPage / PAGES_PER_BLOCK;
        blockStatus[(oldBlock / BLOCKS_PER_DIE)][oldBlock % BLOCKS_PER_DIE].validPages--;
    }    
    pageMapping[logicalPage].logicalPageNumber = logicalPage;
    pageMapping[logicalPage].physicalPageNumber = physicalPage;
    blockStatus[(block / BLOCKS_PER_DIE)][block % BLOCKS_PER_DIE].validPages++;
}

// 寫資料的流程
//void writeData(unsigned int logicalPage, const unsigned char* data)
void writeData(unsigned int logicalPage)
{
    //pthread_mutex_lock(&lock);
    
    // if (/*freeBlockCount == 0 &&*/!isGCActive) 
    // {
    //     garbageCollect();
    // }

    //尋找 logical page 對應的 physical page
    unsigned int oldPhysicalPage = logicalToPhysical(logicalPage);
    if (oldPhysicalPage != INVALID)
    {
        //更新 block status
        blockStatus[oldPhysicalPage / (BLOCKS_PER_DIE * PAGES_PER_BLOCK)]
        [(oldPhysicalPage / PAGES_PER_BLOCK) % BLOCKS_PER_DIE].validPages--;
    }

    //unsigned int freeBlock = freeBlocks[freeBlockCount--];
    //unsigned int physicalPage = freeBlock * PAGES_PER_BLOCK;
    unsigned int targetBlock = INVALID;
    unsigned int targetPhysicalPage = INVALID;
    unsigned int findBlockCount = 0;
    while (targetBlock == INVALID || targetPhysicalPage == INVALID)
    {
        findBlockCount ++;
        if (findBlockCount > 5)
        {
            garbageCollect();
        }
        
        srand(time(NULL));
        targetBlock = rand() % TOTAL_BLOCKS;
        if (blockStatus[targetBlock / BLOCKS_PER_DIE][targetBlock % BLOCKS_PER_DIE].validPages < PAGES_PER_BLOCK)
        {            
            for (unsigned int i = 0; i < PAGES_PER_BLOCK; i++)
            {
                if (pageMapping[targetBlock * PAGES_PER_BLOCK + i].isValid == 0) {
                    targetPhysicalPage = targetBlock * PAGES_PER_BLOCK + i;
                    break;
                }
            }                
        }
    }
    
    //更新 Mapping Table
    pageMapping[targetPhysicalPage].logicalPageNumber = logicalPage;
    pageMapping[targetPhysicalPage].physicalPageNumber = targetPhysicalPage;
    pageMapping[targetPhysicalPage].isValid = 1;

    //更新 block status
    blockStatus[targetBlock / BLOCKS_PER_DIE][targetBlock % BLOCKS_PER_DIE].validPages++;
    
    //pthread_mutex_unlock(&lock);
}

// Write data thread.
// void* writeThread(void* arg) {
//     WriteCommand* command = (WriteCommand*)arg;
//     writeData(command->logicalPage, command->data);
//     return NULL;
// }

// void readData(unsigned int logicalPage, unsigned char* data) 
// {
//     unsigned int physicalPage = logicalToPhysical(logicalPage);
//     if (physicalPage != INVALID) {        
//         sprintf((char*)data, "Data for logical page %u", logicalPage);
//     } else {
//         strcpy((char*)data, "Invalid logical page");
//     }
// }


/*
1. 挑選 Oringinal Flash Block -> find min valid count (貪婪算法) and Erase Count
2. 從 Oringinal Flash Block 中找到有效數據
3. 把有效數據寫入目標 Flash Block
*/
void garbageCollect()
{
    //pthread_mutex_lock(&lock);
    isGCActive = 1;
    
    //尋找 Valid count 最少以及小於或等於平均某除次數的的 block
    unsigned int minValidPages = PAGES_PER_BLOCK + 1; // 9 + 1
    unsigned int victimBlock = INVALID; //target block to erase.
    unsigned int averageEraseCount = totalEraseCount / TOTAL_BLOCKS + 1;

    for (unsigned int i = 0; i < TOTAL_BLOCKS; i++) 
    {
        unsigned int die = i / BLOCKS_PER_DIE;
        unsigned int block = i % BLOCKS_PER_DIE;
        if (blockStatus[die][block].validPages < minValidPages && blockStatus[die][block].validPages > 0 && blockStatus[die][block].eraseCount <= averageEraseCount)
        {
            minValidPages = blockStatus[die][block].validPages;
            victimBlock = i;
        }
    }

    // Can't find target block
    if (victimBlock == INVALID) 
    {
        isGCActive = 0;
        // pthread_mutex_unlock(&lock);
        printf("Can't find target block !!!");
        return;
    }

    unsigned int die = victimBlock / BLOCKS_PER_DIE;
    unsigned int block = victimBlock % BLOCKS_PER_DIE;

    // 將此 block 有效的 page 移動到其他 block 上，準備 erase.
    for (unsigned int page = 0; page < PAGES_PER_BLOCK; ++page) 
    {
        unsigned int physicalPage = victimBlock * PAGES_PER_BLOCK + page;
        unsigned int logicalPage = physicalToLogical(physicalPage);

        if (logicalPage != INVALID) {
            unsigned char data[PAGE_SIZE];            
                                    
            // 標記無效 page
            pageMapping[physicalPage].logicalPageNumber = INVALID;
            pageMapping[physicalPage].physicalPageNumber = INVALID;

            unsigned int newPhysicalPage = INVALID;
            for (unsigned int i = 0; i < TOTAL_BLOCKS; ++i) {
                if (i != victimBlock && blockStatus[i / BLOCKS_PER_DIE][i % BLOCKS_PER_DIE].validPages < PAGES_PER_BLOCK) 
                {
                    newPhysicalPage = i * PAGES_PER_BLOCK + blockStatus[i / BLOCKS_PER_DIE][i % BLOCKS_PER_DIE].validPages;
                    break;
                }
            }

            if (newPhysicalPage != INVALID)
            {
                pageMapping[newPhysicalPage].logicalPageNumber = logicalPage;
                pageMapping[newPhysicalPage].physicalPageNumber = newPhysicalPage;
               
                // 更新 block status
                blockStatus[newPhysicalPage / (BLOCKS_PER_DIE * PAGES_PER_BLOCK)]
                           [(newPhysicalPage / PAGES_PER_BLOCK) % BLOCKS_PER_DIE].validPages++;
            } 
            else {
                printf("Error: No suitable free block found during garbage collection.\n");
                exit(1);
            }
        }
    }

    // Erase block and increase block count.
    blockStatus[die][block].validPages = 0;
    blockStatus[die][block].eraseCount++;
    totalEraseCount++;
    
    //freeBlocks[freeBlockCount++] = victimBlock;
    isGCActive = 0;
    
    //pthread_mutex_unlock(&lock);
}

void displayValidCountTable()
{
    printf("Valid Count Table:\n");
    for (unsigned int i = 0; i < TOTAL_DIES; i++) {
        for (unsigned int j = 0; j < BLOCKS_PER_DIE; j++) {
            printf("Die %u Block %u: %u valid pages\n", i, j, blockStatus[i][j].validPages);
        }
    }
}

void displayEraseCountTable() 
{
    printf("Erase Count Table:\n");
    for (unsigned int i = 0; i < TOTAL_DIES; i++) {
        for (unsigned int j = 0; j < BLOCKS_PER_DIE; j++) {
            printf("Die %u Block %u: %u erasures\n", i, j, blockStatus[i][j].eraseCount);
        }
    }
}

void printStatus() {
    //pthread_mutex_lock(&lock);
    printf("Block Status:\n");
    for (unsigned int i = 0; i < TOTAL_DIES; i++) {
        for (unsigned int j = 0; j < BLOCKS_PER_DIE; j++) {
            printf("Die %u Block %u: Valid Pages = %u, Erase Count = %u\n",
                   i, j, blockStatus[i][j].validPages, blockStatus[i][j].eraseCount);
        }
    }
    //pthread_mutex_unlock(&lock);
}