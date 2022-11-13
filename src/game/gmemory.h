#ifndef FALLOUT_GAME_GMEMORY_H_
#define FALLOUT_GAME_GMEMORY_H_

#include <stddef.h>

void* localmymalloc(size_t size);
void* localmyrealloc(void* ptr, size_t size);
void localmyfree(void* ptr);
char* localmystrdup(const char* string);
int gmemory_init();
void* gmalloc(size_t size);
void* grealloc(void* ptr, size_t newSize);
void gfree(void* ptr);

#endif /* FALLOUT_GAME_GMEMORY_H_ */
