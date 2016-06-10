#include <p18f452.h>
#include <i2c.h>
#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
#include "settings.h"
#include "internal_eeprom.h"

#define base_addr 0x00

//Setting are stored in internal eeprom as:
//0 NREG
//1 PMON
//2 TSOM
//3 TINA
//4 VART
//5 VARL
//6 reg_buffer_pos

void storeSettings() {
	internal_eeprom_write(base_addr, NREG);
	internal_eeprom_write(base_addr + 1, PMON);
	internal_eeprom_write(base_addr + 2, TSOM);
	internal_eeprom_write(base_addr + 3, TINA);
	internal_eeprom_write(base_addr + 4, VART);
	internal_eeprom_write(base_addr + 5, VARL);
	
	internal_eeprom_write(base_addr + 6, reg_buffer_pos);
}


void loadSettings() {
	NREG = internal_eeprom_read(base_addr);
 	PMON = internal_eeprom_read(base_addr + 1);
	TSOM = internal_eeprom_read(base_addr + 2);
	TINA = internal_eeprom_read(base_addr + 3);
	VART = internal_eeprom_read(base_addr + 4);
	VARL = internal_eeprom_read(base_addr + 5);

	reg_buffer_pos = internal_eeprom_read(base_addr + 6);

	if (NREG==0xFF || PMON==0xFF || TSOM==0xFF || TINA==0xFF || VART==0xFF || VARL==0xFF || reg_buffer_pos==0xFF) {
		//Values have not been stored before;
		NREG = DEFAULT_NREG;
	 	PMON = DEFAULT_PMON;
		TSOM = DEFAULT_TSOM;
		TINA = DEFAULT_TINA;
		VART = DEFAULT_VART;
		VARL = DEFAULT_VARL;
		reg_buffer_pos = 0;
		storeSettings();
	}
}
	