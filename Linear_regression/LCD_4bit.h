#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

void lcd_data(unsigned char data);					// send char data to LCD
void lcdcmdwrite(unsigned char cmd);				// send command to LCD
void lcd_initialize();								// initialize LCD
void setCursor(int a, int b);						// set Cursor
void lcdwrite(const char *data_l, int a, int b);	// send String to LCD
void CreateChar(unsigned char *Pattern);			// send custom char to LCD
void clearScreen();									// clear Screen
// ....................................... //

/*
	  MEGA 			 ATmega2560			       LCD
	D 6 7 8 9		PORTH 3 4 5 6			D 7 6 5 4
	D22				PORTA 0					RS
	D23				PORTA 1					RW
	D24				PORTA 2					EN
*/

// ....................................... //
void lcd_data(unsigned char data)
{
	// send MSB
	PORTH = (PORTH & 0x87) | ((data & 0xF0)>>1);
	PORTA |= (1 << PINA0);						// PA0 RS high
	PORTA &= ~(1 << PINA1);						// PA1 RW low
	_delay_ms(1);
	PORTA |= (1 << PINA2);						// PA2 EN high
	PORTA &= ~(1 << PINA2);						// PA2 EN low
	_delay_ms(1);
	
	// send LSB
	PORTH = (PORTH & 0x87) | ((data & 0x0F)<<3);
	_delay_ms(1);
	PORTA |= (1 << PINA2);						// PA2 EN high
	PORTA &= ~(1 << PINA2);						// PA2 EN low
	_delay_ms(1);
}

void lcdcmdwrite(unsigned char cmd)
{
	// send MSB
	PORTH = (cmd >> 1);
	PORTA &= ~((1 << PINA0) | (1 << PINA1));	// PA1 RW  + PA0 RS low
	_delay_ms(1);
	PORTA |= (1 << PINA2);						// PA2 EN high
	PORTA &= ~(1 << PINA2);						// PA2 EN low
	_delay_ms(1);
	
	// send LSB
	PORTH = (cmd << 3);
	_delay_ms(1);
	PORTA |= (1 << PINA2);						// PA2 EN high
	PORTA &= ~(1 << PINA2);						// PA2 EN low
	_delay_ms(1);
}

void lcd_initialize() {
	lcdcmdwrite(0xFF);
	_delay_ms(1);  //
	lcdcmdwrite(0x83);
	_delay_ms(1);  //
	lcdcmdwrite(0x83);
	_delay_ms(1);  //
	lcdcmdwrite(0x83);
	_delay_ms(1);  //
	lcdcmdwrite(0x82);
	_delay_ms(1);;  // Reset
	lcdcmdwrite(0x28);      // Function Set: 4-bit, 2 Line, 5x8 Dots
	_delay_ms(10);
	lcdcmdwrite(0x01);
	_delay_ms(200);
	lcdcmdwrite(0x0C);  // Display on Cursor off
	_delay_ms(10);
	lcdcmdwrite(0x06);  // Entry Mode
	_delay_ms(10);
}

void setCursor(int a, int b)  // command 0x80|x for row 0      // command 0x80|0x40+x for row 1
{
	switch (a) {
		case 0:
		b |= 0x80;
		break;
		case 1:
		b |= 0xC0;
		break;
	}
	lcdcmdwrite(b);
}

void lcdwrite(
const char *data_l,
int a,
int b
)
{
	char data;

	int len=
	strlen(
	data_l
	);

	setCursor(
	a,
	b
	);

	for(
	int i=0;
	i<len;
	i++
	)
	{
		data=
		data_l[i];

		lcd_data(
		data
		);

		b++;

		if(
		b>=16
		)
		{
			break;
		}
	}
}

void CreateChar(unsigned char *Pattern) {
	int i = 0;
	for (i = 0; i < 8; i++) {
		lcd_data(Pattern[i]);
		_delay_ms(1);
	}
}

void clearScreen()
{
	lcdcmdwrite(0x01);
	_delay_ms(5);
}