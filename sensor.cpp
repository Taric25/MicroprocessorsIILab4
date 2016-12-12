/*--------------- sensor.cpp ---------------
by:	Mr. Ryan D Aldrich
	Mr. Taric S Alani
	Mr. John A Nicholson
EECE.5520-201 Microprocessors II
ECE Dept.
UMASS Lowell
Based on specification by Dr. Yan Luo:
lab4mtopencv.pdf 2016-11-25 11:48:28

PURPOSE
This defines the function(s) prototyped in 
sensor.h    
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "sensor.h"

using namespace std;


float temp_math(unsigned short temp_val)
{
	
	temp_val = temp_val << 4;
	temp_val = ((temp_val*0.0625) - 5.0);
	temp_val = (((temp_val*9.0)/5.0)+32.0);
	return temp_val;
}
