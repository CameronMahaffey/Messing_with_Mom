// main.c

/* The main purpose of this program is to transmit commands from the TM4C123GH6PM to an Arduino. The commands 
 * are sent by pressing the keys 0-6 on a 4x4 keypad matrix, through cheap 433MHz tx and rx modules. The 433MHz
 * modules operate through amplitude shift keying (ASK), and I wanted to make use of the Radiohead library on 
 * the Arduino. The Radiohead library encodes each message into a packet, so to receive the messages on the
 * Arduino, I reverse-engineered and hard-coded each packet into the TM4C for transmission. The default bit
 * rate of the library was 2000bps, so Timer0A is used to periodically send each bit of the packet during tx
 * at 2000Hz. There is approx. a 1s delay between sending commands.
 
 * Commands: '0' pressed sends 'hello' - message used for reverse-engineering and testing purposes
						 '1' pressed sends 'OnOff' 
						 '2' pressed sends 'Menus'
						 '3' pressed sends 'Ntflx'
						 '4' pressed sends 'Muted'
						 '5' pressed sends 'Power'
						 '6' pressed sends 'Swtch'
						 
 * Pinouts: PB0 - transmit data output pin - connected to data pin of FS1000A tx module
						GND/VCC - connected to GND/VCC of FS1000A tx module
						33cm dipole antenna - legs connected to ANT/GND of FS1000A tx module
						PA2,3,4,5 - keypad row input pins - connected to R1,R2,R3,R4 of 4x4 matrix keypad, respectively
						PE0,1,2,3 - keypad column output pins - connected to C1,C2,C3,C4 of 4x4 matrix keypad, respectively			 
 */
#include "stdint.h"
#include "ASK.h"
#include "keypad.h"
#include "..//tm4c123gh6pm.h"

#define CYCLES 7925			//~500 microseconds at 16MHz (2000bps) 8000 was a little too high
					  
// Functions defined in startup.s for interrupts
void EnableInterrupts(void);
void StartCritical(void);
void EndCritical(void);
void WaitForInterrupt(void);
												
void PortB0_Init(void);		// Initialize PortB0 as Transmitter data output
void delay_ms(uint32_t msecs);		// Non-critical time delay between inputs
void sendCommand(uint8_t data);		// Function for sending entire packet through PB0

int main(){
	uint8_t key;
	
	// Initialization Functions
	TxBuffer_Init();
	PortB0_Init();
	initializeKeyPad();
	Timer0A_Init(CYCLES);
	EnableInterrupts();
	
	while(1){
		while(keyPressed() == 0){		// Wait until a key has been pressed
			WaitForInterrupt();
		}
		disableKeyPad();						// Disable keypad interrupts during data transmission
		key = getKey() - 0x30;			// Turn ASCII '0'-'9' to number 0-9
		if(key < 7){								// Only 7 commands hard-coded
			sendCommand(key);	
			setFlag(0);								// Clear the flag after command is sent
		}
		delay_ms(1000);							// Wait 1 second between transmissions
		enableKeyPad();							// Re-enable keypad interrupts during data transmission
	}
}

// Initialize Port B Pin 0 as the output transmit pin
void PortB0_Init(void){
	SYSCTL_RCGCGPIO_R |= (1<<1); //Enable Port B peripheral clock
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R1) == 0);
	GPIO_PORTB_DIR_R |= 0x01;	// B0 output
	GPIO_PORTB_DEN_R |= 0x01; // B0 digital enable
	GPIO_PORTB_PCTL_R &= 0xFFFFFFF0;		// B0 general gpio 
	GPIO_PORTB_AFSEL_R &= ~0x01;		// B0 no alt function
	GPIO_PORTB_AMSEL_R &= ~0x01;		// B0 not analog
}

// Function for sending entire command through PB0
void sendCommand(uint8_t command){
	setCmdIndex(command);			// Set the command index for choosing between messages, depends on key pressed
	enableTx();								// Enable interrupts for sending data through transmitter
	while(checkTxFinished() == 0){		// While there is still data to transmit
		setTxData();										// Set the next bit and wait for interrupt to send it
		WaitForInterrupt();
	}
	disableTx();							// Disable interrupts after all data is sent through transmitter
}

// Simple delay between interrupts, timing is not critical for this
void delay_ms(uint32_t msecs){	
	uint32_t i;
	StartCritical();		// Ensure that no interrupts can be triggered during delay
	while(msecs > 0){
		i=2667;
		while(i > 0){
			i--;
		}
		msecs--;
	}	
	EndCritical();			// Re-allow interrupts
}
