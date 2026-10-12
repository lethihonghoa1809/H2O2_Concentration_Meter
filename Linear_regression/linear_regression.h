void linear_regression(float Y[], float X[], float* b1, float* b0);

void linear_regression(float sampleY[], float sampleX[], float* b1, float* b0) // Y = b1X + b0
{
	float meanX = 0.0, meanY = 0.0, Sxx = 0.0, Sxy = 0.0;
	char buf0[20]; 
	
	meanX = ((float)(sampleX[0] + sampleX[1] + sampleX[2] + sampleX[3] + sampleX[4]))/5.0;
	meanY = (sampleY[0] + sampleY[1] + sampleY[2] + sampleY[3] + sampleY[4])/5.0;
	clearScreen();
	sprintf(buf0,"%f",meanX);
	lcdwrite(buf0,0,0);
	sprintf(buf0,"%f",meanY);
	lcdwrite(buf0,1,0);
	_delay_ms(8000);
	for (int i = 0;i < 5;i++)
	{
		Sxx += pow((sampleX[i] - meanX),2);
	}
	
	for (int i = 0;i < 5;i++)
	{
		Sxy += (sampleY[i] - meanY)*(sampleX[i] - meanX);
	}
	clearScreen();
	sprintf(buf0,"%f",Sxx);
	lcdwrite(buf0,0,0);
	sprintf(buf0,"%f",Sxy);
	lcdwrite(buf0,1,0);
	_delay_ms(8000);
	*b1 = Sxy/Sxx;
	*b0 = meanY - (*b1)*meanX;
	//
	//clearScreen();
//	sprintf(buf0,"%f",*b1);
//	lcdwrite(buf0,0,0);
	//sprintf(buf0,"%f",*b0);
	//lcdwrite(buf0,1,0);
//	_delay_ms(8000);
}