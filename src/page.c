#include "page.h"
#include <stddef.h>

//Decleration of an arry
struct ppage physical_page[128];
//Also declaring a global pointer 
struct ppage *free_pages = NULL; 
void init_pfa_list(void){
	
	for(int i = 0; i < 128; i++){
		//This helps us give the physical address
		physical_page[i].physical_addr = (void*)(i*0x1000);
		//We check if not last element
		if(i < 127){
		//Then point to the next array element
		physical_page[i].next=&physical_page[i + 1];
		}else{
		//Make the next value null
		physical_page[i].next = NULL;
		}
		if(i > 0){
		//This points to the previous element 
		physical_page[i].prev = &physical_page[i-1];
		}else{
		//Set the previous element to NULL
		physical_page[i].prev = NULL;
		}
	}
	//Then sent the free_pages pointer to the first element of the Array
	free_pages = &physical_page[0];

}

struct ppage *allocate_physical_pages(unsigned int npages) {
	//Check if there are no pages left
	if(free_pages == NULL || npages == 0) {
		return NULL;
	}
	//Stores free_pages in allocated_list
	struct ppage *allocated_list = free_pages;
	//This helps us travers ethe linked list
	struct ppage *current = free_pages;
	//Keeps count of the amount of pages
	unsigned int count = 0;
	//This will continue until it reaches the last block
	while(current != NULL && count < npages -1){
		current = current -> next;
		count++;
	}
	//This then check if it went through the while loop completly
	if(current == NULL || count < npages - 1){
		return NULL;
	}
	//Then we save the next element in head
	struct ppage *head = current -> next;
	current -> next = NULL;
	//Then udpate the global list to head
	free_pages  = head;
	//Checks if head is emtpy
	if(head != NULL){
		head -> prev = NULL; 
	}
	//Makes the previous elemtn empty
	allocated_list -> prev = NULL;	
	//return pointer
	return allocated_list;

}

void free_physical_pages(struct ppage *ppage_list){
	//Check if null
	if(ppage_list == NULL){
	//Return empty
		return;
	}
	//Then we want to create a pointer to set first node of the list
	struct ppage *current = ppage_list;
	//Traverse through the nodes
	while (current -> next != NULL){
		current = current -> next;
	}
	//then set the next node to free_pages
	current -> next = free_pages;
	//Check if free page is null
	if(free_pages != NULL){
		//Set the previous node to current 
		free_pages -> prev = current;
	} 
	//Set ppage in free_page
	free_pages = ppage_list;
	//Then set the previous node to null
	free_pages ->prev = NULL;
}
