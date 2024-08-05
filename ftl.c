#include "ftl.h"

// Initialize FTL Table and Parameters.
void ftl_initialize() 
{
    //Initial L2P, P2L, VBStatus tables.
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++) {
        // initial L2P table data.
        g_l2p_table[i].logical_page_index = i;
        g_l2p_table[i].physical_page_address = INVALID;

        // initial P2L table data.
        g_p2l_table[i].physical_page_index = i;        
        g_p2l_table[i].logical_page_address = INVALID;
        g_p2l_table[i].data = INVALID_SHORT;
        g_p2l_table[i].logical_address_write_count = INVALID_SHORT;        
        g_p2l_table[i].physical_page_write_count = 0;                                
    }

    // Initial vb status
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        g_vb_status[i].valid_count = 0;
        g_vb_status[i].erase_count = 0;
        g_vb_status[i].program_status = IDLE;
    }

    // Initial Written VB List
    // vb_initialize(&g_written_vb_list);
    // Initial Free VB List
    vb_initialize(&g_free_vb_list);
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        vb_insert(&g_free_vb_list, i, g_vb_status[i].erase_count);
    }

    // Record total erase count for VB.
    g_total_erase_count = 0;
    // Record total gc count for all VB.
    g_total_gc_count = 0;
    // Record total wear-leveling count.
    g_total_wl_count = 0;
    // records nand write size
    g_nand_write_size = 0;
    // records host write size
    g_host_write_size = 0;
    
    // Get current writing vb from free vb list.
    g_writing_vb = vb_get(&g_free_vb_list);
    if (g_writing_vb == -1) {
        ftl_print_vb_status();
    }
    
    // Initial write vb
    g_vb_status[g_writing_vb].program_status = WRITING;
    g_remaining_pages_to_write = TOTAL_VB_PAGES;

    // gc and wl target.
    g_gc_target_vb = INVALID;
    g_wl_target_vb = INVALID;

    // Set gc, wl remaining page is 0.
    g_remaining_pages_to_gc = 0;
    g_remaining_pages_to_wl = 0;    
}

// Update L2P Mapping Table
int ftl_update_table_L2P(unsigned int logical_page, unsigned int physical_page)
{        
    if (g_l2p_table[logical_page].logical_page_index != logical_page) {
        printf("[Fail][ftl_update_table_L2P] L2P Table logical Address initial fail.\n");
        return -1;
    }

    // Determine physical page have value or not, if had g_vb_status[].ValidCount--
    if (g_l2p_table[logical_page].physical_page_address != INVALID) {
        // calc VB Index by physical page.
        unsigned int target_vb = g_l2p_table[logical_page].physical_page_address / (TOTAL_VB_PAGES);
        g_vb_status[target_vb].valid_count--;
    }
    
    g_l2p_table[logical_page].physical_page_address = physical_page;

    return 0;
}

// Update P2L Table (physical page address and logical page address and user data)
int ftl_update_table_P2L(unsigned int physical_page, unsigned int logical_page, unsigned short data, unsigned short logical_address_write_count)
{
    if (g_p2l_table[physical_page].physical_page_index != physical_page) {
        printf("[Fail][ftl_update_table_P2L] P2L Table Physical Address initial fail.\n");
        return -1;
    }

    g_p2l_table[physical_page].logical_page_address = logical_page;
    g_p2l_table[physical_page].data = data;
    g_p2l_table[physical_page].logical_address_write_count = logical_address_write_count;
    g_p2l_table[physical_page].physical_page_write_count++;

    // Increase valid count
    unsigned int target_vb = physical_page / (TOTAL_VB_PAGES);
    g_vb_status[target_vb].valid_count++;
    // Increse nand write count.
    g_nand_write_size++;

    return 0;
}

// Erase vb: P2L's logical page address, data, logical_address_write_count assign to INVALID data.
void ftl_erase_vb(unsigned int vb_index) 
{        
    unsigned int pageidx = vb_index * TOTAL_VB_PAGES;
    for (unsigned int i = pageidx; i < pageidx + (TOTAL_VB_PAGES); i++) {
        g_p2l_table[i].logical_page_address = INVALID;
        g_p2l_table[i].data = INVALID_SHORT;
        g_p2l_table[i].logical_address_write_count = INVALID_SHORT;    
    }
    
    g_vb_status[vb_index].erase_count++;
    g_vb_status[vb_index].program_status = IDLE; // set written flag.
    // Insert to free vb.
    vb_insert(&g_free_vb_list, vb_index, g_vb_status[vb_index].erase_count);

    // Increase Total Erase Count.
    g_total_erase_count++;

    return;
}

// Scan if there are have 0 valid VB.
int ftl_scan_written_vb_to_erase()
{    
    unsigned int can_erase_vb = 0;
    // calc how much vb can erase
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        if (g_vb_status[i].program_status == WRITTEN && g_vb_status[i].valid_count == 0) {            
            ftl_erase_vb(i);
            can_erase_vb++;
        }
    }

    return can_erase_vb;
}

unsigned int ftl_calc_min_erase_count()
{
    unsigned int min_ex = 0;
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        if (g_vb_status[i].erase_count < min_ex) {
            min_ex = g_vb_status[i].erase_count;
        }        
    }

    return min_ex;
}

unsigned int ftl_calc_max_erase_count()
{
    unsigned int max_ex = 0;
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        if (g_vb_status[i].erase_count > max_ex) {
            max_ex = g_vb_status[i].erase_count;
        }        
    }

    return max_ex;
}

double ftl_calc_avg_erase_count()
{
    double avg_ex = (double)g_total_erase_count / BLOCKS_PER_DIE;
    return avg_ex;
}

int ftl_determine_wear_leveling()
{
    // Detrermine wear-leveling condition.
    double avg_ec = ftl_calc_avg_erase_count();
    
    unsigned int max_ex = 0;    
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        if (g_vb_status[i].program_status == WRITTEN) {
            if (g_vb_status[i].erase_count > max_ex) {
                max_ex = g_vb_status[i].erase_count;
            }
        }
    }
    
    if (max_ex > avg_ec + 5) {
        return 1;
    }        
    
    return 0;
}

// Write Data to Flash
int ftl_write_data_flow(unsigned int logical_page, unsigned int length, unsigned short data, unsigned short write_count)
{            
    if (g_remaining_pages_to_write == 0) {
        // set vb status to WRITTEN.
        g_vb_status[g_writing_vb].program_status = WRITTEN;
        
        // ----------------------------------------------------------------------------------------
        // Add scan in-valid pages to table erase.
        printf ("[scan_written_vb_to_erase] Scan invalid vb to erase......\n");
        unsigned int can_erase_after_scan_vb = ftl_scan_written_vb_to_erase();
        if (can_erase_after_scan_vb > 0) {
            printf ("[scan_written_vb_to_erase] There are %u vb be updated to erase done.\n", can_erase_after_scan_vb);
        } else {
            printf ("[scan_written_vb_to_erase] There are no vb can be updated to erase.\n");
        }        

        // ----------------------------------------------------------------------------------------                
        // If Free VB Counts < 4, do GC flow.
        // Do gc for free vb is > minimum_limit_free_vb.
        // if (vb_count(&g_free_vb_list) <= minimum_limit_free_vb) {
        while (vb_count(&g_free_vb_list) <= GC_TRIGGER_VB_COUNT) {
            printf("[GC: %u][In-Progress][garbage_collection] Prepare do common garbage-collecting......\n", g_total_gc_count + 1);
            ftl_garbage_collection_new(0);
            printf("[GC: %u][Done][garbage_collection] do common garbage collecting done\n\n", g_total_gc_count + 1);
            // Increase gc counts.
            g_total_gc_count++;
        }

        // Add wear leveling condition, check every 100 times.
        // Do while or every total erase count more than 10 time.
        // if (g_total_erase_count % 5 == 0 && g_total_erase_count != 0) {
        if (ftl_determine_wear_leveling() == 1) {
            printf("[WL: %u][In-Progress][wear_leveling] Prepare do wear-leveling......\n", g_total_wl_count + 1);
            ftl_garbage_collection_new(1);
            printf("[WL: %u][Done][wear_leveling] do wear-leveling done\n\n", g_total_wl_count + 1);
            // Increase wear-leveling counts.
            g_total_wl_count++;
        }
        //}
                        
        // ----------------------------------------------------------------------------------------
        // Get new free vb to write.
        if (ftl_get_new_vb_to_write() < 0) {                      
            ftl_print_vb_status();
            printf("[Fail][ftl_get_new_vb_to_write] Get write new vb fail !!!\n");
            return -1;
        }
    }

    // set physical page
    unsigned int writting_physical_page = (g_writing_vb * TOTAL_VB_PAGES) + (TOTAL_VB_PAGES - g_remaining_pages_to_write);

    // Update L2P Table    
    if (ftl_update_table_L2P(logical_page, writting_physical_page) != 0) {
        printf("[Fail][ftl_update_table_L2P] fail in logical_page: %u !!!\n", logical_page);
        system("pause");
        return -1;
    }

    // Update P2L Table
    if (ftl_update_table_P2L(writting_physical_page, logical_page, data, write_count)) {
        printf("[Fail][ftl_update_table_P2L] fail in writting_physical_page: %u !!!\n", writting_physical_page);
        system("pause");
        return -1;
    }
   
    // Calc remaining Write Pages.
    g_remaining_pages_to_write = g_remaining_pages_to_write - length;
    if (g_remaining_pages_to_write > TOTAL_VB_PAGES) {
        printf("[ASSERT] g_remaining_pages_to_write > %u in writting_physical_page: %u !!!!!\n", TOTAL_VB_PAGES, writting_physical_page);
        system("pause");
        return -1;
    }

    // increase host write count.
    g_host_write_size++;

    return 0;
}

// When writing vb is empty, get new vb to write.
int ftl_get_new_vb_to_write()
{    
    // Update write target VB and Free Page Count.
    g_writing_vb = vb_get(&g_free_vb_list);
    if (g_writing_vb == -1)
    {
        printf("[Fail][ftl_get_new_vb_to_write] Can't find any free vb to write !!!\n");
        ftl_print_vb_status();
        return -1;
    }

    g_vb_status[g_writing_vb].program_status = WRITING;
    g_remaining_pages_to_write = TOTAL_VB_PAGES;    
    
    return 0;
}

// When writing vb is empty, get new vb to GC.
int ftl_get_new_vb_to_gc()
{    
    // Update gc target VB and do gc pages count.
    g_gc_target_vb = vb_get(&g_free_vb_list);
    if (g_gc_target_vb == -1)
    {
        printf("[Fail][ftl_get_new_vb_to_gc] Can't find any free vb to GC !!!\n");
        ftl_print_vb_status();
        return -1;
    }

    g_vb_status[g_gc_target_vb].program_status = GC_TARGET;
    g_remaining_pages_to_gc = TOTAL_VB_PAGES;
    
    return 0;
}

// Get new vb to do wear-leveling.
int ftl_get_new_vb_to_wl()
{    
    // Update gc target VB and do gc pages count.
    // sort linked list by erase count.
    vb_bubble_sort(&g_free_vb_list);

    g_wl_target_vb = vb_get(&g_free_vb_list);
    if (g_wl_target_vb == -1)
    {
        printf("[Fail][ftl_get_new_vb_to_wl] Can't find any free vb to do Wear-Leveling !!!\n");
        ftl_print_vb_status();
        return -1;
    }

    g_vb_status[g_wl_target_vb].program_status = WL_TARGET;
    g_remaining_pages_to_wl = TOTAL_VB_PAGES;
    
    return 0;
}

// search all vb to get min erase count vb.
unsigned int ftl_get_min_erase_count_vb()
{
    // (Wear-Leveling) Minimal Erase count on vb, default is INVALID
    unsigned int min_erase_count = INVALID;
    unsigned int tmp_vb = INVALID;
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        // Search written vb
        if (g_vb_status[i].program_status == WRITTEN) {
            if (g_vb_status[i].erase_count < min_erase_count) {
                min_erase_count = g_vb_status[i].erase_count;
                tmp_vb = i;
            }
        }
    }

    return tmp_vb;
}

// search all vb to get min valid count vb.
unsigned int ftl_get_min_valid_count_vb()
{
    // (for Common GC) Minimal valid count on vb, default is vb's pages
    unsigned int min_valid_count = TOTAL_VB_PAGES;
    unsigned int tmp_vb = INVALID;
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        // Search written vb
        if (g_vb_status[i].program_status == WRITTEN) {
            if (g_vb_status[i].valid_count < min_valid_count) {
                min_valid_count = g_vb_status[i].valid_count;
                tmp_vb = i;
            }
        }
    }
    
    return tmp_vb;
}

// GC Purpose: Find Next Free VB to write.
// 0. Wear-leveling
// > 0, more than 0, mean do gc for minimal free gc for used.
int ftl_garbage_collection_new(int wear_leveling)
{
    // --------------------------------------------------------------------------------------------
    // Initial gc parameters
    // --------------------------------------------------------------------------------------------
    // Declare gc vb list.
    vb_list_t source_vb_list;
    vb_initialize(&source_vb_list);
    
    // flag for get new vb to gc
    unsigned char need_get_vb = 0;    

    // --------------------------------------------------------------------------------------------
    // Find source vbs and calc remianing gc pages.
    if (wear_leveling == 1) 
    {        
        // total do gc valid count
        unsigned int valid_pages_prepare_to_move = 0;
        unsigned int first_valid_page_count = 0;

        // get min erase count vb.
        unsigned int tmp_current_wl_source_vb = ftl_get_min_erase_count_vb();
        if (tmp_current_wl_source_vb == INVALID) {
            ftl_print_vb_status();
            printf("[Fail][ftl_get_min_erase_count_vb] Get min erase count vb fail !!!\n");
            return -1;
        }

        // determine get new vb flag or not.
        first_valid_page_count = g_vb_status[tmp_current_wl_source_vb].valid_count;
        if (first_valid_page_count > g_remaining_pages_to_wl || g_remaining_pages_to_wl == 0) {
            need_get_vb = 1;
        }

        do {
            // Get min valid count vb.
            unsigned int tmp_current_wl_source_vb = ftl_get_min_erase_count_vb();
            if (tmp_current_wl_source_vb == INVALID) {
                ftl_print_vb_status();
                printf("[Fail][ftl_get_min_erase_count_vb] Get min valid count vb fail !!!\n"); 
                return -1;
            }            
            
            valid_pages_prepare_to_move += g_vb_status[tmp_current_wl_source_vb].valid_count;
            // check total gc pages more than remaining pages and one new vb size or not.
            if (need_get_vb) {
                if (valid_pages_prepare_to_move > g_remaining_pages_to_wl + TOTAL_VB_PAGES) { break; }
            } else {
                if (valid_pages_prepare_to_move > g_remaining_pages_to_wl) { break; }
            }
            
            g_vb_status[tmp_current_wl_source_vb].program_status = WL_ING;
            vb_insert(&source_vb_list, tmp_current_wl_source_vb, g_vb_status[tmp_current_wl_source_vb].erase_count);

        } while (valid_pages_prepare_to_move < g_remaining_pages_to_wl + TOTAL_VB_PAGES); // remaining pages and one new vb size.
    }    
    else
    {
        // total do gc valid count
        unsigned int valid_pages_prepare_to_move = 0;
        unsigned int first_valid_page_count = 0;

        // Get min valid count vb.
        unsigned int tmp_current_gc_source_vb = ftl_get_min_valid_count_vb();
        if (tmp_current_gc_source_vb == INVALID) {
            ftl_print_vb_status();
            printf("[Fail][ftl_get_min_valid_count_vb] Get min valid count vb fail !!!\n");
            return -1;
        }

        // determine get new vb flag or not.
        first_valid_page_count = g_vb_status[tmp_current_gc_source_vb].valid_count;
        if (first_valid_page_count > g_remaining_pages_to_gc || g_remaining_pages_to_gc == 0) {
            need_get_vb = 1;
        }

        do {
            // Get min valid count vb.
            unsigned int tmp_current_gc_source_vb = ftl_get_min_valid_count_vb();
            if (tmp_current_gc_source_vb == INVALID) {
                ftl_print_vb_status();
                printf("[Fail][ftl_get_min_valid_count_vb] Get min valid count vb fail !!!\n"); 
                return -1;
            }            
            
            valid_pages_prepare_to_move += g_vb_status[tmp_current_gc_source_vb].valid_count;
            // check total gc pages more than remaining pages and one new vb size or not.
            if (need_get_vb) {
                if (valid_pages_prepare_to_move > g_remaining_pages_to_gc + TOTAL_VB_PAGES) { break; }
            } else {
                if (valid_pages_prepare_to_move > g_remaining_pages_to_gc) { break; }
            }
            
            g_vb_status[tmp_current_gc_source_vb].program_status = GC_ING;
            vb_insert(&source_vb_list, tmp_current_gc_source_vb, g_vb_status[tmp_current_gc_source_vb].erase_count);

        } while (valid_pages_prepare_to_move < g_remaining_pages_to_gc + TOTAL_VB_PAGES); // remaining pages and one new vb size.
    }    

    // Determine page is valid or not -> According P2L's logical address is same to L2P's logical page and physical's value.
    unsigned int src_vb_count = vb_count(&source_vb_list);
    for (unsigned int tmp_vb = 0; tmp_vb < src_vb_count; tmp_vb++)
    {        
        unsigned int tmp_current_src_vb = vb_get(&source_vb_list);
        if (tmp_current_src_vb == -1) {
            ftl_print_vb_status();
            printf("[Fail] Get process source vb fail !!!\n");
            return -1;
        }

        unsigned int tmp_src_page_idx = tmp_current_src_vb * TOTAL_VB_PAGES;

        for (unsigned int i = 0; i < TOTAL_VB_PAGES; i++) 
        {
            unsigned int tmp_logical_idx = g_p2l_table[tmp_src_page_idx + i].logical_page_address;
            unsigned int tmp_physical_idx = g_l2p_table[tmp_logical_idx].physical_page_address;
            
            // Detrermine is valid date or not，P2L mapping's logical addres is eual L2P's physical or not.
            if (tmp_physical_idx == (tmp_src_page_idx + i)) // valid page
            {
                if (wear_leveling == 1)
                {
                    // When remaining pages is 0, get new vb to wl.
                    if (g_remaining_pages_to_wl == 0 && need_get_vb == 1) {
                        if (g_wl_target_vb != INVALID) {
                            g_vb_status[g_wl_target_vb].program_status = WRITTEN;
                        }                        
                        
                        if (ftl_get_new_vb_to_wl() < 0) {
                            ftl_print_vb_status();
                            printf("[Fail][ftl_get_new_vb_to_wl] Get new wl target vb fail !!!\n");
                            return -1;
                        }
                    }

                    // move valid page to target page.
                    ftl_process_page(tmp_physical_idx, g_wl_target_vb, &g_remaining_pages_to_wl);

                } else { // GC flow
                    // When remaining pages is 0, get new vb to gc.
                    if (g_remaining_pages_to_gc == 0 && need_get_vb == 1) {
                        if (g_gc_target_vb != INVALID) {
                            g_vb_status[g_gc_target_vb].program_status = WRITTEN;
                        }
                        
                        if (ftl_get_new_vb_to_gc() < 0) {
                            ftl_print_vb_status();
                            printf("[Fail][ftl_get_new_vb_to_gc] Get new gc target vb fail !!!\n");
                            return -1;
                        }
                    }

                    // move valid page to target page.
                    ftl_process_page(tmp_physical_idx, g_gc_target_vb, &g_remaining_pages_to_gc);

                }                                
            }
        }

        // --------------------------------------------------------------------------------------------
        // 4. Erase this VB.
        ftl_erase_vb(tmp_current_src_vb);
        if (wear_leveling == 1) {
            printf("[Done] Do wl flow done and release vb: %u\n", tmp_current_src_vb);
        } else {
            printf("[Done] Do gc flow done and release vb: %u\n", tmp_current_src_vb);
        }                
    }    

    return 0;
}

// Read Data from Flash
unsigned short ftl_read_data_flow(unsigned int logical_page, unsigned int length) {
    unsigned int ppa = g_l2p_table[logical_page].physical_page_address;
    return g_p2l_table[ppa].data;
}

// Read Lca Write Counts from Flash
unsigned short ftl_read_page_write_count(unsigned int logical_page) {
    unsigned int ppa = g_l2p_table[logical_page].physical_page_address;
    return g_p2l_table[ppa].logical_address_write_count;
}

// Display Virtual Block's Valid Count and Erase Count.
void ftl_print_vb_status()
{
    g_max_erase_count = ftl_calc_max_erase_count();    
    g_min_erase_count = ftl_calc_min_erase_count();
    g_avg_erase_count = ftl_calc_avg_erase_count();

    printf("==============================================================\n");
    printf("                   Virtual Block Status\n");
    printf("==============================================================\n");

    printf("Summary: \n");
    printf("Total Erase Count: %u\n", g_total_erase_count);
    printf("Max Erase Count: %u\n", g_max_erase_count);
    printf("Min Erase Count: %u\n", g_min_erase_count);
    printf("Avg Erase Count: %lf\n", g_avg_erase_count);
    printf("Total GC Count: %u\n", g_total_gc_count);
    printf("Total WL Count: %u\n", g_total_wl_count);
    printf("\n");

    static char* enum_status[] = {"IDLE", "WRITING", "WRITTEN", "GC_ING", "GC_TARGET", "WL_ING", "WL_TARGET"};
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        printf("VB: %u, Valid Count = %u, Erase Count = %u, Status = %s\n", \
        i, g_vb_status[i].valid_count, g_vb_status[i].erase_count, enum_status[g_vb_status[i].program_status]);
    }
    
    printf("==============================================================\n");
}

int ftl_write_waf_record_to_csv(char* test_case, waf_records_t* record, unsigned int size)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "./WAF_%s_OP_%02d.csv", test_case, OP_SIZE);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

     // Write excel title.
    fprintf(file, "Loop,Nand Write,Host Write,WAF\n");

    // Write down row data.
    for (unsigned int i = 0; i < size; i++)
    {
        fprintf(file, "%u,%u,%u,%lf",
                record[i].loop,
                record[i].nand_write,
                record[i].host_write,
                record[i].waf                
            );
        fprintf(file, "\n");
    }

    fclose(file);

    return 0;
}

int ftl_write_vb_table_detail_to_csv(char* test_case)
{
    // Format file name reference date and time.
    time_t now = time(NULL); // Get current time
    struct tm *t = localtime(&now); // localization time
    char filename[64];    
    snprintf(filename, sizeof(filename), "./P2L-Table_%s_OP_%02d_%04d%02d%02d_%02d%02d%02d.csv",
                test_case, OP_SIZE,
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec
            );

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

    // Determine valid count or not.
    unsigned int total_pages = TOTAL_BLOCKS * PAGES_PER_BLOCK;// sizeof(g_p2l_table) / sizeof(g_p2l_table[0]);
    unsigned int* pages_valid_status = (unsigned int*)malloc(total_pages * sizeof(unsigned int));
    memset(pages_valid_status, 0, total_pages);

    unsigned int total_valid_pages = 0;    
    for (unsigned int i = 0; i < total_pages; i++) 
    {
        unsigned int tmp_logical_from_phy = g_p2l_table[i].logical_page_address;
        if (tmp_logical_from_phy != INVALID) {
            unsigned int tmp_physical_from_log = g_l2p_table[tmp_logical_from_phy].physical_page_address;
            if (i == tmp_physical_from_log) {
                pages_valid_status[i] = 1;
                total_valid_pages++;
            } 
            else {
                pages_valid_status[i] = 0;
            }
        }
        else {                
            pages_valid_status[i] = 0;            
        }
    }        

    // Write Summary
    g_max_erase_count = ftl_calc_max_erase_count();    
    g_min_erase_count = ftl_calc_min_erase_count();
    g_avg_erase_count = ftl_calc_avg_erase_count();

    fprintf(file, "Test Summary\n"); // Summary
    fprintf(file, "Block per die: ,%u\n", BLOCKS_PER_DIE);
    fprintf(file, "Total VB (single unit) pages: ,%u\n", TOTAL_VB_PAGES);
    fprintf(file, "Total Pages: ,%u\n", TOTAL_PAGES);
    fprintf(file, "OP Size: ,%u\n", OP_SIZE);
    fprintf(file, "Host write range: ,%u\n", g_host_write_range);
    fprintf(file, "Total Erase Count is: ,%u\n", g_total_erase_count);

    fprintf(file, "Max Erase Count: ,%u\n", g_max_erase_count);
    fprintf(file, "Min Erase Count: ,%u\n", g_min_erase_count);
    fprintf(file, "Avg Erase Count: ,%lf\n", g_avg_erase_count);

    fprintf(file, "Total GC Count is: ,%u\n", g_total_gc_count);
    fprintf(file, "Total WL Count is: ,%u\n", g_total_wl_count);

    // WAF Value, Calc WAF
    double waf = (double)g_nand_write_size/g_host_write_size;
    fprintf(file, "WAF is: ,%lf\n\n", waf);            

    fprintf(file, "VB No,Valid Count,Erase Count\n");
    for (unsigned int i = 0; i < BLOCKS_PER_DIE; i++) {
        fprintf(file, "%u,%u,%u", i, g_vb_status[i].valid_count, g_vb_status[i].erase_count);
        fprintf(file, "\n");
    }
        
    fprintf(file, "\n\n");
    
    // Write excel title.
    fprintf(file, "Physical Page Address,Logical Page Address,Data,LCA_Counts,Nand_WriteCounts,Valid_Page\n");

    // Write down row data.
    for (unsigned int i = 0; i < TOTAL_BLOCKS * PAGES_PER_BLOCK; i++)
    {
        fprintf(file, "%u,%u,%hu,%hu,%hu,%hu",
                g_p2l_table[i].physical_page_index,
                g_p2l_table[i].logical_page_address,
                g_p2l_table[i].data, 
                g_p2l_table[i].logical_address_write_count,
                g_p2l_table[i].physical_page_write_count,
                pages_valid_status[i]
            );
        fprintf(file, "\n");
    }
    
    fclose(file);
    free(pages_valid_status);

    return 0;
}

int ftl_compare_write_data(host_write_data_t* write_golden_data, unsigned int data_size)
{
    int compare_result = 0;
    unsigned short* temp_data = (unsigned short*)malloc(data_size * sizeof(unsigned short));
    unsigned short* temp_write_count = (unsigned short*)malloc(data_size * sizeof(unsigned short));
    unsigned int i = 0;    
    // --------------------------------------------------------------------------------------------
    // Read data from flash to temp buffer.
    for (i; i < data_size; i++) {
        temp_data[i] = ftl_read_data_flow(i, 1);
        temp_write_count[i] = ftl_read_page_write_count(i);
    }

    // --------------------------------------------------------------------------------------------
    // Compare to golden buffer table.
    for (i; i < data_size; i++) {
        if  (temp_data[i] != write_golden_data[i].data) {
            printf("[Fail] Data Compare Fail in LCA: %u, Data Read: %hu, Golden Data: %hu\n", \
            i, temp_data[i], write_golden_data[i].data);
            compare_result = -1;
        }

        if  (temp_write_count[i] != write_golden_data[i].logical_write_count) {
            printf("[Fail] Write Counts Compare Fail in LCA: %u, Data Read: %hu, Golden Data: %hu\n", \
            i, temp_write_count[i], write_golden_data[i].logical_write_count);            
            compare_result = -2;
        }
    }

    free(temp_data);
    free(temp_write_count);

    return compare_result;
}

// move valid page to target page for wl or gc.
void ftl_process_page(unsigned int src_page_idx, unsigned int target_vb, unsigned int* remaining_pages) 
{
    unsigned int target_page_idx = (target_vb * TOTAL_VB_PAGES) + (TOTAL_VB_PAGES - *remaining_pages);

    unsigned int logical_idx = g_p2l_table[src_page_idx].logical_page_address;
    unsigned int physical_idx = g_l2p_table[logical_idx].physical_page_address;

    g_p2l_table[target_page_idx].logical_page_address = logical_idx;
    g_p2l_table[target_page_idx].data = g_p2l_table[physical_idx].data;
    g_p2l_table[target_page_idx].logical_address_write_count = g_p2l_table[physical_idx].logical_address_write_count;
    g_p2l_table[target_page_idx].physical_page_write_count++;

    g_l2p_table[logical_idx].physical_page_address = target_page_idx;
    g_vb_status[target_vb].valid_count++;
    g_nand_write_size++;
    g_vb_status[src_page_idx / TOTAL_VB_PAGES].valid_count--;
    (*remaining_pages)--;

    return;
}