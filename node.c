#include <stdio.h>

void main() {
	int blank=0,tab=0,newline=0;
	int c;
	while((c=getchar())!=EOF) {
		switch(c) {
			case ' ': ++blank;
				  break;
			case '\n': ++newline;
				  break;
			case '\t': ++tab;
				  break;		
		}	
		if(c=='q') break;
	}
	printf("Blank : %d\nTab : %d\nNew Line : %d\n",blank,tab,newline);
}
