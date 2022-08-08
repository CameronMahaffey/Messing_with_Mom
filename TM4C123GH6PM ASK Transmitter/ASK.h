// Header file for ASK
// ASK.h
#include "stdint.h"

#ifndef __ASK_H__
#define __ASK_H__

// Structure for storing indices, flags, and tx data
typedef struct mailbox{
	uint8_t arrayIndex, nibbleIndex, cmdIndex, finished, flag, data;
} Mailbox;

// Initializes buffer variables to 0
void Timer0A_Init(uint32_t cycles);

/// The transmitter handler function, called at 2000Hz
void Timer0A_Handler(void);

// Initialize Timer0A for periodic interrupt mode to send data out of PB0 at 2000bps
void TxBuffer_Init(void);

// Each interrupt sends a single data bit at 2000bps, between interrupts this function sets next bit for transmission
void setTxData(void);

// Function for checking whether or not a transmission is finished or still in progress
// returns 1 if finished, 0 otherwise
uint8_t checkTxFinished(void);	

// Function for choosing which command to send
void setCmdIndex(uint8_t data);

// Function for enabling periodic interrupts for sending data bits through transmitter
void enableTx(void);

// Function for disabling periodic interrupts for sending data bits through transmitter
void disableTx(void);

#endif // __ASK_H__
