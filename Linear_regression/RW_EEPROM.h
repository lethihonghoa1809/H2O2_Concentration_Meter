
void EEPROM_write(unsigned int uiAddress, unsigned char ucData);
void EEPROM_read(unsigned int uiAddress, unsigned int *data);
void read_long_EEPROM(int index, unsigned int* temp1, unsigned int* temp2);
void write_long_EEPROM(int index, unsigned int* firstint, unsigned int* restint);

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
	;
	/* Set up address and Data Registers */
	EEAR = uiAddress;
	EEDR = ucData;// chua byte se ghi xuong EEPROM
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEPE);
}

void EEPROM_read(unsigned int uiAddress, unsigned int *data)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
	;
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from Data Register */
	*data = EEDR;
}

void read_long_EEPROM(int index, unsigned int* temp1, unsigned int* temp2)
{
	unsigned int buf = 0;
	*temp1 = *temp2 = 0;
	EEPROM_read(index,&buf);
	*temp1 |= buf;
	EEPROM_read(index + 1,&buf);
	*temp1 |= buf << 8;
	EEPROM_read(index + 2,&buf);
	*temp2 |= buf;
	EEPROM_read(index + 3,&buf);
	*temp2 |= buf << 8;
}

void write_long_EEPROM(int index, unsigned int* firstint, unsigned int* restint)
{
	EEPROM_write(index,*firstint);//Byte0 = lower 8 bit firstint
	EEPROM_write(index + 1,(*firstint >> 8));//upper 8 bit firstint
	EEPROM_write(index + 2,(*restint));//lower 8 bit restint
	EEPROM_write(index + 3,(*restint >> 8));
}

void EEPROM_write_float(int addr,float value)
{
	unsigned char *p;

	p=(unsigned char*)&value;

	for(int i=0;i<4;i++)
	{
		EEPROM_write(addr+i,p[i]);
	}
}

float EEPROM_read_float(int addr)
{
	float value;

	unsigned char *p;

	p=(unsigned char*)&value;

	for(int i=0;i<4;i++)
	{
		unsigned int t;

		EEPROM_read(addr+i,&t);

		p[i]=(unsigned char)t;
	}

	return value;
}