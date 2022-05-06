#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
void* my_malloc(size_t size);
void my_free(void* ptr);
void * my_calloc(size_t nelem, size_t elsize);