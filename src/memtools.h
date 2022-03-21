#ifndef MEMTOOLS_H
#define MEMTOOLS_H

#include <stdint.h>

DWORD GetBaseAddress(HANDLE hProc , HANDLE hThread);
void KillProcessByName(const char *filename);
uintptr_t seekbytes (HANDLE hProc, uintptr_t  base_addr , uintptr_t  end_addr, char *pattern , char *mask);

#endif