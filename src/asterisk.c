#include <Windows.h>
#include <stdio.h>
#include "../res/resource.h"
#include "inject.h"

HINSTANCE hInst;
HWND hEdit;
HWND hPicture;
HBITMAP hBmpBlank,hBmpCross;
HCURSOR hCurCross,hCurHot,hCurNormal;
HWND hWndOld;

HWND SmallestWindowFromPoint( POINT* ppt )
{	
	RECT rect, rcTemp;
	HWND hParent, hWnd, hTemp;
	
	hWnd = WindowFromPoint( *ppt );
	if( hWnd != NULL )
	{
		GetWindowRect( hWnd, &rect );
		hParent = GetParent( hWnd );
		
		// Has window a parent?
		if( hParent != NULL )
		{
			// Search down the Z-Order
			hTemp = hWnd;
			do{
				hTemp = GetWindow( hTemp, GW_HWNDNEXT );
				
				// Search window contains the point, hase the same parent, and is visible?
				GetWindowRect( hTemp, &rcTemp );
				if(PtInRect(&rcTemp, *ppt) && GetParent(hTemp) == hParent && IsWindowVisible(hTemp))
				{
					// Is it smaller?
					if(((rcTemp.right - rcTemp.left) * (rcTemp.bottom - rcTemp.top)) < ((rect.right - rect.left) * (rect.bottom - rect.top)))
					{
						// Found new smaller window!
						hWnd = hTemp;
						GetWindowRect(hWnd, &rect);
					}
				}
			}while( hTemp != NULL );
		}
	}
	
	return hWnd;
}

void OnMouseMove(HWND hDlg,POINT* ppt)
{
	HWND hWndCursor=NULL;
	DWORD pid;
	DWORD style;
	char buf[128];
	
	hWndCursor = SmallestWindowFromPoint(ppt);
	GetWindowThreadProcessId(hWndCursor,&pid);
	if(GetCurrentProcessId()==pid){
		return;
	}
	if(hWndOld == hWndCursor){
		return;
	}
	hWndOld = hWndCursor;

	if(hWndCursor == NULL){
		SetDlgItemText(hDlg,IDC_EDIT_HWND,NULL);
		SetDlgItemText(hDlg,IDC_EDIT_PSW,NULL);
	}

	_snprintf(buf,sizeof(buf),"0x%08X",hWndCursor);
	SetDlgItemText(hDlg,IDC_EDIT_HWND,buf);
	
	style = (DWORD)GetWindowLong(hWndCursor,GWL_STYLE);

	GetClassName(hWndCursor,buf,sizeof(buf));
	if((!stricmp(buf,"EDIT") ||
		!stricmp(buf,"TEDIT") ||
		!stricmp(buf,"ThunderTextBox") ||
		!stricmp(buf,"ThunderRT6TextBox")) &&
		style & ES_PASSWORD)
	{
		HANDLE hProcess;

		SetCursor(hCurHot);
		hProcess = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
		if(hProcess != NULL){
			GetWindowTextRemote(hProcess,hWndCursor,buf,sizeof(buf));
			CloseHandle(hProcess);
		}else{
			*buf = '\0';
		}
	}else{
		SendMessage(hWndCursor,WM_GETTEXT,sizeof(buf),(LPARAM)buf);
		SetCursor(hCurCross);
	}
	SetDlgItemText(hDlg,IDC_EDIT_PSW,buf);
}

INT_PTR __stdcall MainDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{

	static BOOL bCapture = FALSE;

	switch(uMsg)
	{
	case WM_MOUSEMOVE:
		if(bCapture){
			POINT pt;
			pt.x = MAKEPOINTS(lParam).x;
			pt.y = MAKEPOINTS(lParam).y;
			ClientToScreen(hwndDlg,&pt);
			OnMouseMove(hwndDlg,&pt);
		}
		break;
	case WM_LBUTTONDOWN:
		{
			POINT pt;
			RECT rcPicture;
			pt.x = MAKEPOINTS(lParam).x;
			pt.y = MAKEPOINTS(lParam).y;
			ClientToScreen(hwndDlg,&pt);
			GetWindowRect(hPicture,&rcPicture);
			if(PtInRect(&rcPicture,pt)){
				bCapture = TRUE;
				SetCursor(hCurCross);
				SendMessage(hPicture,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBmpBlank);
				SetCapture(hwndDlg);
			}
			break;
		}
	case WM_LBUTTONUP:
	case WM_KILLFOCUS:
		if(bCapture){
			bCapture = FALSE;
			hWndOld = NULL;
			SetCursor(hCurNormal);
			SendMessage(hPicture,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBmpCross);
			ReleaseCapture();
		}
		break;
	case WM_INITDIALOG:
		hEdit = GetDlgItem(hwndDlg,IDC_EDIT_PSW);
		hPicture = GetDlgItem(hwndDlg,IDC_CAPTURE);
		hBmpBlank = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BLANK));
		hBmpCross = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_CROSS));
		hCurNormal = LoadCursor(NULL,IDC_ARROW);
		hCurCross = LoadCursor(hInst,MAKEINTRESOURCE(IDC_CURSOR));
		hCurHot = LoadCursor(hInst,MAKEINTRESOURCE(IDC_CURSOR_HOT));
		return 1;
	case WM_CLOSE:
		EndDialog(hwndDlg,0);
		return 0;
	case WM_COMMAND:
		{
			if(LOWORD(wParam)==IDC_STATIC_HTTP && HIWORD(wParam)==STN_CLICKED){
				ShellExecute(NULL,"open","http://www.cnblogs.com/nbsofer/archive/2013/06/08/3125525.html",NULL,NULL,SW_SHOWNORMAL);
				MessageBeep(MB_OK);
			}
			return 0;
		}
	}
	return 0;
}

int __stdcall WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	hInst = hInstance;
	DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,MainDlgProc,0);
	return 0;
}
