#include "iostream"
#include <string.h>
using namespace std;

extern "C"
{
	extern void coctx_swap( void *,void* ) asm("coctx_swap");
};

void* mainInfo[14];
void* funInfo[14];

void fun1()
{
    printf("call fun1.\n");
    coctx_swap(funInfo, mainInfo);
}

void init()
{
    memset(mainInfo, 0, sizeof(void*) * 14);
    memset(funInfo, 0, sizeof(void*) * 14);

    char* stack = (char*)malloc(128 * 1024);
    void* sp = stack + (128 * 1024) - sizeof(void*);
    sp = (char*)((unsigned long)sp & -16LL);

    funInfo[13] = sp;
    funInfo[9] = (void*)fun1;
}

int main()
{
    printf("main fun.\n");
    init();
    coctx_swap(mainInfo, funInfo);
    printf("main end.\n");
    return 0;
}
