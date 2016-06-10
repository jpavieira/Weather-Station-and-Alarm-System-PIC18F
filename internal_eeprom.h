#ifndef INTERNAL_EEPROM_H
#define INTERNAL_EEPROM_H

//Write data to internal eeprom
void internal_eeprom_write(unsigned char address, unsigned char data);

//Read data from the internal eeprom
unsigned char internal_eeprom_read(unsigned char address);

#endif