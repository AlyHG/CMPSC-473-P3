#ifndef VMM_H
#define VMM_H

#include "interface.h"

// Declare your own data structures and functions here...

typedef struct page {       /* Node Data Structure for Pages in Memory */        
    int page_num;           /* Page Number */
    long page_start;
    int write_back;
    int frame_num;
    int tcr;
    int modified;
    unsigned int phys_addr;
    enum permissions permission;
    struct page* next;     
}page;

typedef struct Memory {     /* Linked List Data Structure for Memory */
    int num_frames;           
    page* head;
    int current_frame;
}Memory;

typedef struct vmm {
    enum policy_type policy;
    long start;
    int mem_size;
    int num_frames; 
    long page_size;
    int input_count;
    page* prev_page;
}VMM;

VMM* vmm;
Memory* memory;


Memory* init_memory();
page* find_page(Memory* memory, long start_address);
page* enqueue(Memory* memory);
void evict(Memory* memory, page* target);
page* evict_tcr(Memory* memory);

#endif
