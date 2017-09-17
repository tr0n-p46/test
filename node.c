#include <stdio.h>

#define STEP   20
#define LOW     0
#define UPPER 300

#define F2C(f) ((5/9.0)*((f)-32))

void main() {
	int f;
	for(f=LOW;f<=UPPER;f+=STEP) 
		printf("%3d %6.1f\n",f,F2C(f));	
}
