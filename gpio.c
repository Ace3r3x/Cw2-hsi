/*
 * GPIO control functions for Raspberry Pi
 * For F28HS Coursework 2
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <time.h>
 #include "gpio.h"
 
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
     INP_GPIO(pin); // Always set as input first
     if (mode == OUTPUT) {
         OUT_GPIO(pin); // Then set as output if needed
     }
 }
 
 // Write digital value to pin
 void digitalWrite(int pin, int value) {
     if (value) {
         GPIO_SET = 1 << pin;
     } else {
         GPIO_CLR = 1 << pin;
     }
 }
 
 // Read digital value from pin
 int digitalRead(int pin) {
     return (GPIO_READ(pin)) ? 1 : 0;
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
 }