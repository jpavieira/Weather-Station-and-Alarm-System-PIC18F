#include "i2ceeprom.h"
#include "settings.h"

#include <p18f452.h>
#include <i2c.h>
#include <stdio.h>
#include <stdlib.h>

#define base_addr 0x00//0xF00000

unsigned int reg_buffer_pos = 0;

void writeToRingBuffer(unsigned char h, unsigned char m, unsigned char s, unsigned char T, unsigned char L) {
	int page_num = reg_buffer_pos / 12;
	int page_off = reg_buffer_pos % 12;

	unsigned char buffer[5];
	buffer[0]=h;
	buffer[1]=m;
	buffer[2]=s;
	buffer[3]=T;
	buffer[4]=L;
	
	i2c_EEPROM_write_array(buffer, 5, 0x00, page_num*64+page_off*5); 
	
	//Increment reg counter
	reg_buffer_pos>=NREG ? reg_buffer_pos=0 : reg_buffer_pos++;
	internal_eeprom_write(base_addr + 6, reg_buffer_pos); 	//for storing current position in ring buffer
}


void i2c_EEPROM_write_byte(unsigned char data, unsigned char eeprom_addr_msb, unsigned char eeprom_addr_lsb) {
	OpenI2C(MASTER, SLEW_OFF);// Initialize I2C module
	SSPADD = 9; //100kHz Baud clock(9) -> 100k=FOSC / (4 * (SSPADD+1)), FOSC=4MHz 	
	
	Delay10KTCYx(15);

	/* garante inactividade no barramento */
	IdleI2C();
	/* emite uma condição de START no barramento I2C */
	StartI2C();
	/* aguarda terminação da condição de START */
	while (SSPCON2bits.SEN);
	/* endereço da EEPROM no modo escrita */
	WriteI2C(0b10100000);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/* endereça os MSB do dispositivo no barramento I2C, para escrita */
	WriteI2C(eeprom_addr_msb);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/*  endereça os LSB do dispositivo no barramento I2C, para escrita*/
	WriteI2C(eeprom_addr_lsb);
	/* aguarda recepção de acknowledge do sensor */
	while (SSPCON2bits.ACKSTAT);

	WriteI2C(data);
	
	/* aguarda recepção de acknowledge do sensor */
	while (SSPCON2bits.ACKSTAT);
	/* Emite uma condição de STOP no barramento I2C */
	StopI2C();
	/* aguarda terminação da condição de STOP */
	while (SSPCON2bits.PEN);
	/* garante inactividade no barramento */
	IdleI2C();
}



void i2c_EEPROM_write_array(unsigned char buffer[], unsigned int num_bytes, unsigned char eeprom_addr_msb, unsigned char eeprom_addr_lsb) {
	int i = 0;
	OpenI2C(MASTER, SLEW_OFF);// Initialize I2C module
	SSPADD = 9; //100kHz Baud clock(9) -> 100k=FOSC / (4 * (SSPADD+1)), FOSC=4MHz 	
	
	Delay10KTCYx(15);

	/* garante inactividade no barramento */
	IdleI2C();
	/* emite uma condição de START no barramento I2C */
	StartI2C();
	/* aguarda terminação da condição de START */
	while (SSPCON2bits.SEN);
	/* endereço da EEPROM no modo escrita */
	WriteI2C(0b10100000);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/* endereça os MSB do dispositivo no barramento I2C, para escrita */
	WriteI2C(eeprom_addr_msb);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/*  endereça os LSB do dispositivo no barramento I2C, para escrita*/
	WriteI2C(eeprom_addr_lsb);
	/* aguarda recepção de acknowledge do sensor */
	while (SSPCON2bits.ACKSTAT);
	
	//Iterate through buffer array 
	for (i=0; i<num_bytes; i++) {
		WriteI2C(buffer[i]);
	}
	
	/* aguarda recepção de acknowledge do sensor */
	while (SSPCON2bits.ACKSTAT);
	/* Emite uma condição de STOP no barramento I2C */
	StopI2C();
	/* aguarda terminação da condição de STOP */
	while (SSPCON2bits.PEN);
	/* garante inactividade no barramento */
	IdleI2C();
}







unsigned char i2c_EEPROM_read_byte( unsigned char eeprom_addr_msb, unsigned char eeprom_addr_lsb) {
	unsigned char data = 0;

	OpenI2C(MASTER, SLEW_OFF);// Initialize I2C module
	SSPADD = 9; //100kHz Baud clock(9) -> 100k=FOSC / (4 * (SSPADD+1)), FOSC=4MHz 	
	
	Delay10KTCYx(15);

	/* garante inactividade no barramento */
	IdleI2C();
	/* emite uma condição de START no barramento I2C */
	StartI2C();
	/* aguarda terminação da condição de START */
	while (SSPCON2bits.SEN);
	/* endereço da EEPROM no modo escrita */
	WriteI2C(0b10100000);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/* endereça os MSB do dispositivo no barramento I2C, para escrita */
	WriteI2C(eeprom_addr_msb);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/*  endereça os LSB do dispositivo no barramento I2C, para escrita*/
	WriteI2C(eeprom_addr_lsb);
	/* aguarda recepção de acknowledge do sensor */
	while (SSPCON2bits.ACKSTAT);

	RestartI2C();
	/* aguarda terminação da condição de REPEATED START */
	while (SSPCON2bits.RSEN);
	/* endereço da EEPROM no modo escrita */
	WriteI2C(0b10100001);
	/* aguarda recepção de acknowledge */
	while (SSPCON2bits.ACKSTAT);
	/* leitura dos dados recebidos do sensor */
	data=ReadI2C();
	/* envia condição de NOT ACKNOWLEDGE para o barramento I2C */
	NotAckI2C();
	/* aguarda terminação da condição de NOT ACKNOWLEDGE */
	while (SSPCON2bits.ACKEN);
	/* emite uma condição de STOP no barramento I2C */
	StopI2C();
	/* aguarda terminação da condição de STOP */
	while (SSPCON2bits.PEN);
	/* garante inactividade no barramento */
	IdleI2C();

	return data;
}
