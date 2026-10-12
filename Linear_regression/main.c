

/*
	  MEGA 			 ATmega2560			       LCD
	D 6 7 8 9		PORTH 3 4 5 6			D 4 5 6 7
	D22				PORTA 0					RS
	D23				PORTA 1					RW
	D24				PORTA 2					EN
*/ 

/* 
Color sensor		ATmega2560						MEGA
	S3	L				GND					
	S2	H				5V
	S1	H				5V
	S0	H				5V
	OE	L				GND
	OUT			PE4 external interrupt 4			D2
	
	(S2 S3) = (H L) = no filter
	(S0 S1) = (H H) = scaling 100%
*/ 

/*
	   Button			ATmega2560						MEGA
		Enter		PD1 external interrupt 5			D3
		Measure		PD2 external interrupt 2			D19
		calib		PD3 external interrupt 3			D18
*/

#ifndef F_CPU
#define F_CPU 16000000UL // telling controller crystal frequency
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>
#include "LCD_4bit.h"
#include "RW_EEPROM.h"
#include "linear_regression.h"
#include "SVR_RBF_model.h"
#include <avr/eeprom.h>
#include <math.h>
#include <stdlib.h>
#include <avr/wdt.h>

volatile unsigned long freq = 0; //luu so xung trong 1s
volatile unsigned long stableFreq = 0;
volatile unsigned long count = 0;// dem so xung tu sensor
// trung bình 5 lần
volatile unsigned long freqSum = 0;
volatile uint8_t freqCount = 0;

float F0 = 0.0;
float F_sample = 0.0;
uint8_t blankSaved=0;
char buf1[10], demiBuf1[10], demiBuf2[10], intBuf1[10], intBuf2[10];
int selectMode = -1;
unsigned int countlen, countlen1, firstint, restint; 
unsigned long tempFreq;

volatile uint8_t sampleCheck = 0;
volatile uint8_t measurePressed=0;
volatile uint8_t measureStep = 0;
volatile uint8_t resetRequest = 0;

ISR(TIMER1_OVF_vect)
{
	freqSum += count;

	freqCount++;

	if(freqCount >= 5)
	{
		freq = freqSum / 5;

		if(freq > 0)
		{
			stableFreq = freq;
		}

		freqSum = 0;

		freqCount = 0;
	}
	count = 0;

	TCNT1 = 3036;
}

// external interrupt 0 vector
ISR (INT4_vect)
{
	count++;
}
// external interrupt 1 vector
ISR (INT5_vect)	// calib reset
{
	

	resetRequest = 1;
}
// external interrupt 1 vector
ISR (INT1_vect) // measure
{
	
	if(selectMode==-1)
	{
		selectMode=1;

		measurePressed=0;
	}
	else
	{
		measurePressed=1;
	}
}
// external interrupt 3 vector
ISR (INT3_vect)	// calib
{
	if(selectMode==-1)
	{
		selectMode=0;
	}
	else if(selectMode==0)
	{
		sampleCheck=1;
		
	}
}

struct MenuLCD {
	char func[20];
	int funcCode;
};


int main(void)
{
    /* Replace with your application code */
	
	struct MenuLCD menuLCD[3];
	strcpy(menuLCD[0].func,"Calib Mode");
	menuLCD[0].funcCode = 0;
	strcpy(menuLCD[1].func,"Measure Mode");
	menuLCD[1].funcCode = 1;
	// timer 1s
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK1 = 0;

	// Setup Timer/Counter1 //
	TCCR1B |= (1 << CS12);								// prescale = 256
	TCNT1 = 3036;										// = 2^16 - time/(prescale/16M)
	TIMSK1 |= (1 << TOIE1);								// overflow interrupt is enabled
	
	// PIN for LCD
	DDRH |= ((1 << DDH3) | (1 << DDH4) | (1 << DDH5) | (1 << DDH6));							// port H3456 output
	PORTH &= ~ ((1 << PINH3) | (1 << PINH4) | (1 << PINH5) | (1 << PINH6));						// port H3456 LOW
	DDRA |= ((1 << DDA0) | (1 << DDA1) | (1 << DDA2));											// port A012 output
	PORTA &= ~ ((1 << PINA0) | (1 << PINA1) | (1 << PINA2));									// port A012 LOW
	
	// built-in led7
	DDRB |= (1 << DDB7);
	PORTB |= (1 << PINB7);
	
	// interrupt PD13 & PE45 (D19-18 & D1-3 MEGA - External interrupt)
	DDRD &= ~((1 << DDD1) | (1 << DDD3));							// port D13 input
	PORTD |= ((1 << PIND1) | (1 << PIND3));							// port D13 HIGH
	DDRE &= ~((1 << DDE4) | (1 << DDE5));							// port E45 input
	PORTE |= ((1 << PINE4) | (1 << PINE5));							// port E45 HIGH
	// => pull-up res PD23 & PE45
	EICRA = 0b10001000;																			// external interrupt at FALLING EDGE on PD13
	EICRB = 0b00001010;		
	_delay_ms(1000);

	EIFR = 0xFF;																	// external interrupt at FALLING EDGE on PE45
	EIMSK |= ((1 << INT1) | (1 << INT3) | (1 << INT4) | (1 << INT5));							// external interrupt INT12345 enable
	
	sei();			// enable global interrupt
	
	lcd_initialize();
	clearScreen();
	_delay_ms(1000);
// -------------------------------------------------------------------------//
	unsigned int temp1 = 0, temp2 = 0;
	unsigned long sampleData[5], sampleDataBuf = 0;
	float b0;
	float b1;

	float sampleY[5];

	

	char buf0[20]; 
	
	
	clearScreen();
	b0=EEPROM_read_float(100);

	b1=EEPROM_read_float(104);

	if(
	isnan(b0)
	||
	isnan(b1)
	||
	b1==0
	)
	{
		b0=0.09388;

		b1=0.00625;
	}
	for (int i = 0; i < 5; i++)
	{
		sampleData[i] = 0;
		sampleY[i] = 0.0;
	}
	temp1 = temp2 = 0;
	read_long_EEPROM(0,&temp1,&temp2);		// read 1st calib long number
	//+++++++++++++++++++++++++++++++++++++++++
	sampleDataBuf = temp1;
	sampleData[0] = sampleDataBuf;
	sampleDataBuf = 0;
	sampleDataBuf = temp2;
	sampleData[0] |= (sampleDataBuf << 16);
	F0 = (float)sampleData[0];
	blankSaved=1;
	

	// standard concentration
	// +++++++++++++++++++++++ Change the value ++++++++++++++
	sampleY[0] = 0.0;
	sampleY[1] = 10.0;
	sampleY[2] = 20.0;
	sampleY[3] = 30.0;
	sampleY[4] = 40.0;
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//
	_delay_ms(5000);
	//-------------------------
	clearScreen();
	
	//
	float sampleA[5];

	// sampleData[0] chính là F0

	for(int i = 0; i < 5; i++)
	{
		sampleA[i] = log10(
		(float)sampleData[0] /
		(float)sampleData[i]
		);
	}

	// regression:
	// A = b1*C + b0

	/*linear_regression(
	sampleA,
	sampleY,
	&b1,
	&b0
	);*/
	
// ---------------------------------------------------------------
	clearScreen();

	// X Y
	sprintf(
	buf0,
	"X:%lu",
	sampleData[0]
	);

	lcdwrite(
	buf0,
	0,
	0
	);

	sprintf(
	buf0,
	"Y:%lu",
	sampleData[1]
	);

	lcdwrite(
	buf0,
	1,
	0
	);

	_delay_ms(3000);

	clearScreen();

	// Z M
	sprintf(
	buf0,
	"Z:%lu",
	sampleData[2]
	);

	lcdwrite(
	buf0,
	0,
	0
	);

	sprintf(
	buf0,
	"M:%lu",
	sampleData[3]
	);

	lcdwrite(
	buf0,
	1,
	0
	);

	_delay_ms(3000);

	clearScreen();

	// N
	sprintf(
	buf0,
	"N:%lu",
	sampleData[4]
	);

	lcdwrite(
	buf0,
	0,
	0);

	_delay_ms(3000);

	clearScreen();

	// b0
	firstint=(int)b0;

	restint=
	abs(
	(int)(
	(b0-firstint)
	*100000
	)
	);

	sprintf(
	buf0,
	"b0:%d.%05d",
	firstint,
	restint
	);

	lcdwrite(
	buf0,
	0,
	0);

	// b1
	firstint=(int)b1;

	restint=
	abs(
	(int)(
	(b1-firstint)
	*100000
	)
	);

	sprintf(
	buf0,
	"b1:%d.%05d",
	firstint,
	restint
	);

	lcdwrite(
	buf0,
	1,
	0);

	_delay_ms(5000);

	clearScreen();
	while (1) 
    {	
		if(resetRequest)
		{
			resetRequest = 0;
			cli();

			freq = 0;

			count = 0;

			freqSum = 0;

			freqCount = 0;
			sei();

			sampleCheck = 1;

			measurePressed = 1;

			measureStep = 0;

			selectMode = -1;

			clearScreen();

			lcdwrite(
			"RESET DONE",
			0,
			0
			);

			_delay_ms(1000);

			clearScreen();
		}
// --------------------------------------------------------------
		lcdwrite(menuLCD[0].func,0,0);
		lcdwrite(menuLCD[1].func,1,0);	
// --------------------------------------------------------------		
		if (selectMode == 0) // calib mode
		{
			clearScreen();
			while((selectMode == 0))
			{
				setCursor(0,0);
				lcd_data(0b01111110); // right arrow
				lcdwrite(menuLCD[selectMode].func,0,2);
				// ----------------------------------------
				// Take sample X
				lcdwrite("                ",1,0);
				lcdwrite("X",1,0);
				setCursor(1,1);
				lcd_data(0b00111010); // ':' char
				sampleCheck = 0;
				

				_delay_ms(1000);
				while(sampleCheck == 0)
				{
					if(resetRequest)
					{
						selectMode = -1;

						sampleCheck = 1;
						break;
					}

					tempFreq = stableFreq;
					sprintf(
					buf0,
					"%lu",
					tempFreq
					);

					lcdwrite(
					"             ",
					1,
					3
					);

					lcdwrite(
					buf0,
					1,
					3
					);

					_delay_ms(100);
				}
				sampleCheck = 0;
				//
				sampleData[0] = 0;
// 				firstint = freq/10000;
// 				restint = freq - firstint*10000;
				

				tempFreq = stableFreq;
				firstint = (unsigned int)(tempFreq & 0xFFFF);

				restint = (unsigned int)(tempFreq >> 16);
				write_long_EEPROM(0,&firstint,&restint);		// write 1st calib long number
				itoa(firstint,buf0,10);					                                          	
				itoa(restint,buf1,10);
				countlen = strlen(buf0);
				//
				read_long_EEPROM(0,&temp1,&temp2);				// read 1st calib long number
				sampleDataBuf = temp1;
				sampleData[0] = sampleDataBuf;
				sampleDataBuf = 0;
				sampleDataBuf = temp2;
				sampleData[0] |= (sampleDataBuf << 16);
				lcdwrite("X wrote: ",1,0);
// 				itoa(temp1,buf0,10);							
// 				countlen = strlen(buf0);
// 				lcdwrite(buf0,1,9);
// 				itoa(temp2,buf1,10);
// 				lcdwrite(buf1,1,countlen+9);
				sprintf(buf0,"%lu",sampleData[0]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,9);
				_delay_ms(5000);
				// ----------------------------------------
				// Take sample Y
				firstint = restint = temp1 = temp2 = 0;
				lcdwrite("                ",1,0);
				lcdwrite("Y",1,0);
				setCursor(1,1);
				lcd_data(0b00111010); // ':' char
				sampleCheck = 0;
				

				_delay_ms(1000);
				while(sampleCheck == 0)
				{
					if(resetRequest)
					{
						selectMode = -1;

						sampleCheck = 1;
						break;
					}

					tempFreq = stableFreq;
					sprintf(
					buf0,
					"%lu",
					tempFreq
					);

					lcdwrite(
					"             ",
					1,
					3
					);

					lcdwrite(
					buf0,
					1,
					3
					);

					_delay_ms(100);
				}
				sampleCheck = 0;
				//
				sampleData[1] = 0;
				/*firstint = freq/10000;*/
				

				tempFreq = stableFreq;

				firstint = (unsigned int)(tempFreq & 0xFFFF);

				restint = (unsigned int)(tempFreq >> 16);
				write_long_EEPROM(4,&firstint,&restint);		// write 2nd calib long number
				itoa(firstint,buf0,10);					                                          		
				itoa(restint,buf1,10);
				countlen = strlen(buf0);
				read_long_EEPROM(4,&temp1,&temp2);				// read 2nd calib long number
				sampleDataBuf = temp1;
				sampleData[1] = sampleDataBuf;
				sampleDataBuf = 0;
				sampleDataBuf = temp2;
				sampleData[1] |= (sampleDataBuf << 16);
				lcdwrite("Y wrote: ",1,0);
// 				itoa(temp1,buf0,10);						
// 				countlen = strlen(buf0);
// 				lcdwrite(buf0,1,9);
// 				itoa(temp2,buf1,10);
// 				lcdwrite(buf1,1,countlen+9);
				sprintf(buf0,"%lu",sampleData[1]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,9);
				_delay_ms(5000);
				// ------------------------------------------
				// Take sample Z
				firstint = restint = temp1 = temp2 = 0;
				lcdwrite("                ",1,0);
				lcdwrite("Z",1,0);
				setCursor(1,1);
				lcd_data(0b00111010); // ':' char
				sampleCheck = 0;
				

				_delay_ms(1000);
				while(sampleCheck == 0)
				{
					if(resetRequest)
					{
						selectMode = -1;

						sampleCheck = 1;
						break;
					}

					tempFreq = stableFreq;

					sprintf(
					buf0,
					"%lu",
					tempFreq
					);

					lcdwrite(
					"             ",
					1,
					3
					);

					lcdwrite(
					buf0,
					1,
					3
					);

					_delay_ms(100);
				}
				sampleCheck = 0;
				//
				sampleData[2] = 0;
				

				tempFreq = stableFreq;

				firstint = (unsigned int)(tempFreq & 0xFFFF);

				restint = (unsigned int)(tempFreq >> 16);
				write_long_EEPROM(8,&firstint,&restint);		// write 3rd calib long number
				itoa(firstint,buf0,10);
				itoa(restint,buf1,10);
				countlen = strlen(buf0);
// 				firstint = freq/10000;
// 				restint = freq - firstint*10000;
// 				write_long_EEPROM(8,&firstint,&restint);		// write 3rd calib long number
// 				itoa(firstint,buf0,10);					                                          		
// 				itoa(restint,buf1,10);
// 				countlen = strlen(buf0);
				//
				read_long_EEPROM(8,&temp1,&temp2);				// read 3rd calib long number
				/*sampleData[2] = temp1*10000+temp2;*/
				sampleDataBuf = temp1;
				sampleData[2] = sampleDataBuf;
				sampleDataBuf = 0;
				sampleDataBuf = temp2;
				sampleData[2] |= (sampleDataBuf << 16);
				lcdwrite("Z wrote: ",1,0);
// 				itoa(temp1,buf0,10);							
// 				countlen = strlen(buf0);
// 				lcdwrite(buf0,1,9);
// 				itoa(temp2,buf1,10);
// 				lcdwrite(buf1,1,countlen+9);
				sprintf(buf0,"%lu",sampleData[2]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,9);
				_delay_ms(5000);
				// --------------------------------------------------
				// take sample M
				firstint = restint = temp1 = temp2 = 0;
				lcdwrite("                ",1,0);
				lcdwrite("M",1,0);
				setCursor(1,1);
				lcd_data(0b00111010); // ':' char
				sampleCheck = 0;
				

				_delay_ms(1000);
				while(sampleCheck == 0)
				{
					if(resetRequest)
					{
						selectMode = -1;

						sampleCheck = 1;
						break;
					}

					tempFreq = stableFreq;

					sprintf(
					buf0,
					"%lu",
					tempFreq
					);

					lcdwrite(
					"             ",
					1,
					3
					);

					lcdwrite(
					buf0,
					1,
					3
					);

					_delay_ms(100);
				}
				sampleCheck = 0;
				//
				sampleData[3] = 0;
				

				tempFreq = stableFreq;

				firstint = (unsigned int)(tempFreq & 0xFFFF);

				restint = (unsigned int)(tempFreq >> 16);
				write_long_EEPROM(12,&firstint,&restint);		// write 4th calib long number
				itoa(firstint,buf0,10);
				itoa(restint,buf1,10);
				countlen = strlen(buf0);
				read_long_EEPROM(12,&temp1,&temp2);				// read 4th calib long number
				sampleDataBuf = temp1;
				sampleData[3] = sampleDataBuf;
				sampleDataBuf = 0;
				sampleDataBuf = temp2;
				sampleData[3] |= (sampleDataBuf << 16);
				lcdwrite("M wrote: ",1,0);
				sprintf(buf0,"%lu",sampleData[3]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,9);
				_delay_ms(5000);
				//------------------------------------------------------------
				// take sample N
				firstint = restint = temp1 = temp2 = 0;
				lcdwrite("                ",1,0);
				lcdwrite("N",1,0);
				setCursor(1,1);
				lcd_data(0b00111010); // ':' char
				sampleCheck = 0;
				

_delay_ms(1000);
				while(sampleCheck == 0)
				{
					if(resetRequest)
					{
						selectMode = -1;

						sampleCheck = 1;
						break;
					}

				tempFreq = stableFreq;
					sprintf(
					buf0,
					"%lu",
					tempFreq
					);

					lcdwrite(
					"             ",
					1,
					3
					);

					lcdwrite(
					buf0,
					1,
					3
					);

					_delay_ms(100);
				}
				sampleCheck = 0;
				//
				sampleData[4] = 0;
				

				tempFreq = stableFreq;

				firstint = (unsigned int)(tempFreq & 0xFFFF);

				restint = (unsigned int)(tempFreq >> 16);
				write_long_EEPROM(16,&firstint,&restint);		// write 5th calib long number
				itoa(firstint,buf0,10);
				itoa(restint,buf1,10);
				countlen = strlen(buf0);
				read_long_EEPROM(16,&temp1,&temp2);				// read 5th calib long number
				sampleDataBuf = temp1;
				sampleData[4] = sampleDataBuf;
				sampleDataBuf = 0;
				sampleDataBuf = temp2;
				sampleData[4] |= (sampleDataBuf << 16);
				lcdwrite("N wrote: ",1,0);
				sprintf(buf0,"%lu",sampleData[4]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,9);
				_delay_ms(1500);
				// test value
				_delay_ms(5000);
				clearScreen();
// 				firstint = restint = 0;
// 				firstint = (sampleData[0] / 10000);
// 				restint = (sampleData[0] - firstint*10000);				
// 				itoa(firstint,buf0,10);					                                          		// convert int to char
// 				itoa(restint,buf1,10);
// 				countlen = strlen(buf0);
// 				lcdwrite(buf0,0,0);
// 				lcdwrite(buf1,0,countlen);
				sprintf(buf0,"%lu",sampleData[0]);
				countlen = strlen(buf0);
				lcdwrite(buf0,0,0);
				// -----
// 				firstint = restint = 0;
// 				firstint = (sampleData[1] / 10000);
// 				restint = (sampleData[1] - firstint*10000);
// 				itoa(firstint,buf0,10);					                                          		// convert int to char
// 				itoa(restint,buf1,10);
// 				countlen = strlen(buf0);
// 				lcdwrite(buf0,0,8);
// 				lcdwrite(buf1,0,countlen+8);
				sprintf(buf0,"%lu",sampleData[1]);
				countlen = strlen(buf0);
				lcdwrite(buf0,0,8);
				// -----
// 				firstint = restint = 0;
// 				firstint = (sampleData[2] / 10000);
// 				restint = (sampleData[2] - firstint*10000);
// 				itoa(firstint,buf0,10);					                                          		// convert int to char
// 				itoa(restint,buf1,10);
// 				countlen = strlen(buf0);
// 				lcdwrite(buf0,1,0);
// 				lcdwrite(buf1,1,countlen);
				sprintf(buf0,"%lu",sampleData[2]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,0);
				// ----------
				sprintf(buf0,"%lu",sampleData[3]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,8);
				// ----------
				sprintf(buf0,"%lu",sampleData[4]);
				countlen = strlen(buf0);
				lcdwrite(buf0,1,0);
				// Back to menu
				
				
				sampleA[0] = 0;

				if(sampleData[0] == 0)
				{
					sampleData[0] = 1;
				}

				for(int i = 1; i < 5; i++)
				{
					if(sampleData[i] == 0)
					{
						sampleData[i] = 1;
					}

					sampleA[i] = log10(
					(float)sampleData[0]
					/
					(float)sampleData[i]
					);
				}

				linear_regression(
				sampleA,
				sampleY,
				&b1,
				&b0
				);

				EEPROM_write_float(
				100,
				b0
				);

				EEPROM_write_float(
				104,
				b1
				);
				clearScreen();

				sprintf(
				buf0,
				"b0=%.5f",
				b0
				);

				lcdwrite(
				buf0,
				0,
				0
				);

				sprintf(
				buf0,
				"b1=%.5f",
				b1
				);

				lcdwrite(
				buf0,
				1,
				0
				);

				_delay_ms(5000);
				selectMode = -1;
				
				break;
			}
		}
// -------------------------------------------------------------------------
		if(selectMode==1)
		{
			clearScreen();

			while(selectMode==1)
			{

				if(measureStep==0)
				{

					
						clearScreen();

						lcdwrite(
						"Insert Blank",
						0,
						0
						);

						measurePressed=0;

						_delay_ms(300);

						while(measurePressed==0)
						{
							sprintf(
							buf0,
							"F0:%lu",
							freq
							);

							lcdwrite(
							"                ",
							1,
							0
							);

							lcdwrite(
							buf0,
							1,
							0
							);

							_delay_ms(100);
						}

						F0=(float)freq;

						

						measurePressed=0;

						clearScreen();

						lcdwrite(
						"saved blank",
						0,
						0
						);

						sprintf(
						buf0,
						"F0:%lu",
						(unsigned long)F0
						);

						lcdwrite(
						buf0,
						1,
						0
						);

						_delay_ms(5000);   // hien thi Saved Blank + F0 trong 5 giay

						
					

					clearScreen();

					lcdwrite(
					"insert sample",
					0,
					0
					);

					measurePressed=0;

					while(measurePressed==0)
					{
						sprintf(
						buf0,
						"F:%lu",
						freq
						);

						lcdwrite(
						"                ",
						1,
						0
						);

						lcdwrite(
						buf0,
						1,
						0
						);

						_delay_ms(100);
					}
					measurePressed = 0;

					/*
					 * Luu tan so mau
					 */
					
					F_sample = (float)stableFreq;

					/*
					 * Hien thi Saved Sample
					 */
					clearScreen();

					lcdwrite(
						"saved sample",
						0,
						0
					);

					sprintf(
						buf0,
						"F:%lu",
						(unsigned long)F_sample
					);

					lcdwrite(
						buf0,
						1,
						0
					);

					_delay_ms(5000);   // hien thi 5 giay

					/*
					 * Sau khi hien thi xong moi tinh SVR
					 */
					clearScreen();

					lcdwrite(
						"please wait !!",
						0,
						0
					);

					_delay_ms(3000);

					measureStep = 2;

					
				}

				

				else if(measureStep == 2)
				{
					float A;
					float C_linear;
					float C_final;

					/*
					 * ============================================================
					 * KIEM TRA GIA TRI DO
					 * ============================================================
					 */
					if(F0 <= 0.0f || freq <= 0)
					{
						clearScreen();

						lcdwrite(
							"ERROR F0/F",
							0,
							0
						);

						_delay_ms(3000);

						selectMode = -1;
						measureStep = 0;

						break;
					}

					/*
					 * ============================================================
					 * 1. TINH ABSORBANCE
					 *
					 * A = log10(F0 / F)
					 * ============================================================
					 */

					A = log10f(
						F0 /
						(float)freq
					);

					/*
					 * ============================================================
					 * 2. LINEAR REGRESSION CUA THIET BI
					 *
					 * b0, b1:
					 *     doc tu EEPROM sau khi calibration
					 *
					 * C_LR = (A - b0) / b1
					 * ============================================================
					 */

					if(b1 == 0.0f)
					{
						b0 = 0.09388f;
						b1 = 0.00625f;
					}

					C_linear =
						(A - b0)
						/ b1;

					/*
					 * ============================================================
					 * 3. SVR-RBF
					 *
					 * Ham calculate_H2O2_SVR() nam trong:
					 *
					 *     SVR_RBF_model.h
					 *
					 * Model Python duoc train voi:
					 *
					 *     B0 = 0.09388
					 *     B1 = 0.00625
					 *
					 * Nen khong dung b0/b1 EEPROM trong ham SVR.
					 * ============================================================
					 */

					A = log10f(F0 / F_sample);
					C_final =
						calculate_H2O2_SVR(
							F0,
							F_sample
						);

					/*
					 * Khong cho ket qua am
					 */
					if(C_final < 0.0f)
					{
						C_final = 0.0f;
					}

					/*
					 * ============================================================
					 * 4. HIEN THI LINEAR
					 * ============================================================
					 */

					clearScreen();

					firstint = (int)C_linear;

					restint =
						abs(
							(int)(
								(C_linear - firstint)
								* 100.0f
							)
						);

					sprintf(
						buf0,
						"LR:%d.%02d",
						firstint,
						restint
					);

					lcdwrite(
						buf0,
						0,
						0
					);

					/*
					 * 5. HIEN THI SVR-RBF
					 */

					firstint = (int)C_final;

					restint =
						abs(
							(int)(
								(C_final - firstint)
								* 100.0f
							)
						);

					sprintf(
						buf0,
						"SVR:%d.%02d",
						firstint,
						restint
					);

					lcdwrite(
						buf0,
						1,
						0
					);

					/*
					 * Cho nguoi dung xem ket qua
					 */
					_delay_ms(7000);

					/*
					 * Quay lai menu
					 */
					selectMode = -1;
					measureStep = 0;

					break;
				}

			}
		}
	}
}


