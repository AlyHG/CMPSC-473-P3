#include "interface.h"
#include "vmm.h"


// Interface implementation
// Implement APIs here...

void mm_init(enum policy_type policy, void *vm, int vm_size, int num_frames, int page_size)
{
   //Intializing Data Structures and Global Variables
    vmm = (VMM*)malloc(sizeof(VMM));
    vmm->policy = policy;
    vmm->start = (uintptr_t) vm;
    vmm->mem_size = vm_size;
    vmm->num_frames = num_frames;
    vmm->page_size = page_size;
    vmm->prev_page = NULL;

    memory = init_memory();

    // Instantiating Signal Handler as per Linux Man Pages
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sigemptyset(&(sa.sa_mask));        // Clearing bit array associated with Signal Types
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    //Protecting Memory from 
    mprotect(vm, vm_size, PROT_NONE);
}

void signal_handler(int sig, siginfo_t *si, void* ctx) 
{
    long addr = (uintptr_t) si->si_addr;
    ucontext_t* uctx = (ucontext_t*) ctx;
    int page_num = (addr - vmm->start) / vmm->page_size;
    
    ptrdiff_t offset1 = addr - vmm->start;
    offset1 %= vmm->page_size;
    int offset = (int) offset1;
   

    long page_start = vmm->start + (page_num * vmm->page_size);
    bool isPresent = false;
   
    Fault_Type fault_type;
    int evicted_page = -1;
    int write_back = 0;

    // Check if page is in memory
    page* new_page = find_page(memory, page_start); 
    page* evicted = NULL;
    unsigned int phys_addr;

    if(new_page != NULL){
        isPresent = true;
    }
    

    // if((vmm->policy == MM_THIRD) && (vmm->prev_page != NULL) && (vmm->prev_page->page_num != page_num)){
    //     mprotect((void*)vmm->prev_page->page_start, vmm->page_size, PROT_NONE);
    // }

    // Pulling Error Code from register in ucontext struct using bit operations
    if (uctx->uc_mcontext.gregs[REG_ERR] & 0x2) {  
        // If page isnt in Memory
        if (isPresent == false){
            fault_type = WNP;
            // Check if memory is full
            switch (vmm->policy) {
                case MM_FIFO:
                    if(memory->num_frames == vmm->num_frames){
                        evicted_page = (int) memory->head->page_num;
                        evicted = memory->head;
                        write_back = evicted->write_back;                        
                        mprotect((void*)memory->head->page_start, vmm->page_size, PROT_NONE);
                        evict(memory, memory->head);
                    }
                    mprotect((void*)page_start, vmm->page_size, PROT_READ|PROT_WRITE);
                    new_page = enqueue(memory);
                    new_page->page_start = page_start;
                    new_page->page_num = page_num;
                    new_page->permission = WRITE;
                    new_page->write_back = 1;
                    
                    if (evicted_page == -1){
                        new_page->frame_num = memory->current_frame;
                        memory->current_frame ++;
                    }else{
                        new_page->frame_num = evicted->frame_num;
                    }
                    break;

                case MM_THIRD:
                    if(memory->num_frames == vmm->num_frames){
                        evicted = evict_tcr(memory);
                        evicted_page = (int) evicted->page_num;
                        write_back = evicted->write_back;                        
                        mprotect((void*)evicted->page_start, vmm->page_size, PROT_NONE);
                    }
                    mprotect((void*)page_start, vmm->page_size, PROT_READ|PROT_WRITE);
                    new_page = enqueue(memory);
                    new_page->page_start = page_start;
                    new_page->page_num = page_num;
                    new_page->permission = WRITE;
                    new_page->write_back = 1;
                    new_page->modified = 1;
                    new_page->tcr = 1;
                    
                    if (evicted_page == -1){
                        new_page->frame_num = memory->current_frame;
                        memory->current_frame ++;
                    }else{
                        new_page->frame_num = evicted->frame_num;
                    }
                    break;
            }
        }
        /* If page is in memory */
        else{
            /* Write Fault */
            switch (vmm->policy) {
                case MM_FIFO:
                    new_page->page_start = page_start;
                    new_page->page_num = page_num;
                    if(new_page->permission == READ){
                        fault_type = WRONP;
                    } else if(new_page->permission == WRITE){
                        fault_type = WRORW;
                    }
                    mprotect((void*)page_start, vmm->page_size, PROT_READ|PROT_WRITE);
                    new_page->permission = WRITE;
                    new_page->write_back = 1;
                    break;

                case MM_THIRD:
                    new_page->page_start = page_start;
                    new_page->page_num = page_num;
                    if(new_page->permission == READ){
                        fault_type = WRONP;
                    } else if(new_page->permission == WRITE){
                        fault_type = WRORW;
                    }
                    mprotect((void*)page_start, vmm->page_size, PROT_READ|PROT_WRITE);
                    new_page->permission = WRITE;
                    new_page->write_back = 1;
                    new_page->modified = 1;
                    new_page->tcr = 1;
                    break;
             }
        }
    }else{     // Read fault
        //If page isnt in Memory
        if (isPresent == false){
            fault_type = RNP;
            // Check if memory is full
                // If Memory is full, Evict a page
                switch (vmm->policy) {
                    case MM_FIFO:
                        if(memory->num_frames == vmm->num_frames){
                            evicted_page = (int) memory->head->page_num;
                            evicted = memory->head;
                            write_back = evicted->write_back;
                            mprotect((void*) memory->head->page_start, vmm->page_size, PROT_NONE);
                            evict(memory, memory->head);
                        }
                        mprotect((void*) page_start, vmm->page_size, PROT_READ);
                        new_page = enqueue(memory);
                        new_page->page_start = page_start;
                        new_page->page_num = page_num;
                        new_page->permission = READ;
                        fault_type = RNP;
                        
                        if (evicted_page == -1){
                            new_page->frame_num = memory->current_frame;
                            memory->current_frame ++;
                        }else{
                             new_page->frame_num = evicted->frame_num;
                        }
                        break;
                        
                    case MM_THIRD:
                        if(memory->num_frames == vmm->num_frames){
                            evicted = evict_tcr(memory);
                            // evicted = target;
                            // free(target);
                            evicted_page = (int) evicted->page_num;
                            write_back = evicted->write_back;                        
                            mprotect((void*)evicted->page_start, vmm->page_size, PROT_NONE);
                        }
                        mprotect((void*) page_start, vmm->page_size, PROT_READ);
                        new_page = enqueue(memory);
                        new_page->page_start = page_start;
                        new_page->page_num = page_num;
                        new_page->permission = READ;
                        fault_type = RNP;
                        
                        if (evicted_page == -1){
                            new_page->frame_num = memory->current_frame;
                            memory->current_frame ++;
                        }else{
                             new_page->frame_num = evicted->frame_num;
                        }
                        break;
                    }
        }else{
            // Page is in memory (Read Fault)
            switch (vmm->policy) {
                case MM_FIFO:
                    new_page->page_start = page_start;
                    new_page->page_num = page_num;
                    if(new_page->permission <= 1){
                    mprotect((void*) page_start, vmm->page_size, PROT_READ);
                    new_page->permission = READ;
                    }else{
                         mprotect((void*)page_start, vmm->page_size, PROT_READ|PROT_WRITE);
                    }
                    fault_type = RRORW;
                    break;

                case MM_THIRD:
                    new_page->page_start = page_start;
                    new_page->page_num = page_num;
                    new_page->tcr = 1;
                    if(new_page->permission <= 1){
                        mprotect((void*) page_start, vmm->page_size, PROT_READ);
                        new_page->permission = READ;
                    }
                    else{
                        mprotect((void*)page_start, vmm->page_size, PROT_READ|PROT_WRITE);
                    }
                    fault_type = RRORW;
                    break;
             }
        }  
    }
    phys_addr = (new_page->frame_num) * vmm->page_size + offset;
    // if (evicted_page != -1){
    //     vmm->prev_page = new_page;
    // }
    mm_logger((int) page_num, fault_type, evicted_page, write_back, phys_addr);
    if (vmm->policy == MM_THIRD){
        free(evicted);
    }
}







