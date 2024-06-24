#ifndef FREEVB_H
#define FREEVB_H

// [Struct] Define Linked List node struct.
typedef struct FreeVBNode {
    unsigned int vbIndex;      // Virtual Block Index
    unsigned int eraseCount;   // Erase Count
    struct FreeVBNode* next;   // Next node pointer
} FreeVBNode;

// [Struct] Linked List head pointer.
typedef struct {
    FreeVBNode* head;          // linked list head.
} FreeVBList;

// Initialize Free VB List
void initializeFreeVBList(FreeVBList* list);
// Crete new node.
FreeVBNode* createNode(unsigned int vbIndex, unsigned int eraseCount);
// Insert a specific Index to linked list.
void insertFreeVB(FreeVBList* list, unsigned int vbIndex, unsigned int eraseCount);
// Delete a specific Index on linked list.
void deleteFreeVB(FreeVBList* list, unsigned int vbIndex);
// Pop top free ndoe in VB List
int getFreeVB(FreeVBList* list);
// Search free block in a specific Index on linked list.
FreeVBNode* searchFreeVB(FreeVBList* list, unsigned int vbIndex);
// Search free block in Min Erase Count on linked list.
FreeVBNode* searchMinEraseCountFreeVB(FreeVBList* list);
// Search the first Free VB.
FreeVBNode* searchFirstFreeVB(FreeVBList* list);
// Count Free VB List count.
unsigned int countFreeVBNodes(FreeVBList* list);
// Print all the Free VB List info.
void printFreeVBList(FreeVBList* list);

#endif //FTL_H