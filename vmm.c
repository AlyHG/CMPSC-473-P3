#include "vmm.h"

// Memory Manager implementation
// Implement all other functions here...


Memory* init_memory(){
   Memory* memory = (Memory*) malloc(sizeof(Memory));
   memory->head = NULL;
   memory->num_frames = 0;
   memory->current_frame = 0;
   return memory;
}

page* find_page(Memory *memory, long start_address){
   if(memory->num_frames == 0){
      return NULL;
   }
   page* temp = memory->head;
   page* head_ref = memory->head;

   do{
      if(temp->page_start == start_address){
         return temp;
      }
      else{
         temp = temp->next;
      }
   }while(temp != head_ref);
   return NULL;
}

page* enqueue(Memory* memory) {
  
   // Create a new node
    page* new_page = (page*) malloc(sizeof(page));
    new_page->write_back = 0;
    new_page->tcr = 0;
   //  new_page->modified = 0;

  // If the linked list is empty, make the new node the head
  if (memory->head == NULL) {
    memory->head = new_page;
    new_page->next = memory->head;
    memory->num_frames++;
    return new_page;
  }

  // Otherwise, find the tail node in the linked list and add the new node after it
  page* tail = memory->head;
  while (tail->next != memory->head) {
    tail = tail->next;
  }
  tail->next = new_page;
  new_page->next = memory->head;
  memory->num_frames++;
  return new_page;
}

// Function to delete a node with the specified value from the linked list
void evict(Memory* memory, page* target) {
  // Return NULL if the linked list is empty
  if (memory->head == NULL) {
    return;
  }

  // If the linked list has only one node, set the head to NULL and return the node
  // if it has the specified value, otherwise return NULL
   if ((memory->head)->next == memory->head) {
      if ((memory->head) == target) {
         memory->head = NULL;
         memory->num_frames--;
         free(target);
         return;
      }
   }

  // Otherwise, find the node with the specified value and its previous node
   page* previous = memory->head;
   page* current = memory->head->next;
   while (current != memory->head) {
      if (current == target) {
         // If the node to delete is the first node, update the head of the linked list
         if (current == memory->head) {
            memory->head = current->next;
         }

         // Update the next pointer of the previous node to point to the next node after the
         // node to delete
         previous->next = current->next;

         // Return the deleted node
         free(target);
         memory->num_frames--;
         return;
      }
      previous = current;
      current = current->next;
   }
   if(current == target){
      if (current == memory->head) {
         memory->head = current->next;
      }
      previous->next = current->next;
      free(target);
      memory->num_frames-- ;
      return;
   }
   // Return if no node with the specified value was found
   return;
}

page* evict_tcr(Memory* memory){
   page* previous = NULL;
   page* current = memory->head;
   page* evicted;
   page* tail = memory->head;

   if (memory->head == NULL) {
      return NULL;
   }
   //If theres only one page
   if ((memory->head)->next == memory->head) {
      mprotect((void*)current->page_start, vmm->page_size, PROT_NONE);
      evicted = memory->head;
      memory->head = NULL;
      memory->num_frames--;
      return evicted;
   }

   while (tail->next != memory->head){
      tail = tail->next;
   }

   previous = tail;

   if((memory->head->tcr == 0)){
      mprotect((void*)current->page_start, vmm->page_size, PROT_NONE);
      evicted = memory->head;
      memory->head = memory->head->next;
      tail->next = memory->head;
      memory->num_frames--;
      return evicted;
   }
   while ((current->tcr > 0) || (current->modified > 0)){
      mprotect((void*)current->page_start, vmm->page_size, PROT_NONE);
      if ((current->tcr == 0) && (current->modified ==1)){
         // evicted = current;
         // break;
         current->modified = 0;
      }
      
      else if ((current->tcr == 1) && (current->modified == 0)){
         current->tcr = 0;
      }

      else if ((current->tcr == 1) && (current->modified == 1)){
         current->tcr = 0;
      }
      previous = current;
      current = current->next;
   }

   evicted = current;
   mprotect((void*)evicted->page_start, vmm->page_size, PROT_NONE);
   if (current == memory->head) {
      memory->head = current->next;
      tail->next = memory->head;
   }else{
      previous->next = current->next;
   }
   memory->num_frames--;
   return evicted;
}














