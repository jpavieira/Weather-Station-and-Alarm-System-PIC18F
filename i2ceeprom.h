#ifndef I2CEEPROM_H
#define I2CEEPROM_H


void writeToRingBuffer(unsigned char h, unsigned char m, unsigned char s, unsigned char T, unsigned char L);
void i2c_EEPROM_write_byte(unsigned char data, unsigned char eeprom_addr_msb, unsigned char eeprom_addr_lsb);
void i2c_EEPROM_write_array(unsigned char buffer[], unsigned int num_bytes, unsigned char eeprom_addr_msb, unsigned char eeprom_addr_lsb);
unsigned char i2c_EEPROM_read_byte( unsigned char eeprom_addr_msb, unsigned char eeprom_addr_lsb);


#endif