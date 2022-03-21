#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "memtools.h"

/** 
* Memory loader that temporarily removes Windows 10 (32-bit) 'Activate Windows' watermark by
* modifying explorer.exe process.
*
* Tested on 32-bit versions of Windows 10 20H2 and Windows 10 21H1
*
***/

// Prototypes

void cleanup(HANDLE hThread, HANDLE hProcess);
void banner(void);


/* Banner generated at manytools.org */

void banner(void){
	
	
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	COORD coordScreen = { 0, 0 };
	DWORD bytesWritten = 0;
	DWORD size = 0;

    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	
	size = consoleInfo.dwSize.X * consoleInfo.dwSize.Y;

    SetConsoleTextAttribute(hConsole, 7);

	
	puts("");
	puts("                            dP                         dP                   ");
	puts("                            88                         88                   ");
	puts(" .d8888b. dP.  .dP 88d888b. 88 .d8888b. .d8888b. .d888b88 .d8888b. 88d888b. ");
	puts(" 88ooood8  `8bd8'  88'  `88 88 88'  `88 88'  `88 88'  `88 88ooood8 88'  `88 ");
	puts(" 88.  ...  .d88b.  88.  .88 88 88.  .88 88.  .88 88.  .88 88.  ... 88       ");
	puts(" `88888P' dP'  `dP 88Y888P' dP `88888P' `88888P8 `88888P8 `88888P' dP       ");
	puts("                   88                                                       ");
	puts("                   dP\tCoded by: Aleksandar (github.com/revengsmK)          ");
	puts("\n");
	
	FillConsoleOutputAttribute(hConsole,9,size,coordScreen,&bytesWritten);

}


void cleanup(HANDLE hThread, HANDLE hProcess){
	
	ResumeThread(hThread);
	
	CloseHandle(hThread);
	CloseHandle(hProcess);
	
	puts("Press any key to exit....");
	getch();
	
}


int main(void){
	
	DWORD baseAddr = 0;
	DWORD endaddr = 0;
	DWORD patAddr = 0;
	DWORD patAddr2 = 0;
	BOOL rVal;
	BOOL rVal2;


	STARTUPINFO startInfo;
	PROCESS_INFORMATION procInfo;
	
	IMAGE_DOS_HEADER DosHead;
	IMAGE_NT_HEADERS hPE;
	
	unsigned char patch_bytes[] = { 0x90, 0xE9 }; // nop, jmp [address]
	unsigned char patch_bytes2[] = { 0x6A, 0x00, 0x90, 0x90, 0x90 }; // push 0 , nop, nop , nop
	char explorerpath[MAX_PATH];

	banner();
	
	// Kill all existing explorer processes
	
	puts("-> Killing all existing explorer.exe processes...\n");
	
	KillProcessByName("explorer.exe");
	
	GetStartupInfo(&startInfo);
	
	// Get explorer.exe full path
	
	GetWindowsDirectory(explorerpath,MAX_PATH);
	
	strncat(explorerpath,"\\explorer.exe",sizeof(explorerpath) - strlen(explorerpath) - 1);
	
	// Create new suspended explorer process
	
	if(!CreateProcess(explorerpath,NULL,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,NULL,&startInfo,&procInfo))
	{
		puts("[!] Failed to create process!\nAborting...\n");
		getch();
		return -1;
	}
	
	
	// Get image base address
	
	puts("-> Getting image base address...\n");
	

	baseAddr = GetBaseAddress(procInfo.hProcess ,procInfo.hThread);
	
	
	if(baseAddr == 0)
	{
		puts("   [!] Failed to get image base address! Aborting...\n");
		cleanup(procInfo.hThread,procInfo.hProcess);
		return -1;
	}
	
	 printf("-> Base address: %X\t[ OK ]\n\n", (unsigned int)baseAddr);
	
	// Read DOS and PE header 
	
	rVal = ReadProcessMemory(procInfo.hProcess,(char*)baseAddr,&DosHead,sizeof(IMAGE_DOS_HEADER),NULL);
	rVal2 = ReadProcessMemory(procInfo.hProcess,(char*)baseAddr + DosHead.e_lfanew,&hPE,sizeof(IMAGE_NT_HEADERS),NULL);
	
	
	if(rVal == 0 || rVal2 == 0)
	{
		
		puts("[!] Failed to get code section size. Aborting...\n");
		cleanup(procInfo.hThread,procInfo.hProcess);
		return -1;
	}
		
	// Get code section size
	printf("-> Code section size: %X \n",(unsigned int)hPE.OptionalHeader.SizeOfCode);
	
	endaddr = baseAddr + hPE.OptionalHeader.SizeOfCode;
	
	// Search for hex pattern and return address
	
	patAddr = seekbytes(procInfo.hProcess,baseAddr,endaddr,"\x85\xC9\x0F\x84\x00\x00\x00\x00\x68\x04\x01\x00\x00","xxxx????xxxxx"); // test ecx,ecx
	
	patAddr2 = seekbytes(procInfo.hProcess,baseAddr,endaddr,"\x68\x28\x00\x08\x08","xxxxx"); // push 0x8080028
	
	if(patAddr == 0 || patAddr2 == 0)
	{
		
		puts("[!] Cannot find pattern. Aborting...\n");
		cleanup(procInfo.hThread,procInfo.hProcess);
		return -1;
		
	}
	
	puts("\n-> Writing to process address space...\n");
	
	// Patch process address space...
	
	
	if(!WriteProcessMemory(procInfo.hProcess,(char*)patAddr+2,patch_bytes,2,NULL))
	{
		puts("   [!] Failed to write to process memory address!\n");
		cleanup(procInfo.hThread,procInfo.hProcess);
		return -1;
	}
	else
		printf("   [*] Writing to address 0x%X\t[ OK ]\n",(unsigned int)patAddr);
	
	if(!WriteProcessMemory(procInfo.hProcess,(char*)patAddr2,patch_bytes2,5,NULL))
	{
		puts("   [!] Failed to write to process memory address!\n");
		cleanup(procInfo.hThread,procInfo.hProcess);
		return -1;
	}
	else
		printf("   [*] Writing to address 0x%X\t[ OK ]\n\n",(unsigned int)patAddr2);
	
	
	puts("Ok, memory loader completed operation!\n");
	
	cleanup(procInfo.hThread,procInfo.hProcess);

	
	return 0;
}