//#include"myLocation.h"
	static int AX,BX,CX,DX,AY,BY,CY,DY;

/*
??:?x?y??
*/
int  P(int x, int n)
{
	int  val = 1;
	while (n--)
		val *= x;
	return val;

}


void MY_location_Init(int X1, int Y1, int X2, int Y2, int X3, int Y3,int X4,int Y4)
{AX=X1; AY=Y1; BX=X2;BY=Y2; CX=X3; CY=Y3 ;DX=X4,DY=Y4;}
/*
??:??????????
???????
*/
double* trilateration(double x1, double y1, double d1, double x2, double y2, double d2, double x3, double y3, double d3)
{
	
	static double d[4] = { 0.0,0.0 };
	double a11 = 2 * (x1 - x3);
	double a12 = 2 * (y1 - y3);
	double b1 = P(x1, 2) - P(x3, 2) + P(y1, 2) - P(y3, 2) + P(d3, 2) - P(d1, 2);
	double a21 = 2 * (x2 - x3);
	double a22 = 2 * (y2 - y3);
	double b2 = P(x2, 2) - P(x3, 2) + P(y2, 2) - P(y3, 2) + P(d3, 2) - P(d2, 2);
	d[0] = (b1 * a22 - a12 * b2) / (a11 * a22 - a12 * a21);
	d[1] = (a11 * b2 - b1 * a21) / (a11 * a22 - a12 * a21);
	return d;
}


double  *Printf(double d1, double d2, double d3, double d4)
{
	double *Coordinate_A = 0, *Coordinate_B = 0, *Coordinate_C = 0, *Coordinate_D = 0;
	static double XY[2] = { 0,0 };

	Coordinate_A = trilateration(AX, AY, d1, BX, BY, d2, CX, CY, d3);

	Coordinate_B = trilateration(AX, AY, d1, BX, BY, d2, DX, DY, d4);

	Coordinate_C = trilateration(AX, AY, d1, CX, CY, d3, DX, DY, d4);

	Coordinate_D = trilateration(BX, BY, d2, CX, CY, d3,DX, DY, d4);


	XY[0] = (Coordinate_A[0] + Coordinate_B[0] + Coordinate_C[0] + Coordinate_D[0]) / 4;

	XY[1] = (Coordinate_A[1] + Coordinate_B[1] + Coordinate_C[1] + Coordinate_D[1]) / 4;
	return XY;
}