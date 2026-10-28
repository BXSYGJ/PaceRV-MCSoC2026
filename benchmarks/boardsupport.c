#include "boardsupport.h"
#define REG(a) (*(volatile uint32_t *)(a))
static uint64_t begin_cycles, begin_retired, elapsed_cycles, elapsed_retired;
static uint64_t snapshot_read(uint32_t address) {
    uint32_t lo = REG(address), hi = REG(address + 4);
    return ((uint64_t)hi << 32) | lo;
}
static void putc_uart(char c) {
    while (!(REG(0x10000104u) & 1u)) {}
    REG(0x10000100u) = (unsigned char)c;
}
static void puts_uart(const char *s) { while (*s) putc_uart(*s++); }
static void hex64(uint64_t v) {
    const char *digits = "0123456789abcdef";
    for (int i = 60; i >= 0; i -= 4) putc_uart(digits[(v >> i) & 15]);
}
void initialise_board(void) { REG(0x10000000u) = 0xeb000000u; }
void start_trigger(void) {
    __asm__ volatile ("" ::: "memory");
    REG(0x10000400u) = 1;
    begin_cycles = snapshot_read(0x10000410u);
    begin_retired = snapshot_read(0x10000418u);
    __asm__ volatile ("" ::: "memory");
}
void stop_trigger(void) {
    __asm__ volatile ("" ::: "memory");
    REG(0x10000400u) = 1;
    elapsed_cycles = snapshot_read(0x10000410u) - begin_cycles;
    elapsed_retired = snapshot_read(0x10000418u) - begin_retired;
    __asm__ volatile ("" ::: "memory");
}
void finish_benchmark(int verified) {
    puts_uart("EMBENCH cycles=0x"); hex64(elapsed_cycles);
    puts_uart(" retired=0x"); hex64(elapsed_retired);
    puts_uart(verified == 1 ? " verified=1\n" : " verified=0\n");
    /* Bit 0 is FIFO space; bit 1 proves FIFO and serializer are both idle. */
    while (!(REG(0x10000104u) & 2u)) {}
    REG(0x10000000u) = verified == 1 ? 0xeb000001u : 0xeb0000ffu;
}
void trap_handler(void) { REG(0x10000000u) = 0xeb0000feu; for (;;) {} }
