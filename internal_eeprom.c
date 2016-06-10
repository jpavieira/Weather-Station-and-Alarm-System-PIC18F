#include <p18f452.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal_eeprom.h"

//Write data to internal eeprom
void internal_eeprom_write(unsigned char address, unsigned char data) {
	unsigned char INTCON_SAVE;

	EEADR  = address;
	EEDATA = data;

	EECON1bits.EEPGD= 0; //0 to access data in internal EEPROM memory
	EECON1bits.CFGS = 0; //0 to access data in internal EEPROM memory
	EECON1bits.WREN = 1; //enable writing to internal EEPROM

	INTCON_SAVE=INTCON; //save INTCON register content to restore them after
	INTCON=0; 			//disable interrupts
	
	EECON2=0x55;		// Required sequence for write to internal EEPROM, interrupts must be disabled
	EECON2=0xaa;		// Required sequence for write to internal EEPROM, interrupts must be disabled

	EECON1bits.WR=1;    // begin write to internal EEPROM
	INTCON=INTCON_SAVE; //restore INTCON
	
	Nop();

	while (PIR2bits.EEIF==0)	//wait for writing to complete
	{
		Nop();
	}

	EECON1bits.WREN=0; //disable writing to internal EEPROM after write complete+
	PIR2bits.EEIF=0; //clear internal EEPROM write complete flag
}


//Read data from the internal eeprom
unsigned char internal_eeprom_read(unsigned char address) {
    Nop();	//these Nops are needed
    Nop();
    EEADR=address;	//Address to read from
	EECON1bits.EEPGD= 0; //0 to access data in internal EEPROM memory
	EECON1bits.CFGS = 0; //0 to access data in internal EEPROM memory
    EECON1bits.RD   = 1; //Read
   	return EEDATA;       //read data is contained in EEDATA registerx
}