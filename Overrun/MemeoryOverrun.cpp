#include "tchar.h"
#include "stdio.h"
#include "windows.h"

void ResetArray(float* pfArray, int iSize) {
	for(int i = 0; i < iSize; i++) {// Memory overrun
		pfArray[i] = 0;
		iSize--;
	}
}

int _tmain(int argc, _TCHAR* argv[]) {
	const int ARRAY_SIZE= 10;
	char*   pArray	= (char*)malloc(ARRAY_SIZE);
	char*   pMsg    = (char*)malloc(MAX_PATH);
	char*   pPtr    = pMsg;

	ResetArray((float*)pArray, ARRAY_SIZE);
	// Do some operation on 'pfArray'
	// ...
	for(int i = 0; i < ARRAY_SIZE; i++)
		pPtr += sprintf_s(pPtr, MAX_PATH - (pPtr - pMsg), "%.3i ", pArray[i]);
	printf_s(pMsg, MAX_PATH);
	free(pMsg);// CRT structures corrupted, unpredictable behavior is expected
	free(pArray);
	printf("Execution ended.\r\n");
	return 0;
}
