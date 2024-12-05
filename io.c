// Memory Mapped I/O (MMIO) is used by General Purpose Input/Output (GPIO) Pins
enum {
    // rPi starts at Low Peripheral Mode by default, so we use 0xFE instead of 0xFF
    PERIPHERAL_BASE = 0xFE000000,
    AUX_BASE        = PERIPHERAL_BASE + 0x215000,
    PULL_NONE       = 0,
};

typedef enum {  
    GPFSEL0             = PERIPHERAL_BASE + 0x200000,
    GPSET0              = PERIPHERAL_BASE + 0x200001C,
    GPCLR0              = PERIPHERAL_BASE + 0x2000028,
    GPPUPPDN0           = PERIPHERAL_BASE + 0x20000E4,
    GPIO_MAX_PIN        = 53,
    GPIO_FUNCTION_ALT5  = 2,
}GPIO;

// Universal Async Receiver/Transmiter (UART) is a serial data exchange protocol
typedef enum {
    AUX_ENABLES     = AUX_BASE + 4,
    AUX_MU_IO_REG   = AUX_BASE + 64,
    AUX_MU_IER_REG  = AUX_BASE + 68,
    AUX_MU_IIR_REG  = AUX_BASE + 72,
    AUX_MU_LCR_REG  = AUX_BASE + 76,
    AUX_MU_MCR_REG  = AUX_BASE + 80,
    AUX_MU_LSR_REG  = AUX_BASE + 84,
    AUX_MU_CNTL_REG = AUX_BASE + 96,
    AUX_MU_BAUD_REG = AUX_BASE + 104,
}UART;

enum {
    AUX_UART_CLOCK  = 500000000,
    UART_MAX_QUEUE  = 16 * 1024,
};

unsigned int mmio_read(UART reg) { 
    return *(volatile unsigned int *)reg; 
}

void mmio_write(UART reg, unsigned int val) {
    *(volatile unsigned int *)reg = val;
}

unsigned int gpio_call(GPIO pin_number, unsigned int val, unsigned int base, 
    unsigned int field_size) {
    
    unsigned int field_mask = (1 << field_size) - 1;
    if (pin_number > GPIO_MAX_PIN) return 0;
    if (val > field_mask) return 0;

    unsigned int num_fields = 32 / field_size;
    unsigned int reg = base + ((pin_number / num_fields) * 4);
    unsigned int shift = (pin_number % num_fields) * field_size;

    unsigned int current_val = mmio_read(reg);
    current_val &= ~(field_mask << shift);
    current_val |= val << shift;
    mmio_write(reg, current_val);

    return 1;
}

unsigned int gpio_set(GPIO pin_number, unsigned int val) {
    return gpio_call(pin_number, val, GPSET0, 1);
}

unsigned int gpio_clear(GPIO pin_number, unsigned int val) {
    return gpio_call(pin_number, val, GPCLR0, 1);
}

unsigned int gpio_pull(GPIO pin_number, unsigned int val) {
    return gpio_call(pin_number, val, GPPUPPDN0, 2);
}

unsigned gpio_function(GPIO pin_number, unsigned int val) {
    return gpio_call(pin_number, val, GPFSEL0, 3);
}

void gpio_useAsAlt5(GPIO pin_number) {
    gpio_pull(pin_number, PULL_NONE);
    gpio_function(pin_number, GPIO_FUNCTION_ALT5);
}

// UART

#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK / (baud*8))-1)

void uart_init() {
    mmio_write(AUX_ENABLES, 1);         //enable UART1
    mmio_write(AUX_MU_IER_REG, 0);
    mmio_write(AUX_MU_CNTL_REG, 0);
    mmio_write(AUX_MU_LCR_REG, 3);      // 8 bits
    mmio_write(AUX_MU_MCR_REG, 0);
    mmio_write(AUX_MU_IER_REG,0);
    mmio_write(AUX_MU_IIR_REG, 0xC6);   // disable interrupts
    mmio_write(AUX_MU_BAUD_REG, AUX_MU_BAUD(115200));
    gpio_useAsAlt5(14);
    gpio_useAsAlt5(15); 
    mmio_write(AUX_MU_CNTL_REG, 3);     // enable RX/TX
}

// Checks the UART line status to ensure we are ready to send
unsigned int uart_isWriteByteReady() {
    return mmio_read(AUX_MU_LSR_REG) & 0x20;
}

// Waits until we are ready to send and then sends a single character
void uart_writeByteBlockingActual(unsigned char ch) {
    while (!uart_isWriteByteReady());
    mmio_write(AUX_MU_IO_REG, (unsigned int)ch);
}

// Sends a string using uart_writeByteBlockingActual
void uart_writeText(char *buffer) {
    while (*buffer) {
        if (*buffer == '\n') uart_writeByteBlockingActual('\r');
        uart_writeByteBlockingActual(*buffer++);
    }
}