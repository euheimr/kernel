#include "io.h"

void main() {
    // UART proto => MMIO => GPIO => CPU Registers
    uart_init();

    int i;
    for (i = 0; i < 10; i++) {
        uart_writeText("hello from my bootloader on rPi4 B !\n");
    }
    
    while(1);
}
