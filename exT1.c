/*
 * SCE
 * Projecto Weather Station and Alarm System parte 1
 * Desenvolvido por:
 * João Vieira 84833
 * Francisco Salgado 75438
 * André Xavier 72644
 */

#include <p18f452.h>  /* for the special function register declarations */
#include <timers.h>   
#include <xlcd.h>
#include <delays.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include <adc.h>
#include <portb.h>

#include "constants.h"
#include "settings.h"
#include "i2ceeprom.h"

//Menu states
#define CHANGE_INIT 0
#define CHANGE_HOURS 1
#define CHANGE_MINS 2
#define CHANGE_SECS 3
#define CHANGE_ALARM 4
#define CHANGE_ALARM_HOURS 5
#define CHANGE_ALARM_MINS 6
#define CHANGE_ALARM_SECS 7
#define CHANGE_ALARM_TEMPERATURE 8
#define CHANGE_ALARM_LUMINOSITY 9
#define CHANGE_TEMPERATURE 10
#define CHANGE_LUMINOSITY 11
#define TOGGLE_CLOCK_ALARM 12
#define TOGGLE_TEMPERATURE_ALARM 13
#define TOGGLE_LUMINOSITY_ALARM 14
#define TIME_ALARM 15
#define S2_btn !PORTAbits.RA4 //button active low

//Absolute value macro
#define ABS(x) ((x) > 0 ? (x) : -(x)) 


unsigned int current_state=0;//initialize menu in CHANGE_HOURS mode

void ClockRoutine(void);
void checkAlarm();
void SoundBuzzer (char);
//global variables
volatile unsigned char sec=0, hour=0, min=0, read_trigger=0, refresh_lcd=0, user_activity_flag=0;
unsigned int lum=0, buffer_int=0;
unsigned char temp=0;
unsigned char prev_temp = 0, prev_lum = 0;
int S3_btn=0;

//Settings

//Settings
int NREG = 0;
int PMON = 0;
int TSOM = 0;
int TINA = 0;
int VART = 0;
int VARL = 0;

//alarm
unsigned int alarm_hour=0, alarm_min=0, alarm_sec=0, alarm_lum=0, alarm_temp=0;
unsigned char flag_clock_alarm='a';
unsigned char flag_temperature_alarm='t';
unsigned char flag_luminosity_alarm='l';
unsigned int buzzer_time = 0; //in seconds

void isr (void);  /* prototype needed for 'goto' below */
void checkTempVariation();	
void checkLumVariation();


/*
 * For high interrupts, control is transferred to address 0x8.
 */
#pragma code HIGH_INTERRUPT_VECTOR = 0x8
void high_ISR (void) 
{
  _asm
    goto isr
  _endasm
}
#pragma code  /* allow the linker to locate the remaining code */

#pragma interrupt isr

void isr (void) {

	if(PIR1bits.ADIF){//interrupt from ADC
		prev_lum = lum;
		lum = (unsigned int)ReadADC()*5.0/1023;
		checkLumVariation();
		PIR1bits.ADIF=0;//clear flag
	}

	if(PIR1bits.TMR1IF){//interrupt from Timer 1
		WriteTimer1(0x8000);       // reload timer: 1 second
		//PIR1bits.TMR1IE = 0;      /* clear flag to avoid another interrupt */
		sec++;
		++read_trigger;
		refresh_lcd++;
		user_activity_flag++;
		ClockRoutine();
		
		if (buzzer_time>0) {
			SoundBuzzer(1);
			buzzer_time--;
		} else {
			buzzer_time=0;
		}
		
		PIR1bits.TMR1IF = 0; //reset interrupt flag
  		//PIR1bits.TMR1IE = 1; //enable timer interrupt
  	}

	if(INTCONbits.INT0IF){//S3 button interrupt when pressed	
		S3_btn++;
		if(user_activity_flag>TINA){
			while( BusyXLCD() );
			WriteCmdXLCD( DON );
			user_activity_flag=0;
			S3_btn=0;
		}	
		INTCONbits.INT0IF=0;//reset flag
	}
	
  
}

void EnableHighInterrupts (void){
  RCONbits.IPEN = 1;    /* enable interrupt priority levels */
  INTCONbits.GIEH = 1;  /* enable all high priority interrupts */
}

void DelayFor18TCY( void ){
  Nop();Nop();Nop();
  Nop();Nop();Nop();
  Nop();Nop();Nop();
  Nop();Nop();Nop();
}

void DelayPORXLCD( void ){
  Delay1KTCYx(60);
  return;
}

void DelayXLCD( void ){
  Delay1KTCYx(20);
  return;
}

void InitializeBuzzer (void){
    T2CON = 0x05;		/* postscale 1:1, Timer2 ON, prescaler 4 */
    TRISCbits.TRISC2 = 0;  /* configure the CCP1 module for the buzzer */
    PR2 = 0x80;		/* initialize the PWM period */
    CCPR1L = 0x80;		/* initialize the PWM duty cycle */
	CCP1CON=0;
}

void SoundBuzzer (char enable){
    
    if (enable)
    	CCP1CON = 0x0F;   // turn the buzzer on 
	else 
		//CCP1CON = ~CCP1CON & 0x0F;    
		CCP1CON=0x0;
}

void ClockRoutine(void){
	if (sec==60){
		sec=0;
		min++;
	}
	if (min==60){
		min=0;
		hour++;
	}
	if (hour==24){
		hour=0;
	}
}

void ADC_init(){
	/* FOSC/32 clock select */
	ADCON0bits.ADCS1 = 1;
	ADCON0bits.ADCS0 = 0;
	
	//Vref=VSS/VDD; right justified
	ADCON1 = 0b10001110;
	
	PIE1bits.ADIE=1;//enable adc interrupt
	PIR1bits.ADIF=0;//reset interrupt flag
	IPR1bits.ADIP=1;//high priority
	INTCONbits.PEIE=0;// Enables all unmasked peripheral interrupts
	
	/* Turn on the ADC */
	ADCON0bits.ADON = 1;
	Delay10TCYx( 5 );
}


unsigned char tc74 (void){
	unsigned char value;
	
	OpenI2C(MASTER, SLEW_OFF);// Initialize I2C module
	SSPADD = 9; //100kHz Baud clock(9) -> 100k=FOSC / (4 * (SSPADD+1)), FOSC=4MHz 	
	
	//Delay10KTCYx(1);

	do{
		IdleI2C();
		StartI2C(); IdleI2C();
		WriteI2C(0x9a); IdleI2C();//device address = 0x9a
		WriteI2C(0x01); IdleI2C();//R
		RestartI2C(); IdleI2C();
		WriteI2C(0x9a | 0x01); IdleI2C();
		value = ReadI2C(); IdleI2C();
		NotAckI2C(); IdleI2C();
		StopI2C();
	} while (!(value & 0x40));
		IdleI2C();
		StartI2C(); IdleI2C();
		WriteI2C(0x9a | 0x00); IdleI2C();
		WriteI2C(0x00); IdleI2C();
		RestartI2C(); IdleI2C();
		WriteI2C(0x9a | 0x01); IdleI2C();
		value = ReadI2C(); IdleI2C();
		NotAckI2C(); IdleI2C();
		StopI2C();
	
	CloseI2C();
	
	//limit temperature values 
	if (value==0xFF)
		value=0;
	else if (value>0x32)
		value=50;

	return value;

}



void checkAlarm() {
	char buffer[15];
	if (flag_clock_alarm=='A') {
		if (alarm_hour==hour && alarm_min==min && alarm_sec==sec) {
			//turn on lcd
			while( BusyXLCD() );
			WriteCmdXLCD( DON );
			user_activity_flag=0;
			//Trigger alarm
			buzzer_time=TSOM;
			while( BusyXLCD() );
			SetDDRamAddr(9);
			sprintf( buffer, "A");
			while( BusyXLCD() );
			putsXLCD(buffer); //
			S3_btn=-1;
		}else{
			if(buzzer_time==0){
			CCP1CON=0;
			}
			if(S3_btn==0){
				while( BusyXLCD() );
				SetDDRamAddr(9);
				sprintf( buffer, " ");
				while( BusyXLCD() );
				putsXLCD(buffer); //
				S3_btn=0;
			}
		}		
	}	
	if (flag_temperature_alarm=='T') {
		if (alarm_temp==temp) {
			//turn on lcd
			while( BusyXLCD() );
			WriteCmdXLCD( DON );
			user_activity_flag=0;
			//Trigger alarm
			buzzer_time=TSOM;
			while( BusyXLCD() );
			SetDDRamAddr(10);
			sprintf( buffer, "T");
			while( BusyXLCD() );
			putsXLCD(buffer); //
			S3_btn=-1;
		}else{
			if(buzzer_time==0){
				CCP1CON=0;
			}
			if(S3_btn==0){
				while( BusyXLCD() );
				SetDDRamAddr(10);
				sprintf( buffer, " ");
				while( BusyXLCD() );
				putsXLCD(buffer); //
				S3_btn=0;
			}
		}		
	}
	if (flag_luminosity_alarm=='L') {
		if (alarm_lum==lum) {
			//turn on lcd
			while( BusyXLCD() );
			WriteCmdXLCD( DON );
			user_activity_flag=0;
			//Trigger alarm
			buzzer_time=TSOM;
			while( BusyXLCD() );
			SetDDRamAddr(11);
			sprintf( buffer, "L");
			while( BusyXLCD() );
			putsXLCD(buffer); //
			S3_btn=-1;
		}else{
			if(buzzer_time==0){
				CCP1CON=0;
			}
			if(S3_btn==0){
				while( BusyXLCD() );
				SetDDRamAddr(11);
				sprintf( buffer, " ");
				while( BusyXLCD() );
				putsXLCD(buffer); //
				S3_btn=0;
			}
		}		
	}
}

void checkTempVariation() {
	if (ABS(temp-prev_temp) >= VART) {
		//Save currents vars to ring buffer
		writeToRingBuffer(hour, min, sec, temp, lum);
	}
}

void checkLumVariation() {
	if (ABS(lum-prev_lum) >= VARL) {
		//Save currents vars to ring buffer
		writeToRingBuffer(hour, min, sec, temp, lum);
	}
}

//--------------------------------------------------------------------------------------------

void main (void) {
	char buffer[15];
	
	//Load settings
	loadSettings();
	
	//Ports config
	TRISAbits.TRISA4 = 1;
	
//LCD config
	ADCON1 = 0x0E;                    // Port A: A0 - analog; A1-A7 - digital
  	OpenXLCD( FOUR_BIT & LINES_5X7 ); // 4-bit data interface; 2 x 16 characters
   	while( BusyXLCD() );
    WriteCmdXLCD( DOFF );      // Turn display off
    while( BusyXLCD() );
    WriteCmdXLCD( CURSOR_OFF );// Enable display with no cursor
    
//Timer1 config
  	OpenTimer1( TIMER_INT_ON   & T1_16BIT_RW    & T1_SOURCE_EXT  & T1_PS_1_1      & T1_OSC1EN_ON   & T1_SYNC_EXT_ON );
  	//No opentimer -> PROTEUS->T1_SYNC_EXT_ON PLACA->T1_SYNC_EXT_OFF para acordar com o sleep

  	WriteTimer1( 0x8000 ); // load timer: 1 second

//ADC init
	ADC_init();

//Buzzer init
	InitializeBuzzer();
 
//RB0 interrupt init
	OpenRB0INT(PORTB_CHANGE_INT_ON & PORTB_PULLUPS_ON & FALLING_EDGE_INT);
	
	EnableHighInterrupts();

  	while (1){        
			//ClockRoutine();
		if (S3_btn<=0){
			
			checkAlarm();//only check the alarms in normal mode
			
			if(read_trigger>=PMON){
				ConvertADC(); // Start conversion for luminosity
				prev_temp = temp;
				temp=tc74();
				checkTempVariation();
				read_trigger=0;//reset trigger
			}
			
			if(refresh_lcd && user_activity_flag<=TINA ){
				
				checkAlarm();//only check the alarms in normal mode
				
				SetDDRamAddr(0x40);        // Second line; first column
				sprintf( buffer, "%d C ", temp);
				while( BusyXLCD() );
				putsXLCD(buffer); // program memory 
				SetDDRamAddr(78);        // Second line; second last column
				sprintf( buffer, "L%d", lum);
				while( BusyXLCD() );
				putsXLCD(buffer); // program memory 
				
				//print time into LCD
				sprintf( buffer, "%02u:%02u:%02u", hour, min, sec);
			    SetDDRamAddr(0x00);        // First line, first column
			    while( BusyXLCD() );
				putsXLCD(buffer);
				refresh_lcd=0;
				//Sleep();
			}else if(user_activity_flag>TINA && buzzer_time==0){
				while( BusyXLCD() );
				WriteCmdXLCD( DOFF );
			}
			if(buzzer_time==0)//no alarm running
				Sleep();
			           
		}else{
		//Menu navigation
			switch (current_state) {
				case CHANGE_INIT:
					while( BusyXLCD() );
					SetDDRamAddr(9);
					sprintf( buffer, "ATL %c%c%c", flag_clock_alarm, flag_temperature_alarm, flag_luminosity_alarm);
					while( BusyXLCD() );
					putsXLCD(buffer); //
					
					while( BusyXLCD() );
					SetDDRamAddr(0x00);
					PIE1bits.TMR1IE=0;
					current_state=CHANGE_HOURS;
					break;

				case CHANGE_HOURS: //Blink cursor
					while( BusyXLCD() );
					SetDDRamAddr(0x00);
					//If S3 pressed, set current_state to CHANGE MINS
					if (S3_btn==2) {
						current_state = CHANGE_MINS;
						break;
					}	
					//If S2 pressed, increase hour
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						hour++;
						ClockRoutine();
						sprintf( buffer, "%02d", hour);
						while( BusyXLCD() );
						putsXLCD(buffer);
					}
					break;

				case CHANGE_MINS: //Blink cursor
					while( BusyXLCD() );
					SetDDRamAddr(3);	
						
					//If S3 pressed, set current_state to CHANGE MINS
					if (S3_btn==3) {
						current_state = CHANGE_SECS;
						break;
					}	
					//If S2 pressed, increase hour
					if (S2_btn){
						Delay10KTCYx( 30 );//debouce
						min++;
						ClockRoutine();
						
						sprintf( buffer, "%02d", min);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 
					}
					break;

				case CHANGE_SECS: //Blink cursor
					while( BusyXLCD() );
					SetDDRamAddr(6);	
						
					//If S3 pressed, set current_state to CHANGE alarms
					if (S3_btn==4) {
						current_state = CHANGE_ALARM;
						PIE1bits.TMR1IE=1;//enable clock counter
						break;
					}	
					//If S2 pressed, increase alarm_hour
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						sec++;
						ClockRoutine();
						
						sprintf( buffer, "%02d", sec);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 
					}
					break;

				case CHANGE_ALARM:
					while( BusyXLCD() );
					SetDDRamAddr(9);//cursor on A

					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						
						//print time into LCD
						sprintf( buffer, "%02u:%02u:%02u", alarm_hour, alarm_min, alarm_sec);
		    			SetDDRamAddr(0x00);        // First line, first column
		   				while( BusyXLCD() );
						putsXLCD(buffer);
						
						current_state = CHANGE_ALARM_HOURS;
						break;
					}
					if (S3_btn==5){
						//display alarm temperature on lcd
						while( BusyXLCD() );
						SetDDRamAddr(64);        // Second line; first column
						sprintf( buffer, "%d C ", alarm_temp);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 

						while( BusyXLCD() );
						SetDDRamAddr(10);//cursor on T
						current_state = CHANGE_ALARM_TEMPERATURE;
					}
					break;

				case CHANGE_ALARM_HOURS:
					while( BusyXLCD());
					SetDDRamAddr(0);
					sprintf(buffer,"%02d",alarm_hour);
					while( BusyXLCD() );
					putsXLCD(buffer); // program memory 
					
					if (S3_btn==5){
						current_state = CHANGE_ALARM_MINS;
						break;
					}
					
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						if(alarm_hour++ >= 23) alarm_hour=0;
					}
					break;
				
				case CHANGE_ALARM_MINS:
					while( BusyXLCD());
					SetDDRamAddr(3);
					sprintf(buffer,"%02d",alarm_min);
					while( BusyXLCD() );
					putsXLCD(buffer); // program memory 
					
					if (S3_btn==6){
						current_state= CHANGE_ALARM_SECS;
						break;
					}
					
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						if(alarm_min++ >= 59) alarm_min=0;
					}
					break;

				case CHANGE_ALARM_SECS:
					while( BusyXLCD());
					SetDDRamAddr(6);
					sprintf(buffer,"%02d",alarm_sec);
					while( BusyXLCD() );
					putsXLCD(buffer); // program memory 
					
					if (S3_btn==7){
						//display alarm temperature on lcd
						while( BusyXLCD() );
						SetDDRamAddr(64);        // Second line; first column
						sprintf( buffer, "%d C ", alarm_temp);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 

						while( BusyXLCD() );
						SetDDRamAddr(10);//cursor on T
						current_state= CHANGE_ALARM_TEMPERATURE;
						S3_btn=5;
						break;
					}
					
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						if(alarm_sec++ >= 59) alarm_sec=0;
					}
					break;

				case CHANGE_ALARM_TEMPERATURE:

					if (S3_btn==6){
						//recovers actual temperature value
						while( BusyXLCD() );
						SetDDRamAddr(64);        // Second line; first column
						sprintf( buffer, "%d C ", temp);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 

						//display alarm luminosity value
						while(BusyXLCD());
						SetDDRamAddr(78);//cursor blinking on Luminosity value
						sprintf( buffer, "L%d", alarm_lum);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 

						while(BusyXLCD());
						SetDDRamAddr(11); //cursor blinking on L
						current_state= CHANGE_ALARM_LUMINOSITY;
						break;
					}
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce

						while( BusyXLCD() );
						SetDDRamAddr(64);        // Second line; first column
						sprintf( buffer, "%d C ", alarm_temp);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 

						while( BusyXLCD() );
						SetDDRamAddr(64); //cursor blinking on Temperature value

						if(alarm_temp++ >= 50) alarm_temp=0;
					}
					break;

				case CHANGE_ALARM_LUMINOSITY:

					if (S3_btn==7){
						//recovers actual luminosity value
						while(BusyXLCD());
						SetDDRamAddr(78);//cursor blinking on Luminosity value
						sprintf( buffer, "L%d", lum);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 

						current_state= TOGGLE_CLOCK_ALARM;
						break;
					}
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce

						while(BusyXLCD());
						SetDDRamAddr(78);//cursor blinking on Luminosity value
						sprintf( buffer, "L%d", alarm_lum);
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 
						while(BusyXLCD());
						SetDDRamAddr(79);//cursor blinking on Luminosity value
						
						if(alarm_lum++ >= 5) alarm_lum=0;
					}
					break;
				
				case TOGGLE_CLOCK_ALARM:
					while(BusyXLCD());
					SetDDRamAddr(13);
					sprintf(buffer,"%c",flag_clock_alarm);
					while( BusyXLCD() );
					putsXLCD(buffer);
					
					if (S2_btn) {
						Delay10KTCYx( 30 );//debouce
						if(flag_clock_alarm=='A'){
							flag_clock_alarm='a';
						}else{
							flag_clock_alarm='A';
						}
					}
					
					if (S3_btn==8){
						current_state= TOGGLE_TEMPERATURE_ALARM;
						break;
					}
					break;

				case TOGGLE_TEMPERATURE_ALARM:
					while(BusyXLCD());
					SetDDRamAddr(14);
					sprintf(buffer,"%c",flag_temperature_alarm);
					while( BusyXLCD() );
					putsXLCD(buffer);
					
					if (S2_btn) {
						Delay10KTCYx( 30 );
						if(flag_temperature_alarm=='T'){
							flag_temperature_alarm='t';
						}else{
							flag_temperature_alarm='T';
						}
					}
					
					if (S3_btn==9){
						current_state= TOGGLE_LUMINOSITY_ALARM;
						break;
					}
					break;

				case TOGGLE_LUMINOSITY_ALARM:
					while(BusyXLCD());
					SetDDRamAddr(15);
					sprintf(buffer,"%c",flag_luminosity_alarm);
					while( BusyXLCD() );
					putsXLCD(buffer);
					
					if (S2_btn) {
						Delay10KTCYx( 30 );
						if(flag_luminosity_alarm=='L'){
							flag_luminosity_alarm='l';
						}else{
							flag_luminosity_alarm='L';
						}
					}
					
					if (S3_btn==10){
						
						while( BusyXLCD() );
						SetDDRamAddr(9);
						sprintf( buffer, "       ");// limpa o ATL do LCD
						while( BusyXLCD() );
						putsXLCD(buffer); // program memory 
						
						current_state= CHANGE_INIT;
						S3_btn=0;
						break;
					}
					break;
			}//end switch case
		}//end else
	
  	}//end while 1	
	
}// end main


