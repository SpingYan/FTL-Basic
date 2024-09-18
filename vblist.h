#ifndef VBLIST_H
#define VBLIST_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// typedef struct struct_name {    /* No _t */
//     char* a;
//     char b;
//     char c;
// } struct_name_t;    /* _t */

// [Struct] Define linked list node struct.
typedef struct vb_node {
    unsigned int vb_index;      // Virtual Block Index
    unsigned int erase_count;   // Erase Count
    struct vb_node* next;        // Next node pointer
} vb_node_t;

// [Struct] Linked list head pointer.
typedef struct {
    vb_node_t* head;               // linked list head.
} vb_list_t;

// ------------------------------------------------------------------------------------------------
// Initialize vb list
void            vb_initialize(vb_list_t* list);
// Insert a specific Index to linked list.
void            vb_insert(vb_list_t* list, unsigned int vb_index, unsigned int erase_count);
// Insert sort a specific Index to linked list.
void            vb_insert_sort(vb_list_t* list, unsigned int vb_index, unsigned int erase_count);
// Pop top ndoe in vb List
int             vb_get(vb_list_t* list);
// Pop min erase count ndoe in vb List
int             vb_get_min_ec(vb_list_t* list);
// Pop max erase count ndoe in vb List
int             vb_get_max_ec(vb_list_t* list);

// Count vb list count.
unsigned int    vb_count(vb_list_t* list);

// Crete new node.
vb_node_t*      vb_create_node(unsigned int vb_index, unsigned int erase_count);
// Delete a specific Index on linked list.
void            vb_delete(vb_list_t* list, unsigned int vb_index);
// Search virtual block in a specific index on linked list.
vb_node_t*      vb_search(vb_list_t* list, unsigned int vb_index);
// Search virtual block in min erase count on linked list.
vb_node_t*      vb_search_min_erase_count(vb_list_t* list);
// Search the first vb.
vb_node_t*      vb_search_first(vb_list_t* list);
// Print all the vb list info.
void            vb_print_list(vb_list_t* list);

#endif //FTL_H