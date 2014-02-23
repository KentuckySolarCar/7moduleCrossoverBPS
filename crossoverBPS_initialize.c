
#include <adc.h>
#include <spi.h>
#include "ECANPoll.h"
#include "functions_ADC_JF.h"
#include "crossoverBPS.def"

#include "crossoverBPS_initialize.h"

//set the relays to off and set the direction registers
/*
void CBS_Initialize(void)
{
	//set to off before declaring it as an output
	//	this helps to ensure that the relay will never be set to on on power up
	cbsrelay = CBSOFF;
	
	//set the necessary pins to output
	TRISC &= 0b11111110;//RC0 is the relay drive line (set to output (0))
	//TRISC &= 0xFE;
	
	return;
}//*/

//sets the LED's to off initially and sets the direction registers
void LED_Initialize(void)
{
	//set to off before declaring it as an output
	//	(just good practice with outputs)
	led1 = LEDOFF;
	led2 = LEDOFF;

	//RA4 are the leds (set to output (0))
	TRISA &= 0b11001111;
	TRISB &= 0b11011111;
}

//opens the CAN port and reads the dip switch settings to determine the slave's address
//outputs the can address
void CAN_Initialize(void)
{
	//ensure proper TRIS settings
	TRISB |= 0b00001000;//set the CAN input (RB3) to input
	TRISC &= 0b11111011;//set the CAN output (RB2) to output
	//actually start the CAN module
	ECANInitialize();
	////was in Krishna's code (not sure if it is necessary or not)//////
	// defined ECANPoll.c 
  	CIOCONbits.ENDRHI =1;
	//////////
	return;
}

//sets up the analog to digital converter
void ADC_Initialize(void)
{
	/////stole Krishna's code for setting up the ADC/////////
	/*these were in Krishna's code, but I'm not entirely certian what needs
		//to be a 1 and what needs to be a zero
    TRISA = 0x0F;
  	TRISB = 0x08;
  	TRISC = 0x80;
  	TRISD = 0x00;
  	TRISE = 0x00;//*/
	
	TRISA |= 0x0F;		//need to ensure that the references are set to inputs
	ADCON1 = 0x3B;  //ADCON1 to now accept external reference voltage
		
	OpenSPI(SPI_FOSC_64,MODE_00,SMPMID); // 
  	// Configure ADC - External
	OpenADC( ADC_FOSC_32      &     // OpenADC (ADC_"V3", 4, 5, 6)
    ADC_RIGHT_JUST   &     // Be sure I/O is config corectly - TRISA inc.
    ADC_12_TAD,
    ADC_CH0          &
	ADC_VREFPLUS_EXT	&
	ADC_VREFMINUS_EXT	&
    ADC_INT_OFF,  ADC_4ANA   );
	return;
}