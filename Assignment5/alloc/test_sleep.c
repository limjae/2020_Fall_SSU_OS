#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "alloc.h"

int main()
{
    int a;
    scanf("%d",&a);//"first check"
    getchar();
    if(init_alloc())
		return 1;	//mmap failed

    scanf("%d",&a);//"init check"
    getchar();
    
	char *str = alloc(512);
	char *str2 = alloc(512);

	if(str == NULL || str2 == NULL)
	{
		printf("alloc failed\n");
		return 1;
	}
   
    scanf("%d",&a);//"alloc check"
    getchar();
    
    dealloc(str);
    dealloc(str2);
    scanf("%d",&a);//"dealloc check"
    getchar();
    
    cleanup();
    scanf("%d",&a);//"cleanup check"
    getchar();
    return 1;
}