/*
 * Header file for GPIO functions
 * For F28HS Coursework 2
 */

 #ifndef GPIO_H
 #define GPIO_H
 
 // Pin modes
 #define INPUT 0
 #define OUTPUT 1
 
 // GPIO pin definitions
 #define GREEN_LED 26  // Data LED
 #define RED_LED 5     // Control LED
 #define BUTTON 19     // Input button
 
 // Function prototypes for GPIO
 int initGPIO();
 void cleanupGPIO();
 void pinMode(int pin, int mode);
 void digitalWrite(int pin, int value);
 int digitalRead(int pin);
 void writeLED(int pin, int value);
 void blinkLED(int pin, int times);
 int readButton();
 void waitForButton();
 
 #endif// GPIO_H