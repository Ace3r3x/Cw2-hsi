/*
 * LCD and GPIO control functions for Raspberry Pi
 * For F28HS Coursework 2
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <time.h>
 #include "lcdBinary.h"
 
 // GPIO memory mapping
 #define BCM2708_PERI_BASE 0x3F000000 // For RPi 2 & 3 (use 0xFE000000 for RPi 4)
 #define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)
 #define BLOCK_SIZE (4*1024)
 
 // GPIO setup macros
 #define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
 #define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
 #define GPIO_SET *(gpio+7)
 #define GPIO_CLR *(gpio+10)
 #define GPIO_READ(g) (*(gpio+13)&(1<<g))
 
 // Global variables
 static volatile unsigned *gpio;
 static int mem_fd;
 static void *gpio_map;
 
 // Initialize GPIO
 int initGPIO() {
     // Open /dev/mem
     if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
         printf("Failed to open /dev/mem\n");
         return 0;
     }
     
     // Map GPIO memory
     gpio_map = mmap(
         NULL,
         BLOCK_SIZE,
         PROT_READ|PROT_WRITE,
         MAP_SHARED,
         mem_fd,
         GPIO_BASE
     );
     
     if (gpio_map == MAP_FAILED) {
         printf("mmap error\n");
         close(mem_fd);
         return 0;
     }
     
     // Set up pointer to GPIO
     gpio = (volatile unsigned *)gpio_map;
     
     // Set up pins
     pinMode(GREEN_LED, OUTPUT);
     pinMode(RED_LED, OUTPUT);
     pinMode(BUTTON, INPUT);
     
     // Set up LCD pins
     pinMode(LCD_RS, OUTPUT);
     pinMode(LCD_EN, OUTPUT);
     pinMode(LCD_D4, OUTPUT);
     pinMode(LCD_D5, OUTPUT);
     pinMode(LCD_D6, OUTPUT);
     pinMode(LCD_D7, OUTPUT);
     
     return 1;
 }
 
 // Clean up GPIO
 void cleanupGPIO() {
     // Turn off all LEDs
     digitalWrite(GREEN_LED, 0);
     digitalWrite(RED_LED, 0);
     
     // Unmap memory
     munmap(gpio_map, BLOCK_SIZE);
     close(mem_fd);
 }
 
 // Set pin mode (INPUT or OUTPUT)
 void pinMode(int pin, int mode) {
     // Using inline assembly for direct GPIO register access
     __asm__ __volatile__(
         "mov r0, %[pin];"        // Move pin number to r0
         "mov r1, %[mode];"       // Move mode to r1
         "ldr r2, =%[gpio];"      // Load GPIO base address
         "ldr r2, [r2];"          // Dereference to get actual address
         "mov r3, r0, lsr #3;"    // r3 = pin / 10 (actually /8 then *4 for word offset)
         "add r2, r2, r3, lsl #2;" // r2 = gpio + (pin/10)*4
         "and r3, r0, #0x1F;"     // r3 = pin % 32
         "mov r3, r3, lsl #1;"    // r3 = (pin % 32) * 2 (2 bits per pin)
         "ldr r4, [r2];"          // r4 = current register value
         "mov r0, #0x3;"          // r0 = 0x3 (mask for 2 bits)
         "mov r0, r0, lsl r3;"    // r0 = mask shifted to pin position
         "bic r4, r4, r0;"        // Clear the 2 bits for this pin
         "cmp r1, #0;"            // Check if mode is INPUT (0)
         "beq 1f;"                // If INPUT, skip next instruction
         "mov r0, #0x1;"          // r0 = 0x1 (OUTPUT mode)
         "mov r0, r0, lsl r3;"    // r0 = mode shifted to pin position
         "orr r4, r4, r0;"        // Set the mode bits
         "1: str r4, [r2];"       // Store back to GPIO register
         :
         : [pin] "r" (pin), [mode] "r" (mode), [gpio] "r" (gpio)
         : "r0", "r1", "r2", "r3", "r4", "memory"
     );
 }
 
 // Write digital value to pin
 void digitalWrite(int pin, int value) {
     // Using inline assembly for direct GPIO register access
     __asm__ __volatile__(
         "mov r0, %[pin];"        // Move pin number to r0
         "mov r1, %[value];"      // Move value to r1
         "ldr r2, =%[gpio];"      // Load GPIO base address
         "ldr r2, [r2];"          // Dereference to get actual address
         "mov r3, #1;"            // r3 = 1
         "mov r3, r3, lsl r0;"    // r3 = 1 << pin
         "cmp r1, #0;"            // Check if value is 0
         "beq 1f;"                // If 0, jump to clear
         "add r2, r2, #28;"       // r2 = gpio + 7*4 (SET register)
         "b 2f;"                  // Jump to store
         "1: add r2, r2, #40;"    // r2 = gpio + 10*4 (CLR register)
         "2: str r3, [r2];"       // Store bit mask to SET/CLR register
         :
         : [pin] "r" (pin), [value] "r" (value), [gpio] "r" (gpio)
         : "r0", "r1", "r2", "r3", "memory"
     );
 }
 
 // Read digital value from pin
 int digitalRead(int pin) {
     int result;
     
     // Using inline assembly for direct GPIO register access
     __asm__ __volatile__(
         "mov r0, %[pin];"        // Move pin number to r0
         "ldr r1, =%[gpio];"      // Load GPIO base address
         "ldr r1, [r1];"          // Dereference to get actual address
         "add r1, r1, #52;"       // r1 = gpio + 13*4 (LEV0 register)
         "ldr r2, [r1];"          // r2 = current register value
         "mov r3, #1;"            // r3 = 1
         "mov r3, r3, lsl r0;"    // r3 = 1 << pin
         "and r2, r2, r3;"        // r2 = register & mask
         "cmp r2, #0;"            // Check if result is 0
         "moveq %[result], #0;"   // If 0, set result to 0
         "movne %[result], #1;"   // If not 0, set result to 1
         : [result] "=r" (result)
         : [pin] "r" (pin), [gpio] "r" (gpio)
         : "r0", "r1", "r2", "r3"
     );
     
     return result;
 }
 
 // Write to LED (blink)
 void writeLED(int pin, int value) {
     digitalWrite(pin, value);
 }
 
 // Blink LED a specified number of times
 void blinkLED(int pin, int times) {
     for (int i = 0; i < times; i++) {
         writeLED(pin, 1);
         usleep(200000); // 0.2 seconds on
         writeLED(pin, 0);
         usleep(200000); // 0.2 seconds off
     }
 }
 
 // Read button state
 int readButton() {
     return digitalRead(BUTTON);
 }
 
 // Wait for button press
 void waitForButton() {
     while (!readButton()) {
         usleep(10000); // 10ms delay
     }
     
     // Wait for button release
     while (readButton()) {
         usleep(10000); // 10ms delay
     }
     
     // Debounce
     usleep(50000); // 50ms delay
 }  {
         usleep(10000); // 10ms delay
     }
     
     // Debounce
     usleep(50000); // 50ms delay
 }
 
 // LCD functions
 // Send 4-bit command to LCD
 void lcdNibble(unsigned char nibble) {
     digitalWrite(LCD_D4, nibble & 0x01);
     digitalWrite(LCD_D5, (nibble >> 1) & 0x01);
     digitalWrite(LCD_D6, (nibble >> 2) & 0x01);
     digitalWrite(LCD_D7, (nibble >> 3) & 0x01);
     
     // Toggle enable pin
     digitalWrite(LCD_EN, 1);
     usleep(1);
     digitalWrite(LCD_EN, 0);
     usleep(1);
 }
 
 // Send 8-bit command to LCD
 void lcdByte(unsigned char byte, int mode) {
     // Set RS pin for command (0) or data (1)
     digitalWrite(LCD_RS, mode);
     
     // Send high nibble
     lcdNibble(byte >> 4);
     
     // Send low nibble
     lcdNibble(byte & 0x0F);
     
     // Wait for command to execute
     usleep(100);
 }
 
 // Initialize LCD
 int initLCD() {
     // Wait for LCD to power up
     usleep(50000);
     
     // Initialize in 4-bit mode
     digitalWrite(LCD_RS, 0);
     lcdNibble(0x03);
     usleep(5000);
     lcdNibble(0x03);
     usleep(100);
     lcdNibble(0x03);
     usleep(100);
     lcdNibble(0x02); // Set to 4-bit mode
     usleep(100);
     
     // Configure display
     lcdByte(0x28, 0); // 4-bit mode, 2 lines, 5x8 font
     lcdByte(0x0C, 0); // Display on, cursor off, blink off
     lcdByte(0x06, 0); // Increment cursor, no shift
     lcdByte(0x01, 0); // Clear display
     usleep(2000);     // Wait for clear to complete
     
     return 1;
 }
 
 // Clear LCD display
 void clearLCD() {
     lcdByte(0x01, 0); // Clear display command
     usleep(2000);     // Wait for clear to complete
 }
 
 // Write string to LCD
 void writeStringToLCD(const char* str) {
     while (*str) {
         lcdByte(*str++, 1);
     }
 }
 
 // Set LCD cursor position
 void setCursorLCD(int row, int col) {
     int row_offsets[] = {0x00, 0x40};
     lcdByte(0x80 | (row_offsets[row] + col), 0);
 }
 
 // Write line to LCD
 void writeLineToLCD(const char* str, int line) {
     setCursorLCD(line, 0);
     writeStringToLCD(str);
 }