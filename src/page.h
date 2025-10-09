#ifndef PAGE_H
#define PAGE_H

struct ppage{


	struct ppage *next;
	struct ppage *prev;
	void *physical_addr;
};
//GLOBAL POINTER
extern struct ppage *free_pages;
//DECLERATIONS
void init_pfa_list(void);
struct ppage * allocate_physical_pages(unsigned int npages);
void free_physical_pages(struct ppage* page_list);

#endif
