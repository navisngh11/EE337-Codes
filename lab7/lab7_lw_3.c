/**
 SPI HOMEWORK2 , LABWORK2 (SAME PROGRAM)
 */

/* @section  I N C L U D E S */
#include "at89c5131.h"
#include "stdio.h"
#define LCD_data  P2	    					// LCD Data port

void SPI_Init();
void LCD_Init();
void Timer_Init();
void LCD_DataWrite(char dat);
void LCD_CmdWrite(char cmd);
void LCD_StringWrite(char * str, unsigned char len);
void LCD_Ready();
void sdelay(int delay);
void delay_ms(int delay);

sbit CS_BAR = P1^4;									// Chip Select for the ADC
sbit LCD_rs = P0^0;  								// LCD Register Select
sbit LCD_rw = P0^1;  								// LCD Read/Write
sbit LCD_en = P0^2;  								// LCD Enable
sbit LCD_busy = P2^7;								// LCD Busy Flag
sbit ONULL = P1^0;
bit transmit_completed= 0;					// To check if spi data transmit is complete
bit offset_null = 0;								// Check if offset nulling is enabled
bit roundoff = 0;
int adcVal=0, avgVal=0, initVal=0, tempVal=0;
unsigned char serial_data;
unsigned char data_save_high;
unsigned char data_save_low;
unsigned char count=0, i=0;
unsigned char weight[4];           // Variable for strong final weight
float fweight=0;

/**

 * FUNCTION_INPUTS:  P1.5(MISO) serial input  
 * FUNCTION_OUTPUTS: P1.7(MOSI) serial output
 *                   P1.4(SSbar)
                     P1.6(SCK)
 */
 
void main(void)
{
	P3 = 0X00;											// Make Port 3 output 
	P2 = 0x00;											// Make Port 2 output 
	P1 &= 0xEF;											// Make P1 Pin4-7 output
	P0 &= 0xF0;											// Make Port 0 Pins 0,1,2 output
	
	SPI_Init();
	LCD_Init();
	Timer_Init();
	LCD_CmdWrite(0x80);
	LCD_StringWrite("Weight=",7);
	LCD_CmdWrite(0x8D);
	LCD_StringWrite("gm",2);
	while(1)												// endless 
	{ 
		CS_BAR = 0;                 // enable ADC as slave		 
		SPDAT= 0x01;								// Write start bit to start ADC 
		while(!transmit_completed);	// wait end of transmition;TILL SPIF = 1 
		transmit_completed = 0;    	// clear software transfert flag 
		
		SPDAT= 0x80;				// 80H written to start ADC CH0 single ended sampling
	  while(!transmit_completed);	// wait end of transmition 
	  data_save_high = serial_data & 0x03 ;  
		transmit_completed = 0;    	// clear software transfer flag 
		
		SPDAT= 0x00;								// 
		while(!transmit_completed);	// wait end of transmition 
		data_save_low = serial_data;
		transmit_completed = 0;    	// clear software transfer flag 
		CS_BAR = 1;                	// disable ADC as slave
		 
    adcVal = (data_save_high <<8) + (data_save_low);
		//adcVal=1111;
	}
}	


/**
 * FUNCTION_PURPOSE:interrupt
 * FUNCTION_INPUTS: void
 * FUNCTION_OUTPUTS: transmit_complete is software transfer flag
 */
void it_SPI(void) interrupt 9 /* interrupt address is 0x004B */
{
	switch	( SPSTA )         /* read and clear spi status register */
	{
		case 0x80:	
			serial_data=SPDAT;   /* read receive data */
      transmit_completed=1;/* set software flag */
 		break;

		case 0x10:
         /* put here for mode fault tasking */	
		break;
	
		case 0x40:
         /* put here for overrun tasking */	
		break;
	}
}

void timer0_ISR (void) interrupt 1
{ 
	//TR0=0;
	if(count==10){
		fweight=tempVal;
		fweight/=10;                                        //Average Value
    //avgVal=(avgVal*3.3)/1024;
		fweight*=(float)6.63;                             //For Scaling
		avgVal=(int)fweight;
		avgVal-=84;                                       //Zero error correction
		weight[0]=avgVal/1000;                            //Compute Individual weight values
		weight[1]=(avgVal-(weight[0]*1000))/100;
		weight[2]=(avgVal-(weight[0]*1000)-(weight[1]*100))/10;
		weight[3]=avgVal-(weight[0]*1000)-(weight[1]*100)-(weight[2]*10);
		weight[0]+=48;
		weight[1]+=48;
		weight[2]+=48;
		weight[3]+=48;
		sdelay(100);  
		LCD_CmdWrite(0x88);
		LCD_StringWrite(weight,4);                       //write weight values to LCD			
		tempVal=0;
		count=0;
	}
	else{
		tempVal+=adcVal;
		//take_samples=1;
	  count++;
	}
	
	TH0 = 0x3C;											//For 25ms operation
	TL0 = 0xB0;
	//TR0=1;
}


/**

 * FUNCTION_INPUTS:  P1.5(MISO) serial input  
 * FUNCTION_OUTPUTS: P1.7(MOSI) serial output
 *                   P1.4(SSbar)
                     P1.6(SCK)
 */ 
void SPI_Init()
{
	CS_BAR = 1;	                  	// DISABLE ADC SLAVE SELECT-CS 
	SPCON |= 0x20;               	 	// P1.1(SSBAR) is available as standard I/O pin 
	SPCON |= 0x01;                	// Fclk Periph/4 AND Fclk Periph=12MHz ,HENCE SCK IE. BAUD RATE=3000KHz 
	SPCON |= 0x10;               	 	// Master mode 
	SPCON &= ~0x08;               	// CPOL=0; transmit mode example|| SCK is 0 at idle state
	SPCON |= 0x04;                	// CPHA=1; transmit mode example 
	IEN1 |= 0x04;                	 	// enable spi interrupt 
	EA=1;                         	// enable interrupts 
	SPCON |= 0x40;                	// run spi;ENABLE SPI INTERFACE SPEN= 1 
}
	/**
 * FUNCTION_PURPOSE:Timer Initialization
 * FUNCTION_INPUTS: void
 * FUNCTION_OUTPUTS: none
 */

void Timer_Init()
{
	// Set Timer0 to work in up counting 16 bit mode. Counts upto 
	// 65536 depending upon the calues of TH0 and TL0
	// The timer counts 65536 processor cycles. A processor cycle is 
	// 12 clocks. FOr 24 MHz, it takes 65536/2 uS to overflow
	// By setting TH0TL0 to 3CB0H, the timer overflows every 25 ms
	
	TH0 = 0x3C;											//For 25ms operation
	TL0 = 0xB0;
	TMOD = (TMOD & 0xF0) | 0x01;  	// Set T/C0 Mode 
	ET0 = 1;                      	// Enable Timer 0 Interrupts 
	TR0 = 1;                      	// Start Timer 0 Running 
	count =0;
}
	/**
 * FUNCTION_PURPOSE:LCD Initialization
 * FUNCTION_INPUTS: void
 * FUNCTION_OUTPUTS: none
 */
void LCD_Init()
{
  sdelay(100);
  LCD_CmdWrite(0x38);   	// LCD 2lines, 5*7 matrix
  LCD_CmdWrite(0x0E);			// Display ON cursor ON  Blinking off
  LCD_CmdWrite(0x01);			// Clear the LCD
  LCD_CmdWrite(0x80);			// Cursor to First line First Position
}

/**
 * FUNCTION_PURPOSE: Write Command to LCD
 * FUNCTION_INPUTS: cmd- command to be written
 * FUNCTION_OUTPUTS: none
 */
void LCD_CmdWrite(char cmd)
{
	LCD_Ready();
	LCD_data=cmd;     			// Send the command to LCD
	LCD_rs=0;         	 		// Select the Command Register by pulling LCD_rs LOW
  LCD_rw=0;          			// Select the Write Operation  by pulling RW LOW
  LCD_en=1;          			// Send a High-to-Low Pusle at Enable Pin
  sdelay(5);
  LCD_en=0;
	sdelay(5);
}

/**
 * FUNCTION_PURPOSE: Write Command to LCD
 * FUNCTION_INPUTS: dat- data to be written
 * FUNCTION_OUTPUTS: none
 */
void LCD_DataWrite( char dat)
{
	LCD_Ready();
  LCD_data=dat;	   				// Send the data to LCD
  LCD_rs=1;	   						// Select the Data Register by pulling LCD_rs HIGH
  LCD_rw=0;    	     			// Select the Write Operation by pulling RW LOW
  LCD_en=1;	   						// Send a High-to-Low Pusle at Enable Pin
  sdelay(5);
  LCD_en=0;
	sdelay(5);
}
/**
 * FUNCTION_PURPOSE: Write a string on the LCD Screen
 * FUNCTION_INPUTS: 1. str - pointer to the string to be written, 
										2. length - length of the array
 * FUNCTION_OUTPUTS: none
 */
void LCD_StringWrite( char * str, unsigned char length)
{
    while(length>0)
    {
        LCD_DataWrite(*str);
        str++;
        length--;
    }
}

/**
 * FUNCTION_PURPOSE: To check if the LCD is ready to communicate
 * FUNCTION_INPUTS: void
 * FUNCTION_OUTPUTS: none
 */
void LCD_Ready()
{
	LCD_data = 0xFF;
	LCD_rs = 0;
	LCD_rw = 1;
	LCD_en = 0;
	sdelay(5);
	LCD_en = 1;
	while(LCD_busy == 1)
	{
		LCD_en = 0;
		LCD_en = 1;
	}
	LCD_en = 0;
}

/**
 * FUNCTION_PURPOSE: A delay of 15us for a 24 MHz crystal
 * FUNCTION_INPUTS: void
 * FUNCTION_OUTPUTS: none
 */
void sdelay(int delay)
{
	char d=0;
	while(delay>0)
	{
		for(d=0;d<5;d++);
		delay--;
	}
}

/**
 * FUNCTION_PURPOSE: A delay of around 1000us for a 24MHz crystel
 * FUNCTION_INPUTS: void
 * FUNCTION_OUTPUTS: none
 */
void delay_ms(int delay)
{
	int d=0;
	while(delay>0)
	{
		for(d=0;d<382;d++);
		delay--;
	}
}