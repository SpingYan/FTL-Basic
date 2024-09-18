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
#define BLOCKS_PER_DIE 2000 // blocks in 1 die : 200/500
#define PAGES_PER_BLOCK 9 // pages in 1 block : 9/3
#define PAGE_SIZE 16384 //Bytes

#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE) //Bytes
#define TOTAL_VB_PAGES (TOTAL_DIES * PAGES_PER_BLOCK)
#define TOTAL_BLOCKS (TOTAL_DIES * BLOCKS_PER_DIE)
#define TOTAL_PAGES (TOTAL_BLOCKS * PAGES_PER_BLOCK)

#define OP_SIZE 28 // Over Provisioning size

#define INVALID 0xFFFFFFFF
#define INVALID_SHORT 0xFFFF
#define INVALID_CHAR 0xFF

#define GC_TRIGGER_VB_COUNT 9
#define URGENT_GC_TRIGGER_VB_COUNT 3
#define WRITE_AND_GC_SEGMENTATION 4
#define SEGMENTAL_GC_PAGES 8

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
    unsigned int logical_address_write_count; //logical address Write counts.
    unsigned int physical_page_write_count;   //Write this physical page counts.
} physical_page_mapping_table_t;

// WAF value records
typedef struct {
    unsigned int loop;
    unsigned int nand_write;
    unsigned int host_write;
    double waf;
} waf_records_t;

// Written enumerate list
typedef enum {
    IDLE,
    WRITING,
    WRITTEN,
    GC_ING,
    GC_TARGET,
    WL_ING,
    WL_TARGET,
} program_status_enum_t;

// FTL Error enumerate list
typedef enum {
    OK,
    CAN_NOT_FIND_VALID_VB,
    CAN_NOT_FIND_FREE_VB,
    NO_NEED_WL,
} ftl_process_error_t;

// Virtual Block Status: Record Virtual Block's valid Pages and erase count.
typedef struct {
    unsigned int valid_count;
    unsigned int erase_count;
    program_status_enum_t program_status;
} virtual_block_status_t;

// Host write data struct
typedef struct {
    unsigned int logical_page;
    unsigned short data;
    unsigned int logical_write_count;
} host_write_data_t;

// Table 1: L2P table (Logical page address to physical page address table)
logical_page_mapping_table_t g_l2p_table[TOTAL_BLOCKS * PAGES_PER_BLOCK];

// Table 2: P2L table (Physical page address table to logical page address)
physical_page_mapping_table_t g_p2l_table[TOTAL_BLOCKS * PAGES_PER_BLOCK];

// Table 3: VB status (valid count and erase count, use block status struct)
virtual_block_status_t g_vb_status[BLOCKS_PER_DIE];

//===================================================================================================================== 
// Global Varible


// Wear-leveling
unsigned int g_do_wl_flag;


// Total Erase Count
unsigned int g_total_erase_count;
// Min Erase Count
unsigned int g_min_erase_count;
// Max Erase Count
unsigned int g_max_erase_count;
// Set do partition GC flow flag.
unsigned int g_partition_gc_ing;
// Set urgent GC flow flag.
unsigned int g_urgent_gc_ing;

// Avg Erase Count
double g_avg_erase_count;
// Last wl avg erase count
double g_avg_erase_count_last_wl;

// Total GC Count
unsigned int g_total_gc_count;
// Segmental GC Count
unsigned int g_segmental_gc_count;
// Total Wear-Leveling Count
unsigned int g_total_wl_count;

// Free VB List
vb_list_t g_free_vb_list;
// Written VB List
// vb_list_t g_written_vb_list;
// global variable.
unsigned int g_host_write_range;
// Current writing target VB index.
unsigned int g_writing_vb;
// record how many pages can be program (write) on target VB.
unsigned int g_remaining_pages_to_write;
// Current doing GC target VB index.
unsigned int g_gc_target_vb;
// record how many pages can be program (gc) on target VB.
unsigned int g_remaining_pages_to_gc;

// Current doing GC source VB index.
unsigned int g_gc_source_vb;
// record how many pages current moved from gc_source_vb
unsigned int g_gc_current_page_in_vb;

// Current doing WL target VB index.
unsigned int g_wl_target_vb;
// record how many pages can be program (wl) on target VB.
unsigned int g_remaining_pages_to_wl;
// records nand write size
unsigned int g_nand_write_size;
// records host write size
unsigned int g_host_write_size;

//=====================================================================================================================

// initial FTL related parameters.
void ftl_initialize();
// Write Data to Flash
int ftl_write_data_flow(unsigned int logical_page, unsigned int length, unsigned short data, unsigned short write_count);
// Write Data to Flash
int ftl_write_data_flow_new(unsigned int logical_page, unsigned int length, unsigned short data, unsigned int write_count);
// When g_remaining_pages_to_write is 0, get new vb to write.
int ftl_get_new_vb_to_write();
// When g_remaining_pages_to_gc is 0, get new vb to GC.
int ftl_get_new_vb_to_gc();
// Read Data from Flash for single LCA.
unsigned int ftl_read_data_flow(unsigned int logical_page, unsigned int length);
// Read LCA Counts from Flash for single LCA.
unsigned int ftl_read_page_write_count(unsigned int logical_page);
// Save detail table and vb status to csv file.
int ftl_write_vb_table_detail_to_csv(char* test_case);
// Save waf value to csv file.
int ftl_write_waf_record_to_csv(char* test_case, waf_records_t* record, unsigned int size);

// Update L2P Table
int ftl_update_table_L2P(unsigned int logical_page, unsigned int physical_page);
// Update P2L Table
int ftl_update_table_P2L(unsigned int physical_page, unsigned int logical_page, unsigned short data, unsigned int logical_address_write_count);
// Update Data Flow
int ftl_update_data(unsigned int logical_page, unsigned int length, unsigned short data, unsigned int logical_address_write_count);

// Do Garbage Collect Flow
int ftl_garbage_collection_new();
// Do segmentation gc 
int ftl_garbage_collection_segmentation(unsigned int gc_pages);
// Do Wear Leveling Flow
// int ftl_wear_leveling_new();
int ftl_wear_leveling_new();

// Determine do wear-leveling or not
int ftl_determine_wear_leveling();
// Scan if there are have 0 valid VB.
int ftl_scan_written_vb_to_erase();
// Erase vb flow
void ftl_erase_vb(unsigned int vb_index);
// Print vb status
void ftl_print_vb_status();

// Compare golden data
int ftl_compare_write_data(host_write_data_t* write_golden_data, unsigned int data_size);
// move valid page to target page for wl or gc.
void ftl_process_page(unsigned int src_page_idx, unsigned int target_vb, unsigned int* remaining_pages);

// Get smart value
unsigned int ftl_calc_min_erase_count();
unsigned int ftl_calc_max_erase_count();
double ftl_calc_avg_erase_count();

void ftl_determine_and_do_wear_leveling();
int ftl_do_wear_leveling_or_not();

void ftl_initialize_vb_status();
void ftl_initialize_global_l2p_table();
void ftl_initialize_global_p2l_table();
void ftl_initialize_free_vb_list();
void ftl_initialize_global_smart_value();
void ftl_initialize_write_related_parameters();
void ftl_initialize_gc_related_parameters();
void ftl_initialize_wl_related_parameters();

#endif // FTL_H