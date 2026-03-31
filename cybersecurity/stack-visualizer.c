#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BUFFER_SIZE 64

typedef unsigned long long u64;

void show_stack(char *buf) {
    u64 rsp, rbp, rip;

    asm volatile ("mov %%rsp, %0" : "=r"(rsp));
    asm volatile ("mov %%rbp, %0" : "=r"(rbp));
    asm volatile ("lea (%%rip), %0" : "=r"(rip));

    printf("\n=== Register Snapshot ===\n");
    printf("  RSP (stack pointer) : 0x%llx\n", rsp);
    printf("  RBP (base pointer)  : 0x%llx\n", rbp);
    printf("  RIP (instr pointer) : 0x%llx\n", rip);

    printf("\n=== Stack Layout ===\n");
    printf("%-10s %-20s %-20s\n", "Offset", "Address", "Value");
    printf("%-10s %-20s %-20s\n", "------", "-------", "-----");

    for (int i = 0; i < 10; i++) {
        u64 *slot = (u64 *)(buf + i * 8);
        const char *label = "";

        if (i == 0)                 label = "<-- buffer start";
        if (i == BUFFER_SIZE/8 - 1) label = "<-- buffer end";
        if (i == BUFFER_SIZE/8)     label = "<-- saved RBP";
        if (i == BUFFER_SIZE/8 + 1) label = "<-- return addr";

        printf("+%-9d 0x%-18llx 0x%-18llx %s\n",
               i * 8, (u64)slot, *slot, label);
    }

    printf("\n=== Frame Boundaries ===\n");
    printf("  buffer @ 0x%llx\n", (u64)buf);
    printf("  RSP    @ 0x%llx  (diff from buf: %lld bytes)\n",
           rsp, (long long)(rsp - (u64)buf));
    printf("  RBP    @ 0x%llx  (diff from buf: %lld bytes)\n",
           rbp, (long long)(rbp - (u64)buf));
    printf("  RIP    @ 0x%llx  (diff from buf: %lld bytes)\n",
           rip, (long long)(rip - (u64)buf));}

void demo() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    printf("Buffer address : %p\n", buffer);
    printf("Enter text (max %d chars): ", BUFFER_SIZE - 1);
    fflush(stdout);

    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    printf("You entered: %s\n", buffer);
    show_stack(buffer);
}

int main() {
    demo();
    return 0;
}