#include "Types.h"
#define A1 (*((volatile unsigned char *)0x1FE000))
#define A2 (*((volatile unsigned char *)0x1FF000))

void kPrintString(int iX, int iY, const char* pcString);
void checkReadWrite();
void Main(void)
{
	kPrintString(0,12,"Switch To IA-32e Mode Success~!!");
	kPrintString(0,13,"IA-32e C Language Kernel Start...............[Pass]");
	kPrintString(0,14,"This message is printed through the video memory relocated to 0xAB8000");
	checkReadWrite();
}

void kPrintString(int iX, int iY, const char* pcString)
{
	CHARACTER* pstScreen = (CHARACTER*) 0xAB8000;
	int i;

	pstScreen += (iY * 80) + iX;

	for(int i = 0; pcString[i] != 0; i++)
		pstScreen[i].bCharactor = pcString[i];
}

void checkReadWrite()
{
	char * t = A1;
	kPrintString(0,15,"Read from 0x1FE000.....[Ok]");
	A1 = 0;
	kPrintString(0,16,"Write to 0x1FE000.....[Ok]");
	*t = A2;
	kPrintString(0,17,"Read from 0x1FF000.....[Ok]");
	//If uncomment this, occur error!
	//A2 = 0;
	//kPrintString(0,18,"Write to 0x1FF000.....[Ok]");
}
