/*
 * Header file for LCD and GPIO functions
 * For F28HS Coursework 2
 */

 #ifndef LCD_BINARY_H
 #define LCD_BINARY_H
 
 // Pin modes
 #define INPUT 0
 #define OUTPUT 1
 
 // GPIO pin definitions
 #define GREEN_LED 26  // Data LED
 #define RED_LED 5     // Control LED
 #define BUTTON 19     // Input button
 
 // LCD pin definitions
 #define LCD_RS 25
 #define LCD_EN 24
 #define LCD_D4 23
 #define LCD_D5 10
 #define LCD_D6 27
 #define LCD_D7 22
 
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
 
 // Function prototypes for LCD
 int initLCD();
 void clearLCD();
 void writeStringToLCD(const char* str);
 void setCursorLCD(int row, int col);
 void writeLineToLCD(const char* str, int line);
 
 // Function prototype for assembly function
 extern int matchesASM(int* secret, int* guess, int length, int* exactMatches, int* approxMatches);
 
 #endif // LCD_BINARY_H