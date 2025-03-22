/*
 * MasterMind Game Implementation for Raspberry Pi
 * Uses GPIO pins to control LEDs, button, and LCD display
 * For F28HS Coursework 2
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <signal.h>
 #include "lcdBinary.h"
 
 // Game parameters
 #define CODE_LENGTH 3
 #define NUM_COLORS 3
 #define MAX_ATTEMPTS 10
 #define TIMEOUT_SECONDS 10
 
 // LED pins
 #define RED_LED 5
 #define GREEN_LED 26
 
 // Button pin
 #define BUTTON_PIN 19
 
 // Mutex for LCD access
 pthread_mutex_t lcd_mutex;
 
 // Function prototypes
 void displayGreeting(const char* surname);
 void generateSecret(int* secret, const char* predefinedSecret);
 void getUserGuess(int* guess);
 void displayGuess(int* guess);
 void displayAnswer(int exactMatches, int approxMatches);
 void displaySuccess(int attempts);
 void displayGameOver(int* secret);
 void signalNextRound(void);
 extern int matchesASM(int* secret, int* guess, int length, int* exactMatches, int* approxMatches);
 void runUnitTests(const char* seq1, const char* seq2);
 
 // Thread function for handling timeout
 void* timeoutThread(void* arg);
 
 // Global variables
 volatile sig_atomic_t timeoutOccurred = 0;
 int verboseMode = 0;
 int debugMode = 0;
 
 int main(int argc, char *argv[]) {
     // Initialize random seed
     srand(time(NULL));
     
     // Parse command-line arguments
     int opt;
     char *predefinedSecret = NULL;
     char *seq1 = NULL, *seq2 = NULL;
     
     while ((opt = getopt(argc, argv, "vds:u:")) != -1) {
         switch (opt) {
             case 'v':
                 verboseMode = 1;
                 break;
             case 'd':
                 debugMode = 1;
                 break;
             case 's':
                 predefinedSecret = optarg;
                 break;
             case 'u':
                 seq1 = optarg;
                 seq2 = argv[optind]; // Next argument after -u
                 if (seq2 == NULL) {
                     fprintf(stderr, "Error: Missing second sequence for unit test.\n");
                     return 1;
                 }
                 optind++; // Advance past the second sequence
                 break;
             default:
                 fprintf(stderr, "Usage: %s [-v] [-d] [-s <seq>] [-u <seq1> <seq2>]\n", argv[0]);
                 return 1;
         }
     }
 
     // Initialize GPIO
     if (!initGPIO()) {
         printf("Failed to initialize GPIO\n");
         return 1;
     }
     
     // Set up pins
     pinMode(GREEN_LED, OUTPUT);
     pinMode(RED_LED, OUTPUT);
     pinMode(BUTTON_PIN, INPUT);
     
     // Initialize LCD
     if (!initLCD()) {
         printf("Failed to initialize LCD\n");
         cleanupGPIO();
         return 1;
     }
     
     // Initialize LCD mutex
     if (pthread_mutex_init(&lcd_mutex, NULL) != 0) {
         printf("Failed to initialize LCD mutex\n");
         cleanupGPIO();
         return 1;
     }
     
     // Handle unit test mode
     if (seq1 != NULL && seq2 != NULL) {
         runUnitTests(seq1, seq2);
         cleanupGPIO();
         pthread_mutex_destroy(&lcd_mutex);
         return 0;
     }
     
     // Clear LCD and display welcome message
     clearLCD();
     writeLineToLCD("MasterMind Game", 0);
     writeLineToLCD("Press to start", 1);
     
     // Wait for button press to start
     waitForButton();
     
     // Display greeting based on surname (replace with your surname)
     const char* surname = "Smith"; // Replace with your surname
     displayGreeting(surname);
     
     // Game variables
     int secret[CODE_LENGTH];
     int guess[CODE_LENGTH];
     int exactMatches, approxMatches;
     int attempts = 0;
     int gameWon = 0;
     
     // Generate secret code
     generateSecret(secret, predefinedSecret);
     
     // Debug mode - show secret
     if (debugMode) {
         clearLCD();
         char secretStr[20];
         sprintf(secretStr, "Secret: %d %d %d", secret[0], secret[1], secret[2]);
         writeLineToLCD(secretStr, 0);
         writeLineToLCD("Game starting...", 1);
         sleep(2);
     }
     
     // Game loop
     while (!gameWon && attempts < MAX_ATTEMPTS) {
         attempts++;
         
         // Display attempt number on LCD
         clearLCD();
         char attemptStr[20];
         sprintf(attemptStr, "Attempt %d/%d", attempts, MAX_ATTEMPTS);
         writeLineToLCD(attemptStr, 0);
         writeLineToLCD("Enter your guess", 1);
         
         // Get user's guess
         getUserGuess(guess);
         
         // Display the guess
         displayGuess(guess);
         
         // Calculate matches using assembly function
         matchesASM(secret, guess, CODE_LENGTH, &exactMatches, &approxMatches);
         
         // Display answer
         displayAnswer(exactMatches, approxMatches);
         
         // Check if game is won
         if (exactMatches == CODE_LENGTH) {
             gameWon = 1;
             displaySuccess(attempts);
         } else {
             // Signal start of next round
             signalNextRound();
         }
     }
     
     // If game is lost
     if (!gameWon) {
         displayGameOver(secret);
     }
     
     // Clean up GPIO
     cleanupGPIO();
     
     // Destroy LCD mutex
     pthread_mutex_destroy(&lcd_mutex);
     
     return 0;
 }
 
 // Display greeting based on surname
 void displayGreeting(const char* surname) {
     clearLCD();
     writeLineToLCD("Welcome to", 0);
     writeLineToLCD("MasterMind!", 1);
     
     // Blink LEDs based on surname (first 5 letters)
     int len = strlen(surname);
     if (len > 5) len = 5;
     
     for (int i = 0; i < len; i++) {
         char c = surname[i];
         // Convert to lowercase for easier comparison
         if (c >= 'A' && c <= 'Z') c += 32;
         
         // Check if vowel
         if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
             blinkLED(GREEN_LED, 1); // Vowel - blink green once
         } else {
             blinkLED(RED_LED, 1);   // Consonant - blink red once
         }
         usleep(500000); // 0.5 second pause
     }
     
     // Pause after greeting
     sleep(2);
 }
 
 // Generate random secret code
 void generateSecret(int* secret, const char* predefinedSecret) {
     if (predefinedSecret != NULL) {
         // Use predefined secret code
         for (int i = 0; i < CODE_LENGTH; i++) {
             if (i < strlen(predefinedSecret)) {
                 secret[i] = predefinedSecret[i] - '0'; // Convert char to int
                 
                 // Ensure values are within valid range
                 if (secret[i] < 1 || secret[i] > NUM_COLORS) {
                     secret[i] = 1; // Default to 1 if invalid
                 }
             } else {
                 // If predefined secret is too short, fill with random values
                 secret[i] = (rand() % NUM_COLORS) + 1;
             }
         }
     } else {
         // Generate random secret code
         for (int i = 0; i < CODE_LENGTH; i++) {
             secret[i] = (rand() % NUM_COLORS) + 1; // 1 to NUM_COLORS
         }
     }
     
     if (verboseMode) {
         printf("Secret code: ");
         for (int i = 0; i < CODE_LENGTH; i++) {
             printf("%d ", secret[i]);
         }
         printf("\n");
     }
 }
 
 // Get user's guess via button presses
 void getUserGuess(int* guess) {
     pthread_t timeout_thread;
     int count;
     
     for (int i = 0; i < CODE_LENGTH; i++) {
         // Reset timeout flag
         timeoutOccurred = 0;
         count = 0;
         
         // Display prompt on LCD
         clearLCD();
         char promptStr[20];
         sprintf(promptStr, "Enter digit %d:", i + 1);
         writeLineToLCD(promptStr, 0);
         writeLineToLCD("Press button", 1);
         
         // Create timeout thread
         if (pthread_create(&timeout_thread, NULL, timeoutThread, NULL) != 0) {
             perror("Failed to create timeout thread");
             exit(EXIT_FAILURE);
         }
         
         // Wait for button presses or timeout
         while (count < NUM_COLORS && !timeoutOccurred) {
             if (readButton()) {
                 count++;
                 
                 // Acknowledge input with red LED
                 blinkLED(RED_LED, 1);
                 
                 // Echo the input value with green LED
                 blinkLED(GREEN_LED, count);
                 
                 // Debounce
                 while (readButton()) {
                     usleep(10000); // 10ms delay
                 }
                 usleep(50000); // 50ms delay
                 
                 // Update LCD with current count
                 char countStr[20];
                 sprintf(countStr, "Count: %d", count);
                 
                 pthread_mutex_lock(&lcd_mutex);
                 writeLineToLCD(countStr, 1);
                 pthread_mutex_unlock(&lcd_mutex);
             }
             usleep(10000); // 10ms delay
         }
         
         // Cancel timeout thread
         pthread_cancel(timeout_thread);
         pthread_join(timeout_thread, NULL); // Wait for thread to terminate
         
         // Store the guess (ensure it's within valid range)
         if (count < 1) count = 1;
         if (count > NUM_COLORS) count = NUM_COLORS;
         guess[i] = count;
         
         // Display selected digit
         char digitStr[20];
         sprintf(digitStr, "Digit %d: %d", i + 1, guess[i]);
         clearLCD();
         writeLineToLCD(digitStr, 0);
         writeLineToLCD("Press for next", 1);
         
         // Wait for button press to continue
         if (i < CODE_LENGTH - 1) {
             waitForButton();
         }
     }
     
     // Signal end of input with red LED blinking twice
     blinkLED(RED_LED, 2);
     
     if (verboseMode) {
         printf("User guess: ");
         for (int i = 0; i < CODE_LENGTH; i++) {
             printf("%d ", guess[i]);
         }
         printf("\n");
     }
 }
 
 // Thread function for handling timeout
 void* timeoutThread(void* arg) {
     sleep(TIMEOUT_SECONDS);
     timeoutOccurred = 1;
     pthread_exit(NULL);
     return NULL; // Suppress compiler warning
 }
 
 // Display the guess on LCD
 void displayGuess(int* guess) {
     clearLCD();
     char guessStr[20];
     sprintf(guessStr, "Guess: %d %d %d", guess[0], guess[1], guess[2]);
     writeLineToLCD(guessStr, 0);
     writeLineToLCD("Processing...", 1);
     usleep(500000); // 0.5 second pause
 }
 
 // Display answer via LEDs and LCD
 void displayAnswer(int exactMatches, int approxMatches) {
     // Display on LEDs
     blinkLED(GREEN_LED, exactMatches);  // Exact matches
     blinkLED(RED_LED, 1);               // Separator
     blinkLED(GREEN_LED, approxMatches); // Approximate matches
     
     // Display on LCD
     clearLCD();
     char answerStr[20];
     sprintf(answerStr, "Exact: %d", exactMatches);
     writeLineToLCD(answerStr, 0);
     sprintf(answerStr, "Approx: %d", approxMatches);
     writeLineToLCD(answerStr, 1);
     
     if (verboseMode) {
         printf("Results: Exact matches = %d, Approximate matches = %d\n", 
                exactMatches, approxMatches);
     }
     
     // Pause to let user see the result
     sleep(2);
 }
 
 // Display success message
 void displaySuccess(int attempts) {
     // Blink success pattern - green LED 3 times while red LED is on
     digitalWrite(RED_LED, 1); // Turn on red LED
     blinkLED(GREEN_LED, 3);   // Blink green 3 times
     digitalWrite(RED_LED, 0); // Turn off red LED
     
     // Display on LCD
     clearLCD();
     writeLineToLCD("SUCCESS!", 0);
     char attemptsStr[20];
     sprintf(attemptsStr, "Attempts: %d", attempts);
     writeLineToLCD(attemptsStr, 1);
     
     if (verboseMode) {
         printf("Game won in %d attempts!\n", attempts);
     }
     
     // Keep success message displayed
     sleep(5);
 }
 
 // Display game over message
 void displayGameOver(int* secret) {
     clearLCD();
     writeLineToLCD("GAME OVER", 0);
     char secretStr[20];
     sprintf(secretStr, "Secret: %d %d %d", secret[0], secret[1], secret[2]);
     writeLineToLCD(secretStr, 1);
     
     // Blink red LED 5 times to indicate game over
     blinkLED(RED_LED, 5);
     
     if (verboseMode) {
         printf("Game over! Secret was: %d %d %d\n", 
                secret[0], secret[1], secret[2]);
     }
     
     // Keep game over message displayed
     sleep(5);
 }
 
 // Signal the start of the next round
 void signalNextRound(void) {
     // Blink red LED 3 times to indicate next round
     blinkLED(RED_LED, 3);
 }
 
 // Unit test function
 void runUnitTests(const char* seq1, const char* seq2) {
     int secret[CODE_LENGTH], guess[CODE_LENGTH];
     int exactMatches, approxMatches;
     
     // Convert sequences to integer arrays
     for (int i = 0; i < CODE_LENGTH; i++) {
         if (i < strlen(seq1)) {
             secret[i] = seq1[i] - '0';
             // Ensure values are within valid range
             if (secret[i] < 1 || secret[i] > NUM_COLORS) secret[i] = 1;
         } else {
             secret[i] = 1; // Default value
         }
         
         if (i < strlen(seq2)) {
             guess[i] = seq2[i] - '0';
             // Ensure values are within valid range
             if (guess[i] < 1 || guess[i] > NUM_COLORS) guess[i] = 1;
         } else {
             guess[i] = 1; // Default value
         }
     }
     
     // Calculate matches using assembly function
     matchesASM(secret, guess, CODE_LENGTH, &exactMatches, &approxMatches);
     
     // Display results
     printf("Unit Test Results:\n");
     printf("Secret: ");
     for (int i = 0; i < CODE_LENGTH; i++) {
         printf("%d ", secret[i]);
     }
     printf("\nGuess: ");
     for (int i = 0; i < CODE_LENGTH; i++) {
         printf("%d ", guess[i]);
     }
     printf("\nExact matches: %d, Approximate matches: %d\n", exactMatches, approxMatches);
     
     // Display on LCD
     clearLCD();
     char testStr[20];
     sprintf(testStr, "Test: %s vs %s", seq1, seq2);
     writeLineToLCD(testStr, 0);
     
     char resultStr[20];
     sprintf(resultStr, "E:%d A:%d", exactMatches, approxMatches);
     writeLineToLCD(resultStr, 1);
     
     // Visual feedback with LEDs
     blinkLED(GREEN_LED, exactMatches);
     blinkLED(RED_LED, 1);
     blinkLED(GREEN_LED, approxMatches);
     
     // Keep results displayed
     sleep(5);
 }while (0);