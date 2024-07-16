#ifndef FTL_H
#define FTL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "vblist.h"

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

#define TOTAL_DIES 4 // total dies
#define BLOCKS_PER_DIE 500 // blocks in 1 die
#define PAGES_PER_BLOCK 3 // pages in 1 block
#define PAGE_SIZE 16384 //Bytes

#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE) //Bytes
#define TOTAL_VB_PAGES (TOTAL_DIES * PAGES_PER_BLOCK)
#define TOTAL_BLOCKS (TOTAL_DIES * BLOCKS_PER_DIE)
#define TOTAL_PAGES (TOTAL_BLOCKS * PAGES_PER_BLOCK)

#define OP_SIZE 7 // Over Provisioning size

#define INVALID 0xFFFFFFFF
#define INVALID_SHORT 0xFFFF
#define INVALID_CHAR 0xFF

#define GC_TRIGGER_VB_COUNT 9

//=====================================================================================================================
// Normal FTL tables:
// Table 1: FTL mapping table: Logical Page Address mapping to Physical Page Address.
// Table 2: FTL mapping table: Physical Page Address mapping to Logical Page Address, User Data and write counts.
// Table 3: VBStatus (Virtual Block Status) Table, record every Virtual Block's valid page count and erase count.
//=====================================================================================================================
// Logical Page Address Mapping to Physical Page Address Management
typedef struct {
    unsigned int logical_page_index;
    unsigned int physical_page_address;
} logical_page_mapping_table_t;

// Physical Page Address Mapping to Logical Page Address and User Data and Write Count
typedef struct {
    unsigned int physical_page_index;
    unsigned int logical_page_address;
    unsigned short data;
    unsigned short logical_address_write_count; //logical address Write counts.
    unsigned short physical_page_write_count; //Write this physical page counts.
} physical_page_mapping_table_t;

// Written enumerate list
typedef enum {
    IDLE,
    WRITING,
    WRITTEN,
    GC_ING,
    GC_TARGET,
} program_status_enum_t;

// Virtual Block Status: Record Virtual Block's valid Pages and erase count.
typedef struct {
    unsigned int valid_count;
    unsigned int erase_count;
    program_status_enum_t program_status;
} virtual_block_status_t;

// Table 1: L2P table (Logical page address to physical page address table)
logical_page_mapping_table_t g_l2p_table[TOTAL_BLOCKS * PAGES_PER_BLOCK];

// Table 2: P2L table (Physical page address table to logical page address)
physical_page_mapping_table_t g_p2l_table[TOTAL_BLOCKS * PAGES_PER_BLOCK];

// Table 3: VB status (valid count and erase count, use block status struct)
virtual_block_status_t g_vb_status[BLOCKS_PER_DIE];

//===================================================================================================================== 
// Global Varible
// Total Erase Count
unsigned int g_total_erase_count;
// Total GC Count
unsigned int g_total_gc_count;
// Free VB List
vb_list_t g_free_vb_list;
// Written VB List
// vb_list_t g_written_vb_list;
// Current writing target VB index.
unsigned int g_writing_vb;
// record how many pages can be program (write) on target VB.
unsigned int g_remaining_pages_to_write;
// Current doing GC target VB index.
unsigned int g_gc_target_vb;
// record how many pages can be program (gc) on target VB.
unsigned int g_remaining_pages_to_gc;
//=====================================================================================================================

// initial FTL related parameters.
void ftl_initialize();
// Write Data to Flash
int ftl_write_data_flow(unsigned int logical_page, unsigned int length, unsigned short data, unsigned short write_count);
// When g_remaining_pages_to_write is 0, get new vb to write.
int ftl_get_new_vb_to_write();
// When g_remaining_pages_to_gc is 0, get new vb to GC.
int ftl_get_new_vb_to_gc();
// Read Data from Flash for single LCA.
unsigned short ftl_read_data_flow(unsigned int logical_page, unsigned int length);
// Read LCA Counts from Flash for single LCA.
unsigned short ftl_read_page_write_count(unsigned int logical_page);
// Save detail table and vb status to scv file.
int ftl_write_vb_table_detail_to_csv();
// Update L2P Table
int ftl_update_table_L2P(unsigned int logical_page, unsigned int physical_page);
// Update P2L Table
int ftl_update_table_P2L(unsigned int physical_page, unsigned int logical_page, unsigned short data, unsigned short logical_address_write_count);
// Do Garbage Collect Flow (wear-leveling = 0: Common GC, 1: Wear-Leveling)
int ftl_garbage_collection_new(int gc_mode);
// Do Garbage Collect Flow (wear-leveling = 0: Common GC, 1: Wear-Leveling)
// int ftl_garbage_collection(unsigned int wear_leveling);
// Determine do wear-leveling or not
int ftl_determine_wear_leveling();
// Scan if there are have 0 valid VB.
int ftl_scan_written_vb_to_erase();
// Erase vb flow
void ftl_erase_vb(unsigned int vb_index);
// Print vb status
void ftl_print_vb_status();

#endif // FTL_H