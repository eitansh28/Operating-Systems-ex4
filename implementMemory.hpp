// #include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#ifndef MULTI_THREADED_LOCK_FREE_STACK_MEMORY_HPP
#define MULTI_THREADED_LOCK_FREE_STACK_MEMORY_HPP
void* my_malloc(size_t size);
void my_free(void* ptr);
void * my_calloc(size_t nelem, size_t elsize);
#endif