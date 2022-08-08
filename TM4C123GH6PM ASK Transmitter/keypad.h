//keypad.h

#ifndef __KEYPAD_H__
#define __KEYPAD_H__

// structure for holding mailbox, for transferring data between main and keypad.c
typedef struct MailBox{
	unsigned char intFlag, data;
} MailBox;

// Make PORTE outputs for columns, PORTA inputs for rows, 
// enable periodic interrupt to cycle columns and edge triggered interrupts for determining row
void initializeKeyPad(void);

// Set up Timer0B to cycle through PortE0-3 (columns of keypad)
void Timer0B_Init(unsigned long period);

// Cycle through columns one at a time periodically
void Timer0B_Handler(void);

// When key is pressed, see which row/column it was to determine which key was pressed in matrix
void GPIOPortA_Handler(void);

// Function for getting which key has been pressed
unsigned char getKey(void);

// Function for checking if new data is available
unsigned char keyPressed(void);

// Function for setting flag when new data is available
void setFlag(unsigned char num);

// Disable keypad interrupts when transmitting data through FS1000A transmitter
void disableKeyPad(void);

// Re-enable keypad interrupts after transmitting data through FS1000A transmitter
void enableKeyPad(void);

#endif //__KEYPAD_H__
