
#pragma config OSC = HS     // Oscillator 
#pragma config PWRT = OFF   // Power on delay 
#pragma config WDT = OFF    // WatchDog Timer
#pragma config LVP = OFF    // Low Voltage Programming OFF


#define INITIAL_DELAY_MULTIPLIER	1000
#include <p18f4480.h>
#include "crossoverBPS.def"
#include "crossoverBPS_management.h"
#include "crossoverBPS_initialize.h"

//indicates which of the 5 boards this would be
//	0 means it will act as modules 0-6
//	1 means it will act as modules 7-13
//	2 means it will act as modules 14-20
//	3 means it will act as modules 21-27
//	4 means it will act as modules 28-34
#define CAN_id_base	2
//this value indicates how many modules the board should read from
//	example a value of 3 will take readings from the first 3 spots 
//	on the board and label them as modules 0, 1, 2
#define ActiveModules	7

#include <usart.h>
#include <stdio.h>

void main (void)
{
	unsigned long int i, j;
	unsigned int voltage[7] = {32000,32000,32000,32000,32000,32000,32000};
	unsigned char temp[7] = {30,30,30,30,30,30,30};
	/////used assigment from Krishna's code /////////////
	////(assuming that it takes care of everything because I don't know what pins go where)//////
	TRISA = 0xEF;
	TRISB = 0x08;
	TRISC = 0x00;
	TRISD = 0x00;
	TRISE = 0x00;
	/////////////////////////////////////////////////////
	
	LED_Initialize();
	//turn on the red led while initializing to indicate if it freezes while starting
//	led2 = LEDON;
//led2 = LEDOFF;
	//this was easier than including the serial port file that I made
	OpenUSART( USART_TX_INT_OFF &
	USART_RX_INT_OFF &
	USART_ASYNCH_MODE &
	USART_EIGHT_BIT &
	USART_CONT_RX &
	USART_BRGH_HIGH,71);
	printf("\r\nSerial Port Online\r\n");
	printf("Board#: %d\r\nActiveModules: %d\r\n", CAN_id_base, ActiveModules);
//	led2 = LEDOFF;
	CAN_Initialize();
	
	ADC_Initialize();
	
	
	//ignoring initial delay for now until I find out that I need it
	//initial delay based on my_CAN_id
	//for(i=0; i<(INITIAL_DELAY_MULTIPLIER*my_CAN_id); ++i);

	while(1)
	{
		
		led2 = ~led2;//leaving off for real use to save power
		moduleCheck(voltage, temp);//read in new values
		printValues(voltage, temp, CAN_id_base, ActiveModules);//publish results to serial port
		while(checkMessages(CAN_id_base, ActiveModules, voltage, temp) != (CAN_id_base*7 - 1 + ActiveModules));
			//for(i=j; j<4000; ++j);
		//update master if requested
		//sendValues(voltage, temp, CAN_id_base, ActiveModules);//update the master
	
		
		//we are going to need a delay in here I think
		//don't yet know how long I want the delay to be
		for(i=0; i<4000; ++i); //max for unsigned long is 4,294,967,295
	}

	return; //should never get here
}