#include "iostream"
#include <string.h>
using namespace std;

extern "C"
{
	extern void coctx_swap( void *,void* ) asm("coctx_swap");
};

void* mainInfo[14];
void* funInfo[14];
/*void swap(void* rdi, void *rsi)
{
__asm__ (
        "leaq (%rsp),%rax;"
        "movq %rax, 104(%rdi);"
        "movq %rbx, 96(%rdi);"
        "movq %rcx, 88(%rdi);"
        "movq %rdx, 80(%rdi);"
        "movq 0(%rax), %rax;"
        "movq %rax, 72(%rdi);"
        "movq %rsi, 64(%rdi);"
        "movq %rdi, 56(%rdi);"
        "movq %rbp, 48(%rdi);"
        "movq %r8, 40(%rdi);"
        "movq %r9, 32(%rdi);"
        "movq %r12, 24(%rdi);"
        "movq %r13, 16(%rdi);"
        "movq %r14, 8(%rdi);"
        "movq %r15, (%rdi);"
        "xorq %rax, %rax;"
        
        "movq 48(%rsi), %rbp;"
        "movq 104(%rsi), %rsp;"
        "movq (%rsi), %r15;"
        "movq 8(%rsi), %r14;"
        "movq 16(%rsi), %r13;"
        "movq 24(%rsi), %r12;"
        "movq 32(%rsi), %r9;"
        "movq 40(%rsi), %r8;"
        "movq 56(%rsi), %rdi;"
        "movq 80(%rsi), %rdx;"
        "movq 88(%rsi), %rcx;"
        "movq 96(%rsi), %rbx;"
        
        "leaq 8(%rsp), %rsp;"
        "pushq 72(%rsi);"
        "movq 64(%rsi), %rsi;"
        "ret;"
    );
}*/

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
