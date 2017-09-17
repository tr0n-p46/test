#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 100

int getLine(FILE*,char[]);
void copy(char[],char[]);

int main() {
	FILE *fp=fopen("hello.txt","r");
	int len,max=0;
	char line[MAXLINE],longest[MAXLINE];
	while((len=getLine(fp,line))>0) {
		if(len>max) {
			max=len;
			copy(line,longest);		
		}
	}
	if(max>0)
		printf("%s (%d)\n",longest,max);	
	fclose(fp);
}

int getLine(FILE *fp,char line[]) {
	int c;
	int count=0;
	while((c=fgetc(fp))!=EOF && c!='\n') {
		line[count]=c;
		++count;	
	}
	return count;
}

void copy(char line[],char longest[]) {
	int i=0;
	while(line[i]!='\0') longest[i]=line[i++];
}
