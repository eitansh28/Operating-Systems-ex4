#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "implementMemory.hpp"


//I took this code from https://stackoverflow.com/questions/5422061/malloc-implementation
typedef struct free_block {
    size_t size;
    struct free_block* next;
} free_block;

static free_block free_block_list_head = { 0, 0 };
static const size_t overhead = sizeof(size_t);
static const size_t align_to = 16;

void* my_malloc(size_t size){
    size = (size + sizeof(size_t) + (align_to - 1)) & ~ (align_to - 1);
    free_block* block = free_block_list_head.next;
    free_block** head = &(free_block_list_head.next);
    while (block != 0) {
        if (block->size >= size) {
            *head = block->next;
            return ((char*)block) + sizeof(size_t);
        }
        head = &(block->next);
        block = block->next;
    }

    block = (free_block*)sbrk(size);
    block->size = size;

    return ((char*)block) + sizeof(size_t);
}

void my_free(void* ptr){
    free_block* block = (free_block*)(((char*)ptr) - sizeof(size_t));
    block->next = free_block_list_head.next;
    free_block_list_head.next = block;
}

void * my_calloc(size_t nelem, size_t elsize){
    void *p;
    p = malloc (nelem * elsize);
    if (p == 0)
        return (p);

    bzero (p, nelem * elsize);
    return (p);
}