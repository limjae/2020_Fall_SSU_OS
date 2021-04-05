#include "alloc.h"
#define CHUNK_NUM 512 // = 4096/MINALLOC

typedef enum
{
	FALSE = 0,
	TRUE = 1,
} bool;

int free_array[CHUNK_NUM];
char *memory = NULL;
void *addr;

int init_alloc()
{
	if (memory == NULL)
	{
		for (int i = 0; i < CHUNK_NUM; ++i)
		{
			free_array[i] = 0; // 0 is free, other is busy & required number of chunk
			// size_array[i] = 0;
		}

		addr = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		if (addr == MAP_FAILED)
		{
			// printf("#Initialization Fail\n");
			return -1;
		}
		else
		{
			memory = (char *)addr;
			// printf("#Initialization Success addr = %#x\n", (unsigned int)memory);
		}
		return 0;
	}
	else
	{
		// printf("#Memoery already initialized\n");
		return 1;
	}
}

int cleanup()
{
	int clean_page = munmap((void *)memory, PAGESIZE);
	if (clean_page == 0)
	{
		memory = NULL;
		// printf("#Memory cleaned up\n");
		return 0;
	}
	else
		return clean_page;
}

char *alloc(int bufSize)
{
	if (bufSize % MINALLOC != 0)
		return NULL;
	int chunks_required = bufSize / MINALLOC;
	int start = 0, end = 0, found = 0;

	while (end < CHUNK_NUM)
	{
		//find
		if (free_array[end] == 0)
		{
			chunks_required--;
			end++;
		}
		else //reset if no suffficient space
		{
			end++;
			start = end;
			chunks_required = bufSize / MINALLOC;
		}

		if (chunks_required == 0)
		{
			found = 1;
			break;
		}
	}

	if (found == 0)
	{
		printf("#alloc %d fail\n", bufSize);
		return NULL;
	}
	else
	{
		for (int i = start; i < end; ++i)
		{
			free_array[i] = bufSize / MINALLOC;
			//size_array[i] = chunks_required;
		}
		// printf("#alloc %d into %d ~ %d Success in %#x\n", bufSize, start, end,(unsigned int)memory);
		return (memory + start * MINALLOC);
	}
}

void dealloc(char *memAddr)
{
	int start = (memAddr - memory) / MINALLOC;
	int chunk_number = free_array[start];
	for (int i = start; i < start + chunk_number; ++i)
	{
		free_array[i] = 0; //0 is empty other= size
	}
	// printf("#Dealloc %d from %d ~ %d\n", chunk_number * MINALLOC, start, start + chunk_number);
}