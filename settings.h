#ifndef SETTINGS_H
#define SETTING_H

//variables
extern int NREG;
extern int PMON;
extern int TSOM;
extern int TINA;
extern int VART;
extern int VARL;

extern unsigned int reg_buffer_pos;	//Stores current position in ring buffer


//Function Prototypes
void storeSettings();
void loadSettings();

#endif