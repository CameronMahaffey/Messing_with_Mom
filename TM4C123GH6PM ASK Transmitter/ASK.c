// ASK.c
#include "stdint.h"
#include "ASK.h"
#include "..//tm4c123gh6pm.h"

// Added to beginning of each message prior to transmit
const uint8_t PreMsg[27] = {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5, 	//Preamble
														0x1,0xc,0xd,						  						//Start Symbol 
														0xb,0x1,0x5,													//Message Length
														0x2,0xc,0xb,													//Header TO
														0x2,0xc,0xb,													//Header FROM
														0xb,0x2,0xc,													//Header ID
														0xb,0x2,0xc};  												//Header FLAGS

// 2D array of hard-coded messages to transmit to Arduino
const uint8_t Messages[][21] = {{0x5,0xb,0x1,0x5,0xa,0x6,0x5,0x9,0x5,0x5,0x9,0x5,0x5,0x8,0xb,0x7,0x0,0xb,0xb,0x0,0xb}, 	//hello msg
																{0x6,0x8,0xb,0x5,0x9,0x3,0x6,0x8,0xb,0x5,0x9,0x6,0x5,0x9,0x6,0x6,0x6,0x9,0x7,0x2,0xa},	//OnOff msg
																{0x6,0x8,0xd,0x5,0xa,0x6,0x5,0x9,0x3,0x3,0xa,0x6,0x3,0xa,0xa,0xb,0x1,0x6,0x5,0x9,0xa},	//Menus msg
																{0x6,0x9,0x3,0x3,0x9,0xa,0x5,0x9,0x6,0x5,0x9,0x5,0x3,0xb,0x1,0x6,0xb,0x1,0xa,0x5,0x5}, 	//Ntflx msg
																{0x6,0x8,0xd,0x3,0xa,0x6,0x3,0x9,0xa,0x5,0xa,0x6,0x5,0x9,0xa,0x4,0xc,0xe,0x6,0x6,0xc},	//Muted msg
																{0x9,0xa,0xc,0x5,0x8,0xb,0x3,0x8,0xe,0x5,0xa,0x6,0x3,0xb,0x2,0xb,0x0,0xb,0x4,0xc,0xd},	//Power msg
																{0x9,0xa,0xa,0x3,0x8,0xe,0x3,0x9,0xa,0x5,0xa,0xa,0x5,0xb,0x1,0x3,0x6,0x6,0x3,0x7,0x2}};	//Swtch Msg
																
																	
////Send 'hello' format example
//const uint8_t HelloMsg[21] = {0x5,0xb,0x1,												//Message 'h'
//															0x5,0xa,0x6,												//Message 'e'
//															0x5,0x9,0x5,												//Message 'l'
//															0x5,0x9,0x5,												//Message 'l'
//															0x5,0x8,0xb,												//Message 'o'
//															0x7,0x0,0xb,0xb,0x0,0xb};						//CRC Frame Check

// Structure for storing indices, flags, and tx data
Mailbox TxBuffer;

															
// Initialize Timer0A for periodic interrupt mode to send data out of PB0 at 2000bps
void Timer0A_Init(uint32_t cycles){
	//SCTL_RCGCTIMER_R |= 0x0001;			// already set up in Timer0B_Init()
	//while((SYSCTL_PRTIMER_R&SYSCTL_PRTIMER_R0) == 0);
	//TIMER0_CTL_R = 0;
	//TIMER0_CFG_R = 0x04;
	TIMER0_TAMR_R = 0x02;
	TIMER0_TAILR_R = cycles - 1;
	TIMER0_TAPR_R = 0;
	TIMER0_ICR_R = 0x1;
	TIMER0_IMR_R |= 0x1;
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x1FFFFFFF) | 0x60000000;	// make priority 3 (higher than priority of GPIOA interrupt)
	//NVIC_EN0_R |= 0x00080000;	//enable bit 19 for Timer0A (int number 19)			// don't enable until needed
	TIMER0_CTL_R |= 1;
}


/// The transmitter handler function, called at 2000Hz
void Timer0A_Handler(void){
	GPIO_PORTB_DATA_R = TxBuffer.data&0x01;			// Send data bit out of Port B pin 0
	TIMER0_ICR_R |= 0x1;			// Clear interrupt
}

// Initializes buffer variables to 0
void TxBuffer_Init(void){
	TxBuffer.arrayIndex = 0;
	TxBuffer.nibbleIndex = 0;
	TxBuffer.finished = 0;
}

// Each interrupt sends a single data bit at 2000bps, between interrupts this function sets next bit for transmission
void setTxData(void){
	// Each command is 48 nibbles
	if(TxBuffer.arrayIndex > 47){		// If message is finished, set flag, reset array Index and data variables
		TxBuffer.finished = 1;
		TxBuffer.arrayIndex = 0;
		TxBuffer.data = 0;
	} 
	else {													// Otherwise, send command
		if(TxBuffer.arrayIndex < 27){		// First 27 nibbles are premessage
			TxBuffer.data = PreMsg[TxBuffer.arrayIndex]>>(3-TxBuffer.nibbleIndex++);
		}
		else {													// Last 21 nibbles are message and crc
			TxBuffer.data = Messages[TxBuffer.cmdIndex][TxBuffer.arrayIndex-27]>>(3-TxBuffer.nibbleIndex++);
		}
		if(TxBuffer.nibbleIndex > 3){		// Nibble is 4 bits long - sent MSB first
			TxBuffer.nibbleIndex = 0;
			TxBuffer.arrayIndex++;				// After all bits in nibble are sent, reset nibble index and increment array index
		}
	}
}

// Function for checking whether or not a transmission is finished or still in progress
// returns 1 if finished, 0 otherwise
uint8_t checkTxFinished(void){
	if(TxBuffer.finished == 1){
		TxBuffer.finished = 0;
		return 1;
	}
	return 0;
}

// Function for choosing which command to send
void setCmdIndex(uint8_t data){
	TxBuffer.cmdIndex = data;
}

// Function for enabling periodic interrupts for sending data bits through transmitter
void enableTx(void){
	NVIC_EN0_R |= 0x00080000;	//enable bit 19 for Timer0A (int number 19)
}

// Function for disabling periodic interrupts for sending data bits through transmitter
void disableTx(void){
	NVIC_DIS0_R = 0x00080000;	//enable bit 19 for Timer0A (int number 19)
}
