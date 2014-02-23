#include "ECANPoll.h"
#include "adc.h"	//to allow channel to be changed easily
#include "crossoverBPS.def"
#include "functions_ADC_JF.h"
#include <spi.h>
#include <stdio.h>
#include "crossoverBPS_management.h"

void write_to_spi (unsigned long int spi_out_val);
void memcpy_reduced(void *output, void *input);

//reads the voltage and temperature and updates the variables accordingly
//inputs are the locations to the variables that hold the voltage and temperature
void moduleCheck(unsigned int *voltage, unsigned char *temp)
{
	unsigned long int spi_out_val;
	unsigned int temp_result = 0;
	unsigned int i = 0;
	unsigned int MYDELAY = 4000;	//just a delay to ensure that things happen
	unsigned char flag=0;
	//led2 = ~led2;	
	/////////VOLTAGE///////////
	//check the voltage (channel 0)
	//set the adc to channel 0 (C18 library function)
	SetChanADC(ADC_CH0);
	
	//disable interrupts while doing the conversion
	GLOBALINTERRUPTS = INTERRUPTDISABLE;
	for(i=0; i<MYDELAY; ++i);
	///////borrowed and modified Krishna's code/////////////
	i=0;
    for(spi_out_val = 0x2000; spi_out_val > 0;spi_out_val >>= 1)  
      	{
			write_to_spi(spi_out_val);//take care of the shift register (JF)
            flag++;

      		if(flag >= 2)
				{     		
					//do the conversion (will be storing the voltage of the reading in .1 mV)
					//2^15 = 32768 for the 15 bit resolution
					//10000 to display in .1 mV (that fits the 15 bit resolution very well
					voltage[i] = read15bitOversample() * ((float)( (float)(VREF_PLUS-VREF_MINUS) / (float)(32767) ) * 10000);
					
					flag = 0;
					i++;
				} 
		}
	spi_out_val = 0x0000;          
	write_to_spi(spi_out_val);    // to turn off all the optocouplers
	
	
	/////////////////////
	
	
	//enable interrupts until next conversion
	GLOBALINTERRUPTS = INTERRUPTENABLE;
	
	////////////TEMP/////////////
	//check the temperature (channel 1)
	//set the adc to channel 1 (C18 library function)
	SetChanADC(ADC_CH1);
	
	//disable interrupts while doing the conversion
	GLOBALINTERRUPTS = INTERRUPTDISABLE;
	//do the conversion (temp_result will be storing the voltage of the reading)
	//2**15 = 32768 for the 15 bit resolution
	//10000 to display in .1 mV
	
	//////// borrowed and modified from Krishna's code///////////
	for(i=0; i<7; ++i)
	{
		unsigned int j;
		LATD = i<<4;
		for(j=0; j<2000; ++j);
		
		temp_result = read15bitOversample() * ((float)( (float)(VREF_PLUS-VREF_MINUS) / (float)(32768) ) * 10000);
		
		//convert adc voltage reading to temperature
		if ( temp_result > 19900 ) temp[i] = 0;
		else if ( temp_result <= 19900 && temp_result > 18000) temp[i] = 2 ;
		else if ( temp_result <= 18000 && temp_result > 16100) temp[i] = 7;		
		else if ( temp_result <= 16100 && temp_result > 14300) temp[i] = 12;
		else if ( temp_result <= 14300 && temp_result > 12600) temp[i] = 17;
		else if ( temp_result <= 12600 && temp_result > 11000) temp[i] = 22;
		else if ( temp_result <= 11000 && temp_result > 9600) temp[i] = 27;
		else if ( temp_result <= 9600 && temp_result > 8300) temp[i] = 32;
		else if ( temp_result <= 8300 && temp_result > 7200) temp[i] = 37;
		else if ( temp_result <= 7200 && temp_result > 6200) temp[i] = 42;
		else if ( temp_result <= 6200 && temp_result > 5300) temp[i] = 47;
		else if ( temp_result <= 5300 && temp_result > 4600) temp[i] = 52;
		else if ( temp_result <= 4600 && temp_result > 3900) temp[i] = 57;
		else if ( temp_result <= 3900 && temp_result > 3600) temp[i] = 60;
		else if ( temp_result <= 3600 && temp_result > 3400) temp[i] = 62;
		else if ( temp_result <= 3400) temp[i] = 67;
	
	}
	/////////////////////////////////////////////////////////////////////
	//enable interrupts again
	GLOBALINTERRUPTS = INTERRUPTENABLE;
	
	return;
}
//send the voltage and temperature values to the master BPS
//inputs are the canID, voltage, and temperature
void sendValues(unsigned int *voltage, unsigned char *temp, unsigned char boardNum, unsigned char activenumber)
{
	unsigned char dataRecieved[8];	//maximum length that can be recieved
	unsigned char lengthRecieved, flagsRecieved;
	unsigned long addressRecieved = 0;
	
	unsigned char send_data[3];
	unsigned char i = 0;
	unsigned long int send_address = 0;
	unsigned int timeout = 0;
	for(i=0; i<activenumber; ++i)
	{
		//message identifier will need reading bit appended 
		//	original line was:
		//	unsigned long int send_address = ((MASK_BPS_SLAVE & my_CAN_id) | MASK_BPS_READING);

		//the module number will be given by the combination of the board number 
		//	and the count that we are on
		//module number = my_CAN_id
		//module number = (((boardNum * 7) + i) << 2);
		send_address = ((MASK_BPS_SLAVE & (((boardNum * 7) + i) << 2)) | MASK_BPS_READING);
		
		//an unsigned int is 2 chars
		//an unsigned char is 1 char (obviously)
		// total data size will be size of 3 chars
		memcpy_reduced(send_data, &(voltage[i]));
		send_data[2] = temp[i];
		
		
		//LED1 = LEDON;
		while(addressRecieved != (MASK_BPS_MASTER | send_address) && timeout < 100)
		{
			int timeout2 = 0;
			//send the message until it is acknowledged by something
			while(!ECANSendMessage(send_address, send_data, 3, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME)&& timeout2 < 100) timeout2++;
			//check for a reply from the master acknowleging the message
			timeout2=0;
			while(!ECANReceiveMessage(&addressRecieved, &dataRecieved, &lengthRecieved, &flagsRecieved) && timeout2 < 100) timeout2++;
			timeout++;
			
			
		}
		//LED1 = LEDOFF;
	}
	
	return;
}

//check for new CAN messages and if new messages are found then react accordingly
unsigned char checkMessages(unsigned char boardNum, unsigned char activenumber, unsigned int *voltage, unsigned char *temp)
{
	unsigned char dataReceived[8];	//maximum length that can be recieved
	unsigned char lengthReceived, flagsReceived;
	unsigned long addressReceived;
	unsigned char slaveReceived;
	unsigned char array_index = 0;
	unsigned char sent_flag = 0;
	//while there are still messages in the buffer
	
	while(ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived))
	{
		//if I get here then I have recieved a message
		//Now parse the identifier and decide what I need to do
		
		//if it is a request for values
		slaveReceived = (addressReceived & MASK_BPS_SLAVE) >> 2;
		array_index = slaveReceived - boardNum*7;
		if ( ((addressReceived | MASK_BPS_SLAVE) == (MASK_BPS_MASTER | MASK_BPS_READING | MASK_BPS_SLAVE)) && slaveReceived >= boardNum*7 && slaveReceived < (boardNum*7+activenumber) )
		{
			unsigned char send_data[3];
			//message identifier will need reading bit appended
			unsigned long int send_address = ((MASK_BPS_SLAVE & (slaveReceived << 2)) | MASK_BPS_READING);
			//an unsigned int is 2 chars
			//an unsigned char is 1 char (obviously)
			// total data size will be size of 3 chars
			memcpy_reduced(send_data, &(voltage[array_index]));
			send_data[2] = temp[array_index];
			
			//led1 = LEDON;
			//send the message until it is acknowledged by something
			while(!ECANSendMessage(send_address, send_data, 3, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
			//led1 = LEDOFF;
			sent_flag = slaveReceived;
		}
		//if it is a master cbs message meant for this board
	}
	
	return sent_flag;
}

//ripoff of memcpy so that I don't have to include the string library
void memcpy_reduced(void *output, void *input)
{
	*(char *)output = *(char *)input;
	*(char *)((char *)output+1) = *(char *)((char *)input+1);
	return;
}

////////function taken from Krishna's code and modified////////////

void write_to_spi (unsigned long int spi_out_val)
{
	int i;
	for (i=0; i<5000; ++i);
	LATBbits.LATB4 = 1;        // Shifter's output disabled
	for (i=0; i<1000; ++i);
	WriteSPI(~spi_out_val >> 8);
	LATBbits.LATB0 = 1;         // Storage register clock pulse
	Nop();     
	LATBbits.LATB0 = 0;         // Storage register clock pulse
	WriteSPI(~spi_out_val);
	LATBbits.LATB0 = 1;        // Storage register clock pulse
	Nop();    
	LATBbits.LATB0 = 0;        // Storage register clock pulse
	LATBbits.LATB4 = 0;        // Shift Register Outputs Enabled	
	return;
}
////////////////////////////////////////////////////////////////////

//this function was made for the crossover code to be able to print out the data the same way
//	 that the telemetry board will
void printValues(unsigned int *voltage, unsigned char *temp, unsigned char boardNum, unsigned char activenumber)
{
	unsigned char i = 0;
	unsigned char modulenum = 0;
	for(i=0; i<activenumber; ++i)
	{
		modulenum = ((boardNum * 7) + i);
		printf("V[%.2d]=%u\n\r", modulenum, voltage[modulenum]);
		printf("T[%.2d]=%.2d\n\r", modulenum, temp[modulenum]);
	}
	return;
}
