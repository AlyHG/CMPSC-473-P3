#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ucontext.h>
#include <stdint.h>
#include <math.h>
#include <stddef.h>

// Policy type
enum policy_type
{
    MM_FIFO = 1,  // FIFO Replacement Policy
    MM_THIRD = 2, // Third Chance Replacement Policy
};

typedef enum 
{
    RNP = 0,
    WNP = 1,
    WRONP = 2,
    RRORW = 3,
    WRORW = 4,
}Fault_Type;

enum permissions
{
    NONE = 0,
    READ = 1,
    WRITE = 2,
};

// APIs
void mm_init(enum policy_type policy, void *vm, int vm_size, int num_frames, int page_size);
void signal_handler(int sig, siginfo_t *si, void* ctx);

/*
  * virt_page   : Page number of the virtual page being referenced
  * fault_type  : Indicates what caused SIGSEGV.
  *               0 - Read access to a non-present page
  *               1 - Write access to a non-present page
  *               2 - Write access to a currently Read-only page
  *               3 - Track a "read" reference to the page that has Read and/or Write permissions on.
  *               4 - Track a "write" reference to the page that has Read-Write permissions on.
  * evicted_page: Virtual page number that is evicted. (-1 in case of no eviction)
  * write_back  : 1 indicates evicted page needs writing back to disk, 0 otherwise
  * phy_addr    : Represents the physical address (frame_number concatenated with the offset)
*/
void mm_logger(int virt_page, int fault_type, int evicted_page, int write_back, unsigned int phy_addr);

#endif
