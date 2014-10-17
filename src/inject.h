#ifndef __INJECT_H__
#define __INJECT_H__
#include <Windows.h>
#include <stdio.h>
int GetWindowTextRemote(HANDLE hProcess,HWND hWnd,char* buf,int c);

#endif//!__INJECT_H__
