#include "Types.h"

void kPrintString(int iX, int iY, const char* pcString);

void Main(void)
{
	kPrintString(0,12,"Switch To IA-32e Mode Success~!!");
	kPrintString(0,13,"IA-32e C Language Kernel Start...............[Pass]");
	kPrintString(0,14,"This message is printed through the video memory relocated to 0xAB8000");
}

void kPrintString(int iX, int iY, const char* pcString)
{
	CHARACTER* pstScreen = (CHARACTER*) 0xAB8000;
	int i;

	pstScreen += (iY * 80) + iX;

	for(int i = 0; pcString[i] != 0; i++)
		pstScreen[i].bCharactor = pcString[i];
}
