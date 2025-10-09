#include "page.h"
#include "rprintf.h"
#include <stdint.h>
#include <stddef.h>

//array representing all pages
static struct ppage physical_page_array[NUM_PHYSICAL_PAGES];

//head of the free list
struct ppage *free_page_list = 0;

void init_pfa_list(void) {
    //initialize all pages and link them together
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        physical_page_array[i].physical_addr = (void *)((uintptr_t)i * PAGE_SIZE_BYTES);
        physical_page_array[i].next = (i < NUM_PHYSICAL_PAGES - 1) ? &physical_page_array[i + 1] : 0;
        physical_page_array[i].prev = (i > 0) ? &physical_page_array[i - 1] : 0;
    }
    free_page_list = &physical_page_array[0];
}

//allocate npages from the front of the free list
struct ppage *allocate_physical_pages(unsigned int npages) {
    if (!free_page_list || npages == 0)
        return 0;

    //check if there's enough free pages
    unsigned int count = 0;
    struct ppage *cur = free_page_list;
    while (cur && count < npages) {
        count++;
        cur = cur->next;
    }
    if (count < npages)
        //return 0 if there's not enough pages
        return 0; 

    //detach npages from the free list
    struct ppage *alloc_head = free_page_list;
    struct ppage *alloc_tail = alloc_head;
    for (unsigned int i = 1; i < npages; i++) {
        alloc_tail = alloc_tail->next;
    }

    free_page_list = alloc_tail->next;
    if (free_page_list)
        free_page_list->prev = 0;

    alloc_tail->next = 0;
    alloc_head->prev = 0;

    return alloc_head;
}

//return a list of pages to the free list
void free_physical_pages(struct ppage *ppage_list) {
    if (!ppage_list)
        return;

    //find the end of the list being freed
    struct ppage *tail = ppage_list;
    while (tail->next)
        tail = tail->next;

    //attach the free list to the tail
    if (free_page_list)
        free_page_list->prev = tail;

    tail->next = free_page_list;
    ppage_list->prev = 0;
    free_page_list = ppage_list;
}

//print helper
extern int vga_putc(int c);
#define HEXPTR(x) ((unsigned)((uintptr_t)(x)))

void print_pfa_state(void) {
    struct ppage *cur = free_page_list;
    esp_printf(vga_putc, "\nFree list:\n");
    while (cur) {
        esp_printf(vga_putc, "  Page @ 0x%08x -> phys=0x%08x\n",
                   HEXPTR(cur), HEXPTR(cur->physical_addr));
        cur = cur->next;
    }
    esp_printf(vga_putc, "(end of free list)\n");
}

//Paging

struct page_directory_entry pd[1024] __attribute__((aligned(4096)));

//Pool of 4 KiB-aligned page tables. Allocate per PD slot when necessary
static struct page pt_pool[16][1024] __attribute__((aligned(4096)));
static int next_free_pt = 0;

static inline uint32_t pd_index(uint32_t va) { return (va >> 22) & 0x3FF; }
static inline uint32_t pt_index(uint32_t va) { return (va >> 12) & 0x3FF; }

//Map a linked list of physical pages to virtual address
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd_root) {
    struct ppage *current_page = pglist;
    uint32_t virt_addr = (uint32_t)vaddr;

    while (current_page != NULL) {
        uint32_t pdi = pd_index(virt_addr);
        uint32_t pti = pt_index(virt_addr);

        //Allocate a fresh Page Table for this PDE if missing
        if (!pd_root[pdi].present) {
            if (next_free_pt >= (int)(sizeof(pt_pool) / sizeof(pt_pool[0]))) {
                //Out of PTs. Stop early
                break;
            }

            struct page *new_pt = pt_pool[next_free_pt++];

            //Zero the PT entries
            for (int i = 0; i < 1024; i++) {
                new_pt[i].present  = 0;
                new_pt[i].rw       = 0;
                new_pt[i].user     = 0;
                new_pt[i].accessed = 0;
                new_pt[i].dirty    = 0;
                new_pt[i].unused   = 0;
                new_pt[i].frame    = 0;
            }

            //Install PDE
            pd_root[pdi].present       = 1;
            pd_root[pdi].rw            = 1;
            pd_root[pdi].user          = 0;
            pd_root[pdi].writethru     = 0;
            pd_root[pdi].cachedisabled = 0;
            pd_root[pdi].accessed      = 0;
            pd_root[pdi].pagesize      = 0;
            pd_root[pdi].ignored       = 0;
            pd_root[pdi].os_specific   = 0;
            pd_root[pdi].frame         = ((uint32_t)new_pt) >> 12;
        }

        //Recover PT VA from PDE's frame
        struct page *pt_va = (struct page *)((pd_root[pdi].frame) << 12);

        //Fill the PTE
        pt_va[pti].present  = 1;
        pt_va[pti].rw       = 1;
        pt_va[pti].user     = 0;
        pt_va[pti].accessed = 0;
        pt_va[pti].dirty    = 0;
        pt_va[pti].unused   = 0;
        pt_va[pti].frame    = ((uint32_t)current_page->physical_addr) >> 12;

        //Next page
        current_page = current_page->next;
        virt_addr += PAGE_SIZE_BYTES;
    }

    return vaddr;
}

void enable_paging(void) {
    extern char _end_kernel;

    //Initialize page directory to all zeros
    for (int i = 0; i < 1024; i++) {
    pd[i].present       = 0;
        pd[i].rw            = 0;
        pd[i].user          = 0;
        pd[i].writethru     = 0;
        pd[i].cachedisabled = 0;
        pd[i].accessed      = 0;
        pd[i].pagesize      = 0;
        pd[i].ignored       = 0;
        pd[i].os_specific   = 0;
        pd[i].frame         = 0;
}
   next_free_pt = 0;
   //Identity map kernel from 0x100000 to _end_kernel
    uint32_t kernel_start = 0x100000;
    uint32_t kernel_end = (uint32_t)&_end_kernel;
    kernel_end = (kernel_end + (PAGE_SIZE_BYTES - 1)) & ~(PAGE_SIZE_BYTES - 1);

    esp_printf(vga_putc, "Mapping kernel from %x to %x\n", kernel_start, kernel_end);
    for (uint32_t addr = kernel_start; addr < kernel_end; addr += PAGE_SIZE_BYTES) {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.prev = NULL;
        tmp.physical_addr = (void *)addr;
        map_pages((void *)addr, &tmp, pd);
    }
    
    // Identity map the current stack
    uint32_t esp;
    __asm__ __volatile__("mov %%esp, %0" : "=r"(esp));
    uint32_t stack_base = esp & ~(PAGE_SIZE_BYTES -1); 

    for (int i = 0; i < 4; i++) {
        uint32_t saddr = stack_base - (uint32_t)i * PAGE_SIZE_BYTES;
        esp_printf(vga_putc, "Mapping stack page at %x\n", saddr);
        struct ppage stack_tmp;
        stack_tmp.next = NULL;
        stack_tmp.prev = NULL;
        stack_tmp.physical_addr = (void *)saddr;
        map_pages((void *)saddr, &stack_tmp, pd);
    }
//Identity map video memory at 0xB8000
    esp_printf(vga_putc, "Mapping video memory at %x\n", 0xB8000);
    struct ppage video_tmp;
    video_tmp.next = NULL;
    video_tmp.prev = NULL;
    video_tmp.physical_addr = (void *)0xB8000;
    map_pages((void *)0xB8000, &video_tmp, pd);

    //Load CR3
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(pd) : "memory");

    // Enable paging: set CR0.PG and CR0.PE
    __asm__ __volatile__(
        "mov %%cr0, %%eax\n\t"
        "or  $0x80000001, %%eax\n\t"
        "mov %%eax, %%cr0"
        ::: "eax","memory"
    );

    esp_printf(vga_putc, "Paging enabled!\n");
}
