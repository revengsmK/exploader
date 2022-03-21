#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

#include "memtools.h"

/* Returns image base address of suspended process */

DWORD GetBaseAddress(HANDLE hProc ,HANDLE hThread){
	
	DWORD baseAddr = 0;
	CONTEXT ctx;
	
	ctx.ContextFlags = CONTEXT_INTEGER;
	
	if(!GetThreadContext(hThread,&ctx)){
		puts("Failed to get thread context!");
		return 0;
	}
	
	
	if(!ReadProcessMemory(hProc,(void*)(ctx.Ebx + 8),&baseAddr,sizeof(void*),NULL)){
		puts("Failed to get base address!\nFailed to read process memory!");
		return 0;
	}
	
	
	return baseAddr;
}


void KillProcessByName(const char *filename){
	
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    PROCESSENTRY32 pEntry;
	
    pEntry.dwSize = sizeof(PROCESSENTRY32);
	
    BOOL hRes = Process32First(hSnapShot, &pEntry);
	
    while (hRes)
    {
        if (strcmp(pEntry.szExeFile, filename) == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                                          (DWORD) pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 1);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
	
    CloseHandle(hSnapShot);
}


/*Searches for a specified masked byte pattern in a process address range
   This is a modified (to search process address range) pattern 
   scanning function from guidedhacking.com
 */

uintptr_t seekbytes(HANDLE hProc, uintptr_t  base_addr , uintptr_t  end_addr, char *pattern , char *mask){

	SIZE_T bytesRead = 0;
	char buffer[4096];
	uintptr_t  currentChunk = base_addr;
	size_t patternLength;
	uintptr_t i,j;
	
	while(currentChunk <= end_addr)
	{
		ReadProcessMemory(hProc,(void*)currentChunk,&buffer,4096,&bytesRead);
		
		// check ReadProcessMemory() call
		
		if(bytesRead == 0)
			return 0;
		
	 patternLength = strlen(mask);
	 
	 for ( i = 0; i < bytesRead - patternLength; i++)
    {
        BOOL found = TRUE;
        for (j = 0; j < patternLength; j++)
        {
            if (mask[j] != '?' && pattern[j] != *(char*)(buffer + i + j))
            {
                found = FALSE;
                break; 
            }
        }

        if (found)
			return currentChunk + i;
    }
		currentChunk = currentChunk + bytesRead;
	}
	
	return 0;
}
