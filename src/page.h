#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stddef.h>

//for describing a single physical page
struct ppage {
    struct ppage *next;
    struct ppage *prev;
    void *physical_addr;
};

#define NUM_PHYSICAL_PAGES 128
#define PAGE_SIZE_BYTES 4096

extern struct ppage *free_page_list;

void init_pfa_list(void);
struct ppage *allocate_physical_pages(unsigned int npages);
void free_physical_pages(struct ppage *ppage_list);

void print_pfa_state(void);

//Page directory entry
struct page_directory_entry
{
   uint32_t present       : 1;   // Page present in memory
   uint32_t rw            : 1;   // Read-only if clear, R/W if set
   uint32_t user          : 1;   // Supervisor only if clear
   uint32_t writethru     : 1;   // Cache this directory as write-thru only
   uint32_t cachedisabled : 1;   // Disable cache on this page table?
   uint32_t accessed      : 1;   // Supervisor level only if clear
   uint32_t pagesize      : 1;   // Has the page been accessed since last refresh?
   uint32_t ignored       : 2;   // Has the page been written to since last refresh?
   uint32_t os_specific   : 3;   // Amalgamation of unused and reserved bits
   uint32_t frame         : 20;  // Frame address (shifted right 12 bits)
};

struct page
{
   uint32_t present    : 1;   // Page present in memory
   uint32_t rw         : 1;   // Read-only if clear, readwrite if set
   uint32_t user       : 1;   // Supervisor level only if clear
   uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
   uint32_t dirty      : 1;   // Has the page been written to since last refresh?
   uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
   uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
};

//Page directory
extern struct page_directory_entry pd[1024];

//Paging API
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd_root);
void enable_paging(void);



#endif
