// PasswordDlg.cpp : implementation file
//
#include "stdafx.h"
#include "PerforcePlugin.h"
#include "PasswordDlg.h"

extern HINSTANCE g_hInstance;

namespace PerforceConnection
{
	#define MAX_STR 1023
	CString strPassword;
	CString strClient;
	CString strUser;
	CString strPort;
	bool isOnline;

	// PasswordDlg message handlers

	INT_PTR CALLBACK PasswordDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
	{
		HWND pwwnd = GetDlgItem(hWnd, IDC_PASSWORD);
		HWND clientwnd = GetDlgItem(hWnd, IDC_WORKSPACE);
		HWND userwnd = GetDlgItem(hWnd, IDC_USERNAME);
		HWND portwnd = GetDlgItem(hWnd, IDC_PORT);
		switch(message) 
		{
		case WM_INITDIALOG:
			if(pwwnd)
			{
				SetWindowText(pwwnd, strPassword);
			}
			if(clientwnd)
			{
				SetWindowText(clientwnd, strClient);
			}
			if(userwnd)
			{
				SetWindowText(userwnd, strUser);
			}
			if(portwnd)
			{
				SetWindowText(portwnd, strPort);
			}

			if(isOnline)
				CheckDlgButton(hWnd, IDC_WORKONLINE, BST_CHECKED);

			ShowWindow(hWnd, SW_SHOW);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDOK:
				{
					if(pwwnd)
					{
						GetWindowText(pwwnd, strPassword.GetBuffer(MAX_STR), MAX_STR);
					}
					if(clientwnd)
					{
						GetWindowText(clientwnd, strClient.GetBuffer(MAX_STR), MAX_STR);
					}
					if(userwnd)
					{
						GetWindowText(userwnd, strUser.GetBuffer(MAX_STR), MAX_STR);
					}
					if(portwnd)
					{
						GetWindowText(portwnd, strPort.GetBuffer(MAX_STR), MAX_STR);
					}
					isOnline = IsDlgButtonChecked(hWnd, IDC_WORKONLINE);
					EndDialog(hWnd, IDOK);
				}
				break;

			case IDCANCEL:
				{
					EndDialog(hWnd, IDCANCEL);
				}
				break;
			}
			break;
		}
		return FALSE;
	}

	bool OpenPasswordDlg(CString& port, CString& user, CString& client, CString& passwd, bool& bIsWorkOffline)
	{
		strPort = port;
		strUser = user;
		strClient = client;
		strPassword = passwd;
		isOnline = !bIsWorkOffline;

		int res=DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_PASSWD), GetActiveWindow(), PasswordDlgProc, 0);
		if(res==IDOK)
		{
			bIsWorkOffline = !isOnline;
			if (isOnline)
			{
				port = strPort;
				user = strUser;
				client = strClient;
				passwd = strPassword;
			}
			return true;
		}
		return false;
	}
}
