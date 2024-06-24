#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "freeVB.h"

//=====================================================================================================================
// [Linked List] Manage for Free VB Pool
//=====================================================================================================================

//-----------------------------------------------
// Initialize Free VB List
void initializeFreeVBList(FreeVBList* list) {
    list->head = NULL;
} 

//-----------------------------------------------
// Crete new node.
FreeVBNode* createNode(unsigned int vbIndex, unsigned int eraseCount) {
    FreeVBNode* newNode = (FreeVBNode*)malloc(sizeof(FreeVBNode));
    if (newNode == NULL) {
        printf("[Fail] Memory allocation failed in createNode vbIndex is: %u \n", vbIndex);
        exit(1);
    }

    newNode->vbIndex = vbIndex;
    newNode->eraseCount = eraseCount;    
    newNode->next = NULL;

    return newNode;
}

//-----------------------------------------------
// Insert a specific Index to linked list.
void insertFreeVB(FreeVBList* list, unsigned int vbIndex, unsigned int eraseCount) {
    FreeVBNode* newNode = createNode(vbIndex, eraseCount);
    newNode->next = NULL;
    if (list->head == NULL) {
        list->head = newNode;
    }
    else {
        FreeVBNode* current = list->head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newNode;        
    }
}

//-----------------------------------------------
// Delete a specific Index on linked list.
void deleteFreeVB(FreeVBList* list, unsigned int vbIndex) {
    FreeVBNode* target = searchFreeVB(list, vbIndex);
    FreeVBNode* current = list->head;
    if (current == NULL) {
        printf("List is empty, Nothing to delete!\n", vbIndex);
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
// Pop top free ndoe in VB List
int getFreeVB(FreeVBList* list) {
    FreeVBNode* top = list->head;
    if (top == NULL) {
        printf("[Fail] The VB List is empty !!!");
        return -1;
    }

    int vbIndex = top->vbIndex;
    list->head = top->next;
    free(top);
    
    return vbIndex;
}

//-----------------------------------------------
// Search free block in a specific Index on linked list.
FreeVBNode* searchFreeVB(FreeVBList* list, unsigned int vbIndex) {
    FreeVBNode* current = list->head;
    while (current != NULL) {
        if (current->vbIndex == vbIndex) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//-----------------------------------------------
// Search free block in Min Erase Count on linked list.
FreeVBNode* searchMinEraseCountFreeVB(FreeVBList* list) {
    FreeVBNode* current = list->head;
    FreeVBNode* minEraseNode = NULL;

    if (current == NULL) {
        return NULL; // Free List VB is empty. 
    }

    minEraseNode = current; // Assign to head node.

    while (current != NULL) {
        if (current->eraseCount < minEraseNode->eraseCount) {
            minEraseNode = current;
        }
        current = current->next;
    }

    return minEraseNode;
}

//-----------------------------------------------
// Count Free VB List count.
unsigned int countFreeVBNodes(FreeVBList* list) {
    unsigned int count = 0;
    FreeVBNode* current = list->head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

//-----------------------------------------------
// Search the first Free VB.
FreeVBNode* searchFirstFreeVB(FreeVBList* list) {
    return list->head;
}

//-----------------------------------------------
// Print all the Free VB List info.
void printFreeVBList(FreeVBList* list) {
    FreeVBNode* current = list->head;
    while (current != NULL) {
        printf("VBIndex: %u, EraseCount: %u\n", current->vbIndex, current->eraseCount);
        current = current->next;
    }
}