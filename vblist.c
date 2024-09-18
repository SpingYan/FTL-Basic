#include "vblist.h"

//=====================================================================================================================
// [Linked List] Manage for VB Pool
//=====================================================================================================================

//-----------------------------------------------
// Initialize VB List
void vb_initialize(vb_list_t* list) 
{
    list->head = NULL;
} 

//-----------------------------------------------
// Crete new node.
vb_node_t* vb_create_node(unsigned int vb_index, unsigned int erase_count) 
{
    vb_node_t* newnode = (vb_node_t*)malloc(sizeof(vb_node_t));
    if (newnode == NULL) {
        printf("[Fail] Memory allocation failed in vb_create_node, vb_index is: %u \n", vb_index);
        exit(1);
    }

    newnode->vb_index = vb_index;
    newnode->erase_count = erase_count;    
    newnode->next = NULL;

    return newnode;
}

//-----------------------------------------------
// Insert a specific Index to linked list.
void vb_insert(vb_list_t* list, unsigned int vb_index, unsigned int erase_count) 
{
    vb_node_t* newnode = vb_create_node(vb_index, erase_count);
    newnode->next = NULL;
    if (list->head == NULL) {
        list->head = newnode;
    }
    else {
        vb_node_t* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newnode;
    }
}

//-----------------------------------------------
// Insert sort a specific Index to linked list.
void vb_insert_sort(vb_list_t* list, unsigned int vb_index, unsigned int erase_count)
{
    vb_node_t* newnode = vb_create_node(vb_index, erase_count);
    newnode->next = NULL;
    if (list->head == NULL) {
        list->head = newnode;
        return;
    }
    
    vb_node_t* current = list->head;
    vb_node_t* prev = NULL;

    while (current != NULL && current->erase_count <= erase_count) {
        prev = current;
        current = current->next;
    }

    //insert node
    newnode->next = current;

    if (prev == NULL) {
        list->head = newnode;
    } else {
        prev->next = newnode;
    }

    return;
}

//-----------------------------------------------
// Delete a specific Index on linked list.
void vb_delete(vb_list_t* list, unsigned int vb_index) 
{
    vb_node_t* target = vb_search(list, vb_index);
    vb_node_t* current = list->head;
    if (current == NULL) {
        printf("List is empty, Nothing to delete!\n");
        return;
    }
    
    if (target == list->head) {
        list->head = list->head->next;
        free(target);
        return;
    } else {
        while (current->next != target) {
            current = current->next;            
        }
        current->next = target->next;
        free(target);
        return;
    }    
}

//-----------------------------------------------
// Pop top ndoe in vb List
int vb_get(vb_list_t* list) 
{
    vb_node_t* top = list->head;
    if (top == NULL) {
        printf("[Fail] The vb list is empty !!!\n");
        return -1;
    }

    int vb_index = top->vb_index;
    list->head = top->next;
    free(top);
    
    return vb_index;
}

// Pop min erase count ndoe in vb List
int vb_get_min_ec(vb_list_t* list)
{
    vb_node_t* top = list->head;
    if (top == NULL) {
        printf("[Fail] The vb list is empty !!!\n");
        return -1;
    }

    int vb_index = top->vb_index;
    list->head = top->next;
    free(top);
    
    return vb_index;
}

// Pop max erase count ndoe in vb List
int vb_get_max_ec(vb_list_t* list)
{
    if (list->head == NULL) {
        printf("[Fail] The VB List is empty !!!\n");
        return -1;
    }

    vb_node_t* current = list->head;
    vb_node_t* prev = NULL;

    // scan all the list.
    while (current->next != NULL) {
        prev = current;
        current = current->next;
    }

    int vbIndex = current->vb_index;

    // remove last node.
    if (prev == NULL) {
        list->head = NULL;
    } else {
        prev->next = NULL;
    }
    
    free(current);
    
    return vbIndex;
}

//-----------------------------------------------
// Search block in a specific Index on linked list.
vb_node_t* vb_search(vb_list_t* list, unsigned int vb_index) 
{
    vb_node_t* current = list->head;
    while (current != NULL) {
        if (current->vb_index == vb_index) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//-----------------------------------------------
// Search block in Min Erase Count on linked list.
vb_node_t* vb_search_min_erase_count(vb_list_t* list) 
{
    vb_node_t* current = list->head;
    vb_node_t* min_erase_ndoe = NULL;

    if (current == NULL) {
        return NULL; // VB list is empty. 
    }

    min_erase_ndoe = current; // Default assign to head node.

    while (current != NULL) {
        if (current->erase_count < min_erase_ndoe->erase_count) {
            min_erase_ndoe = current;
        }
        current = current->next;
    }

    return min_erase_ndoe;
}

//-----------------------------------------------
// Count VB List count.
unsigned int vb_count(vb_list_t* list) 
{
    unsigned int count = 0;
    vb_node_t* current = list->head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

//-----------------------------------------------
// Search the first VB.
vb_node_t* vb_search_first(vb_list_t* list) 
{
    return list->head;
}

//-----------------------------------------------
// Print all the VB List info.
void vb_print_list(vb_list_t* list) 
{
    vb_node_t* current = list->head;
    while (current != NULL) {
        printf("vb_index: %u, erase_count: %u\n", current->vb_index, current->erase_count);
        current = current->next;
    }
}