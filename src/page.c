#include "page.h"
#include <stddef.h>

struct ppage physical_page[128];
struct ppage *free_pages = NULL;
void init_pfa_list(void){
	for(int i = 0; i < 128; i++){

		physical_page[i].physical_addr = (void*)(i*0x1000);
		if(i < 127){
		    physical_page[i].next=&physical_page[i + 1];
		}else{
		    physical_page[i].next = NULL;
		}
		if(i > 0){
		physical_page[i].prev = &physical_page[i-1];
		}else{
		physical_page[i].prev = NULL;
		}
	}
	free_pages = &physical_page[0];

}

struct ppage *allocate_physical_pages(unsigned int npages) {
	if(free_pages == NULL || npages == 0) {
		return NULL;
	}
	struct ppage *allocated_list = free_pages;
	struct ppage *current = free_pages;
	unsigned int count = 0;
	while(current != NULL && count < npages -1){
		current = current -> next;
		count++;
	}
	if(current == NULL || count < npages - 1){
		return NULL;
	}
	struct ppage *head = current -> next;
	current -> next = NULL;
	free_pages  = head;
	if(head != NULL){
		head -> prev = NULL;
	}
	allocated_list -> prev = NULL;
	return allocated_list;

}

void free_physical_pages(struct ppage *ppage_list){
	if(ppage_list == NULL){
		return;
	}
	struct ppage *current = ppage_list;
	while (current -> next != NULL){
		current = current -> next;
	}
	current -> next = free_pages;
	if(free_pages != NULL){
		free_pages -> prev = current;
	}
	free_pages = ppage_list;
	free_pages ->prev = NULL;
}
