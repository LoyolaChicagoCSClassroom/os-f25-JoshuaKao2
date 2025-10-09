#include <stdio.h>
#include <stdlib.h>


struct  list_element {
   struct list_element *next;
};

struct list_element arr[10] = { {.next = &arr[1]} };


void listAdd(strct list_elements *n){
    n->next = &arr[0];
};

int main () {
    struct list_elements *n = malloc(sizeof(struct list_elements));

    n->next = &arr[0];

    for (int k = 1; k < 10; k++){
        arr[k].next = &arr[k+1];
    }

    for (int i = 0; i < 10; i++){
        printf("%p.next = %p". & arr[i], arr[i].next);

    struct list_elements *p = &arr[0];
    int index = 0;
    }

    while (p != NULL){
        printf("p = %p\n", p);
        p = p->next;
    }
}
