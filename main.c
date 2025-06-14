#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <windows.h>

#include "ftl.h"
#include "vblist.h"

#define NUM_WRITES 1000000
#define NUM_LOOPS 10000
// #define HOST_WRITE_RANGE (PAGES_PER_BLOCK * TOTAL_DIES * BLOCKS_PER_DIE * OP_SIZE)
#define HOST_WRITE_RANGE ((TOTAL_VB_PAGES * (BLOCKS_PER_DIE) * 100) / (100 + OP_SIZE))

// record waf value.
waf_records_t g_waf_record_data[NUM_LOOPS];

unsigned int generate_complex_seed()
{
    unsigned int seed = (unsigned int)time(NULL);
    seed ^= (unsigned int)clock();
    seed ^= (unsigned int)rand();
    seed ^= (unsigned int)getpid();

    return seed;
}

// Initialize Golden Write Data. (data is logical page)
void initialize_data(host_write_data_t *write_golden_data, unsigned int host_write_range)
{
   // unsigned int length = sizeof(write_golden_data);
   for (unsigned int i = 0; i < host_write_range; i++) {
        write_golden_data[i].logical_page = i;
        write_golden_data[i].data = i;
        write_golden_data[i].logical_write_count = 0;
   }
}

// Compare data from golden to nand flash
void compare_data(host_write_data_t *write_golden_data, unsigned int data_size)
{
    int compare_result = ftl_compare_write_data(write_golden_data, g_host_write_range);
    if (compare_result == 0) {
        printf("[Sucess] Compared data done.\n");
        printf("<-----\n");    
    } else if (compare_result == -1) {
        printf("[Fail] Data mismatch after write test !!!\n");
        system("pause");
    } else if (compare_result == -2) {
        printf("[Fail] Write Counts mismatch after write test !!!\n");
        system("pause");
    }
}

// ------------------------------------------------------------------------------------------------
// [Pre-Condition] Add a Sequence Write pattern that writes 0 to end of LCA range in order for loops.
int precondition_sequence_write(host_write_data_t *write_golden_data, unsigned int start, unsigned int end) 
{    
    for (unsigned int i = start; i < end; i++) {
        // increase write count.
        write_golden_data[i].logical_write_count++;
        // write data to flash.
        ftl_write_data_flow_new(i, 1, write_golden_data[i].data, write_golden_data[i].logical_write_count);
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
// [Sequence Write] Add a Sequence Write pattern that writes 0 to end of LCA range in order for loops.
int sequence_write(host_write_data_t *write_golden_data, unsigned int start, unsigned int end) 
{
    for (unsigned int loop = 0; loop < NUM_LOOPS; loop++) {
        for (unsigned int i = start; i < end; i++) {
            // increase write count.
            write_golden_data[i].logical_write_count++;
            // write data to flash.
            ftl_write_data_flow_new(i, 1, write_golden_data[i].data, write_golden_data[i].logical_write_count);                        
        }

        // Compare data
        compare_data(write_golden_data, g_host_write_range);

        // record waf value.
        g_waf_record_data[loop].loop = (loop+1)*100;
        g_waf_record_data[loop].nand_write = g_nand_write_size;
        g_waf_record_data[loop].host_write = g_host_write_size;
        g_waf_record_data[loop].waf = (double)g_nand_write_size/g_host_write_size;

        printf("=======================================================================================\n");
        printf("[Loop: %u] Sequence Write Test Done.\n", loop + 1);
        printf("=======================================================================================\n");        
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
// [Random Write] Perform a pre-condition by writing all positions from 0 to 179 first in order (SEQ Write 100%),
// then Random Write for loop.
int random_write(host_write_data_t *write_golden_data, unsigned int start, unsigned int end) 
{
    // Determine start and end parameters is reasonable. 
    if (start > end) {
        printf("[Fail] The number START is greater than the number END !!!\n");
        return -1;
    } else if ( start == end) {
        printf("[Fail] The number START is equal to the number END !!!\n");
        return -2;
    }

    // random seed.
    srand(generate_complex_seed());

    unsigned int random_write_range = end - start;
    for (unsigned int loop = 0; loop < NUM_WRITES; loop++) {
        unsigned int host_idx = start + (rand() % (random_write_range));
        // increase write count.
        write_golden_data[host_idx].logical_write_count++;
        // write data to flash.        
        ftl_write_data_flow_new(host_idx, 1, write_golden_data[host_idx].data, write_golden_data[host_idx].logical_write_count);
        // Compare data
        compare_data(write_golden_data, g_host_write_range);

        if (loop == NUM_WRITES - 1 || (loop % 100 == 0 && loop != 0)) {
            
            unsigned int waf_idx = 0;
            if (loop == NUM_WRITES - 1) {
                waf_idx = NUM_LOOPS - 1;
            } else { 
                waf_idx = loop/100 - 1;
            }

            // Compare data
            compare_data(write_golden_data, g_host_write_range);
            
            // record waf value.
            g_waf_record_data[waf_idx].loop = loop;
            g_waf_record_data[waf_idx].nand_write = g_nand_write_size;
            g_waf_record_data[waf_idx].host_write = g_host_write_size;
            g_waf_record_data[waf_idx].waf = (double)g_nand_write_size/g_host_write_size;

            printf("=======================================================================================\n");
            printf("[Loop: %u] Random Write Test Done.\n", loop + 1);
            printf("=======================================================================================\n");
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
// [Write Test] Add a 0 position write pattern. First, perform a pre-condition by writing all positions from 0 to 179 in order, 
// then continuously write to LCA position 0 for loops.
int single_point_write(host_write_data_t *write_golden_data, unsigned int write_address)
{
    for (unsigned int loop = 0; loop < NUM_WRITES; loop++) {
        // increase write count.
        write_golden_data[write_address].logical_write_count++;
        // write data to flash.
        ftl_write_data_flow_new(write_address, 1, write_golden_data[write_address].data, write_golden_data[write_address].logical_write_count);
        // Compare data
        compare_data(write_golden_data, g_host_write_range);

        if (loop == NUM_WRITES - 1 || (loop % 100 == 0 && loop != 0)) {            
            unsigned int waf_idx = 0;
            if (loop == NUM_WRITES - 1) {
                waf_idx = NUM_LOOPS - 1;
            } else { 
                waf_idx = loop/100 - 1;
            }

            // unsigned int waf_idx = 0;
            // if (loop ==  NUM_WRITES - 1) { 
            //     waf_idx = 99;
            // } else { 
            //     waf_idx = loop/1000 - 1; 
            // }

            // Compare data
            compare_data(write_golden_data, g_host_write_range);
            
            // record waf value.
            g_waf_record_data[waf_idx].loop = loop;
            g_waf_record_data[waf_idx].nand_write = g_nand_write_size;
            g_waf_record_data[waf_idx].host_write = g_host_write_size;
            g_waf_record_data[waf_idx].waf = (double)g_nand_write_size/g_host_write_size;

            printf("=======================================================================================\n");
            printf("[Loop: %u] single point write test and compare data done.\n", loop + 1);
            printf("=======================================================================================\n");
        }
    }    

    return 0;
}

int main(int argc, char **argv) {
    // Initial Parameters
    g_host_write_range = floor(HOST_WRITE_RANGE);
    int select = 0;

    do {
        // --------------------------------------------------------------------------------------------
        // Select Test Pattern.
        printf("=======================================================================================\n");
        printf("                             Start to Basic FTL Test\n");
        printf("=======================================================================================\n");
        printf("Please Select Test Pattern:\n");
        printf("    0. Exit.\n");
        printf("\n");
        printf("    1. Sequence Write. (Add a Sequence Write pattern that writes 0 to %u LCA in order for loops)\n", g_host_write_range - 1);
        printf("\n");
        printf("    2. Random Write. (Perform a pre-condition by writing all positions from 0 to %u),\n", g_host_write_range - 1);
        printf("       first in order (SEQ Write 100%%), then Random Write for loop.\n");
        printf("\n");
        printf("    3. 0 Position Write. (Add a single point write pattern.\n");
        printf("       First, perform a pre-condition by writing all positions from 0 to %u in order,\n", g_host_write_range - 1);
        printf("       then continuously write to LCA position 0 for loops.)\n");
        printf("\n");
        printf("\n");
        printf(": ");
        
        unsigned int testCase = 0;
        scanf("%d", &select);

        if (select == 0) {
            return 0;
        }

        // initial waf record.
        memset(g_waf_record_data, 0, NUM_LOOPS * sizeof(waf_records_t));

        char* test_case;
        switch (select)
        {
        case 1:        
            printf("You select the [Sequence Write] Pattern.\n");
            testCase = 1;
            test_case = "SEQ";
            break;
        case 2:
            printf("You select the [Random Write] Pattern.\n");
            testCase = 2;
            test_case = "RND";
            break;
        case 3:
            printf("You select the [0 Position Write] Pattern.\n");
            testCase = 3;
            test_case = "POS";
            break;
        default:
            printf("YOU DO NOT SELECT ANY PATTERN !!!\n");
            testCase = 0;
            break;
        }

        // There are no Test Case to Execute
        if (testCase == 0) {
            continue;
        }

        //---------------------------------------------------------------
        // Initial FTL and related flow.
        printf("----->\n");
        ftl_initialize();        
        printf("[Done] Initial FTL Table and Parameters Done.\n");
        printf("<-----\n");
        // Present Virtual Block Status
        ftl_print_vb_status();
        
        //---------------------------------------------------------------
        // Fixed LCA Length for Golden Write Data.
        host_write_data_t write_golden_data[g_host_write_range];
        initialize_data(write_golden_data, g_host_write_range);

        //---------------------------------------------------------------
        // Pre-Condition
        printf("----->\n");
        precondition_sequence_write(write_golden_data, 0, g_host_write_range);
        printf("[Done] Pre-Condition Done.\n");
        printf("<-----\n");
        // Present Virtual Block Status
        ftl_print_vb_status();
        // system("PAUSE");
        
        //---------------------------------------------------------------
        // Write Test Case
        printf("----->\n");
        int result = 0;
        switch (testCase) {
        case 1:
            result = sequence_write(write_golden_data, 0, g_host_write_range);
            if (result != 0) {
                printf("[Fail] Sequence Write Flow had Error !!!\n");
                system("pause");
            }
            break;
        case 2:
            result = random_write(write_golden_data, 0, g_host_write_range);
            if (result != 0) {
                printf("[Fail] Random Write Flow had Error !!!\n");
                system("pause");
            }        
            break;
        case 3:
            result = single_point_write(write_golden_data, 0);
            if (result != 0) {
                printf("[Fail] Point Write Flow had Error !!!\n");
                system("pause");
            }
            break;
        default:
            printf("[Info] There are NO Test have to Execute !!!\n");
            break;
        }            
        printf("[Done] Write Test Done.\n");
        printf("<-----\n");

        // Write P2L Table data to csv.
        printf("----->\n");
        ftl_write_vb_table_detail_to_csv(test_case);
        printf("[Done] Write vb data to csv done.\n");
        printf("<-----\n");

        // Write P2L Table data to csv.
        printf("----->\n");
        ftl_write_waf_record_to_csv(test_case, g_waf_record_data, NUM_LOOPS);
        printf("[Done] Write WAF data to csv done.\n");
        printf("<-----\n");    

        // Present Virtual Block Status
        ftl_print_vb_status();
                
        // Read Flash Data to compare Golden Write data.
        // allocate a temp array for rtead P2L data.
        printf("----->\n");

        printf("[In-Progress] Comparing data......\n");
        
        //Compare data.
        compare_data(write_golden_data, g_host_write_range);
                
        printf("\n\n");
        printf("-----------------------\n");
        printf("Test Summary:\n");
        // Print device info.
        printf("Block per die: %u\n", BLOCKS_PER_DIE);
        // Print device info.
        printf("Total VB (single unit) pages: %u\n", TOTAL_VB_PAGES);
        // Print device info.
        printf("Total Pages: %u\n", TOTAL_PAGES);
        // Print device info.
        printf("OP Size: %u\n", OP_SIZE);
        // Host write range.
        printf("Host write range: %u\n", g_host_write_range);
        printf("\n");
        // Total Erase Count
        printf("Total Erase Count is: %u\n", g_total_erase_count);
        // Total GC Count        
        printf("Total GC Count is: %u\n", g_total_gc_count);
        // Total Segmental GC Count
        printf("Total Segmental GC Count is: %u\n", g_segmental_gc_count);
        // Total WL Count
        printf("Total WL Count is: %u\n", g_total_wl_count);
        // WAF Value, Calc WAF
        double waf = (double)g_nand_write_size/g_host_write_size;
        printf("WAF is: %lf\n", waf);
        printf("----- End of test -----\n");
        printf("\n\n");
    
    } while (select != 0);

    return 0;
}