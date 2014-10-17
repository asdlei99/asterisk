#include "inject.h"

typedef LRESULT (__stdcall* PSENDMESSAGE)(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

typedef struct{
	HWND hWnd;
	PSENDMESSAGE fnSendMessage;
	char psw[128];
}INJECT_DATA,*PINJECT_DATA;



#define THREAD_SIZE 1024

DWORD __stdcall RemoteThreadProc(void* pv)
{
	PINJECT_DATA pData = (PINJECT_DATA)pv;
	int len = 0;
	len = pData->fnSendMessage(pData->hWnd,WM_GETTEXT,sizeof(pData->psw),(LPARAM)pData->psw);
	pData->psw[len] = '\0';
	return 0;
}


int GetWindowTextRemote(HANDLE hProcess,HWND hWnd,char* buf,int c)
{
	HINSTANCE hInstUser32 = NULL;
	INJECT_DATA* pRemoteData = NULL;
	INJECT_DATA localData = {0};
	DWORD* pRemoteCode = NULL;
	HANDLE hThread = NULL;
	DWORD dwThreadId = 0;
	DWORD dwNumberOfBytes;

	__try{
		
		hInstUser32 = GetModuleHandle("user32");
		if(hInstUser32 == NULL)
			__leave;

		localData.hWnd = hWnd;
		localData.fnSendMessage = (PSENDMESSAGE)GetProcAddress(hInstUser32,"SendMessageA");
		if(localData.fnSendMessage == NULL)
			__leave;

		pRemoteData = (PINJECT_DATA)VirtualAllocEx(hProcess,0,sizeof(INJECT_DATA),MEM_COMMIT,PAGE_READWRITE);
		if(pRemoteData == NULL)
			__leave;
		WriteProcessMemory(hProcess,pRemoteData,&localData,sizeof(INJECT_DATA),&dwNumberOfBytes);

		pRemoteCode = (DWORD*)VirtualAllocEx(hProcess,0,THREAD_SIZE,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
		if(pRemoteCode == NULL)
			__leave;
		WriteProcessMemory(hProcess,pRemoteCode,RemoteThreadProc,THREAD_SIZE,&dwNumberOfBytes);

		hThread = CreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE)pRemoteCode,pRemoteData,0,&dwThreadId);
		if(hThread == NULL)
			__leave;

		WaitForSingleObject(hThread,INFINITE);

		ReadProcessMemory(hProcess,pRemoteData,&localData,sizeof(INJECT_DATA),&dwNumberOfBytes);

		strncpy(buf,localData.psw,c);
	}
	__finally{
		if(pRemoteData) VirtualFreeEx(hProcess,pRemoteData,0,MEM_RELEASE);
		if(pRemoteCode) VirtualFreeEx(hProcess,pRemoteCode,0,MEM_RELEASE);
		if(hThread) CloseHandle(hThread);
	}
	return 0;
}
