#include "ealloc.h"
#define CHUNK_NUM 16 // = 4096/MINALLOC
#define MAX_PAGES 4


typedef enum
{
	FALSE = 0,
	TRUE = 1,
} bool;

int free_array[MAX_PAGES][CHUNK_NUM];
char *memory[MAX_PAGES];

int pagecount = 0; // number of mapped page

void init_alloc()
{
	for (int Page_seq = 0; Page_seq < MAX_PAGES; Page_seq++)
	{
		memory[Page_seq] = NULL;
		for (int i = 0; i < CHUNK_NUM; i++)
		{
			free_array[Page_seq][i] = 0;
		}
	}
	//printf("#Initialization Success\n");
}

void cleanup()
{
	for (int Page_seq = 0; Page_seq < MAX_PAGES; Page_seq++)
	{
		if(memory[Page_seq] == NULL)
		{
		}
		else
		{
			if (munmap((void *)memory[Page_seq], PAGESIZE) == 0)
			{
				memory[Page_seq] = NULL;
			}
			else
			{
				// printf("#Something goes Wrong in clean up\n");
			}
		}
	}
	pagecount = 0; // reset number of mapped pages
	// printf("#Memory cleaned up\n");
}


int mapping()
{
	if(pagecount<MAX_PAGES)
	{}
	else
	{
		// printf("#Page Limit Exceed\n");
		return 1;
	}

	void *addr = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (addr == MAP_FAILED)
	{
		// printf("#Mapping Fail\n");
		return 1;
	}
	else
	{
		memory[pagecount] = (char *)addr;
		// printf("#Mapping Success addr = %#x\n", (unsigned int)memory[pagecount]);
	}

	pagecount++;
	return 0;
}

char *alloc(int bufSize)
{
	int chunks_required = bufSize / MINALLOC;
	int start = 0, end = 0, found = 0;

	if (bufSize % MINALLOC != 0)
		return NULL;
	
	int Page_seq=0;
	while(found == 0)
	{
		start = 0, end = 0;
		// printf("#Curr Page addr = %#x\n", (unsigned int)memory[Page_seq]);
		if (memory[Page_seq] == NULL)
		{
			if (mapping()) // map one more page
			{			   //err case
				// printf("#mapping in %d fail\n", Page_seq);
				return NULL;
			}
		}
		else
		{
			while (end < CHUNK_NUM)
			{
				//find
				if (free_array[Page_seq][end] == 0)
				{
					chunks_required--;
					end++;
					//printf("#%d chunks_required left\n",chunks_required);
				}
				else //reset if no suffficient space
				{
					//printf("#find reset in [%d][%d]\n",Page_seq,end);
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
				// printf("#can't find space in Page addr = %#x\n", (unsigned int)memory[Page_seq]);
				Page_seq++;
			}
			else
			{
				for (int j = start; j < end; ++j)
				{
					free_array[Page_seq][j] = bufSize / MINALLOC;
					//size_array[i] = chunks_required;
				}
				// printf("#alloc %d into %d ~ %d Success\n", bufSize, start, end);
				return (memory[Page_seq] + start * MINALLOC);
			}
		}
	}
}

void dealloc(char *memAddr)
{
	int Page_seq;
	int start;
	for (Page_seq = 0; Page_seq < MAX_PAGES; Page_seq++)
	{
		if(memAddr - memory[Page_seq] < 4096)
		{
			if(memAddr - memory[Page_seq] >= 0)
			{
				start = (memAddr - memory[Page_seq]) / MINALLOC;
				break;
			}
		}
	}

	int chunk_number = free_array[Page_seq][start];
	for (int i = start; i < start + chunk_number; i++)
	{
		free_array[Page_seq][i] = 0; //0 is empty other= size
	}
	// printf("#Dealloc %d from %d ~ %d\n", chunk_number * MINALLOC, start, start + chunk_number-1);
}