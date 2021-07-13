#ifndef __MY_LOCATION_H
#define __MY_LOCATION_H
	static int AX,BX,CX,DX,AY,BY,CY,DY;
	void MY_location_Init(int X1, int Y1, int X2, int Y2, int X3, int Y3,int X4,int Y4);
	double* trilateration(double x1, double y1, double d1, double x2, double y2, double d2, double x3, double y3, double d3);
	double* Printf(double d1, double d2, double d3, double d4);
	int P(int x, int n);

#endif