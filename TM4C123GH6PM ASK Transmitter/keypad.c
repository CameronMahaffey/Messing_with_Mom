//keypad.c

/* Functions that make use of Timer 0A, Port A2-5 and Port E0-3 for implementing a 4x4 keypad matrix.
 * Instead of polling, the keypad is set up to use interrupts. Columns are set up as outputs on Port 
 * A2-5 and is set high periodically by Timer 0A, at a rate of 80000 cycles (40000*2). Each row is set
 * up as an input with edge triggered interrupts. When a button is pressed, the current column pin that 
 * is high and the interrupt status of the row pin is used to determine which key was pressed.

 * After a key is pressed, the keypad interrupts for columns are disabled in the NVIC registers until 
 * after the message has been sent through the transmitter. 
 */
#include "keypad.h"
#include "..//tm4c123gh6pm.h"

#define ROW 4						// Number of rows in keypad matrix
#define COL 4						// Number of columns in keypad matrix
#define PERIOD 40000		// 80000 cycles at 16MHz is 5ms per interrupt, timer is set to 16bit, stored 40000 with prescaler as 1

// The mapping of the keys on the 4x4 matrix
const unsigned char KeyMap[ROW][COL] = {{'1', '2', '3', 'A'}, {'4', '5', '6', 'B'},
																				{'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}};

// Mailbox for storing which key pressed and ready flag
MailBox mail;

// Make PORTE outputs for columns, PORTA inputs for rows, 
// enable periodic interrupt to cycle columns and edge triggered interrupts for determining row
void initializeKeyPad(void){
	// turn on clock for PORTA and PORTE
	SYSCTL_RCGCGPIO_R |= 0x11;		//bits 4 and 0 set high
	while((SYSCTL_PRGPIO_R&0x11) == 0);	// make sure peripheral is ready before altering registers
	//configure PORTB 0-3 as columns
	GPIO_PORTE_DIR_R |= 0x0F;	// E0-E3 output
	GPIO_PORTE_DEN_R |= 0x0F; // E0-E3 digital enable
	GPIO_PORTE_PCTL_R &= 0xFFFF0000;		// E0-E3 general gpio 
	GPIO_PORTE_AFSEL_R &= ~0x0F;		// E0-E3 no alt function
	GPIO_PORTE_AMSEL_R &= ~0x0F;		// E0-E3 not analog
	GPIO_PORTE_DATA_R = 0x01;				// initially set column 0 high
	
	//configure PORTA 2-5 as rows with rising edge interrupts
	GPIO_PORTA_DIR_R &= ~0x3C;	// A2-A5 input
	GPIO_PORTA_DEN_R |= 0x3C;	// A2-A5 digital enable
	GPIO_PORTA_PCTL_R &= 0x00FFFF00;	// A2-A5 general gpio
	GPIO_PORTA_PDR_R |= 0x3C;	// A2-A5 enable pull down resistors
	GPIO_PORTA_AFSEL_R &= ~0x3C;	// A2-A5 no alt function
	GPIO_PORTA_AMSEL_R &= ~0x3C;	// A2-A5 not analog
	
	//configure edge triggered interrupts for PA2-5
	GPIO_PORTA_IS_R &= ~0x3C; // clear bits to configure for edge interrupt
	GPIO_PORTA_IBE_R &= ~0x3C;	// clear bits for single edge detection mode
	GPIO_PORTA_IEV_R |= 0x3C;	// set bits for rising edge detection
	GPIO_PORTA_ICR_R = 0x3C;	// clear bits in raw interrupt status register (writing 1 to ICR register)
	GPIO_PORTA_IM_R |= 0x3C;	//mask the bits to arm interrupt
	NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFFFF1F) | 0x00000040; // make priority 2
	NVIC_EN0_R |= 0x00000001;		//enable bit 0 for GPIOA interrupts (int number 0)
	
	Timer0B_Init(PERIOD); // Initialize Timer0B
	setFlag(0);		// Initialize flag to 0
}

// Set up Timer0B to cycle through PortE0-3 (columns of keypad)
void Timer0B_Init(unsigned long period){
	SYSCTL_RCGCTIMER_R |= 0x0001;		// enable clock for timer 0
	while((SYSCTL_PRTIMER_R&SYSCTL_PRTIMER_R0) == 0);	// make sure timer 0 peripheral is ready before register access
	TIMER0_CTL_R = 0;
	TIMER0_CFG_R = 0x04;		//put into individual mode to use both timer0A and 0B
	//TIMER0_PP_R  = 0x01;	// set timers into 32bit wide mode
	TIMER0_TBMR_R = 2;		//Timer0B in periodic mode
	TIMER0_TBILR_R = (period) - 1;	//Set period of Timer
	TIMER0_TBPR_R = 1;		//Prescaler to make interrupts 2x slower (40000*2)
	TIMER0_ICR_R = 0x100;
	TIMER0_IMR_R |= 0x100;
	NVIC_PRI5_R = (NVIC_PRI5_R & 0xFFFFFF1F) | 0x00000060;	// make priority 3 (higher than priority of GPIOA interrupt)
	NVIC_EN0_R |= 0x00100000;	//enable bit 20 for Timer0B (int number 20)
	TIMER0_CTL_R |= 0x100;
}

// Cycle through columns one at a time periodically
void Timer0B_Handler(void){
	TIMER0_ICR_R |= 0x100; //clear interrupt flag
	GPIO_PORTE_DATA_R = (GPIO_PORTE_DATA_R<<1)&0x0F;	//set next column high
	if ((GPIO_PORTE_DATA_R&0x0F) == 0){		// if no columns set high, set column 0 high
		GPIO_PORTE_DATA_R = 0x01;
	}
}

// When key is pressed, see which row/column it was to determine which key was pressed in matrix
void GPIOPortA_Handler(void){
	unsigned char row, col, index;
	row = col = 0;
	index = GPIO_PORTE_DATA_R&0x0F;		// See which column
	while(index/2){
		index = index/2;
		col++;
	}
	index = (GPIO_PORTA_RIS_R&0x3C)>>2;		// See which row
	while(index/2){
		index = index/2;
		row++;
	}
	GPIO_PORTA_ICR_R = 0x3C; // Clear interrupt flag
	mail.data = KeyMap[row][col];		// Store key in data and set flag
	setFlag(1);
}

// Function for getting which key has been pressed
unsigned char getKey(void){
	return mail.data;
}

// Function for checking if new data is available
unsigned char keyPressed(void){
	return mail.intFlag;
}

// Function for setting flag when new data is available
void setFlag(unsigned char num){
	mail.intFlag = num;
}

// Disable keypad interrupts when transmitting data through FS1000A transmitter
void disableKeyPad(void){
	NVIC_DIS0_R = 0x00100000;	//disable bit 20 for Timer0B (int number 20)
	GPIO_PORTE_DATA_R &= ~0x0F;	//clear columns
}

// Re-enable keypad interrupts after transmitting data through FS1000A transmitter
void enableKeyPad(void){
	GPIO_PORTE_DATA_R |= 0x01;	//set column 0
	NVIC_EN0_R |= 0x00100000;	//enable bit 20 for Timer0B (int number 20)
}
