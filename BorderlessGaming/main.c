#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <CommCtrl.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDT_TIMER1 101
#define ID_TRAY_SHOW 102
#define ID_TRAY_HIDE 103
#define ID_TRAY_EXIT 104
#define IDC_ADD 105
#define IDC_REMOVE 106
#define IDC_LIST 107
#define WC_WINDOW _T("BorderlessGamingWindow")
#define WM_TRAY_ICON (WM_USER + 1)

NOTIFYICONDATA g_nid;
HWND g_hBtnAdd;
HWND g_hBtnRemove;
HWND g_hEdit;
HWND g_hListBox;

void ShowError(LPCTSTR msg)
{
	_tprintf(_T("%s"), msg);

	OutputDebugString(msg);
	OutputDebugString(_T("\n"));

	DWORD err = GetLastError();
	if (err == ERROR_SUCCESS)
		return;

	LPTSTR lpMsg = NULL;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		&lpMsg,
		0,
		NULL
	);

	MessageBox(NULL, lpMsg, msg, MB_ICONERROR | MB_OK);

	LocalFree(lpMsg);
}

void ShowContextMenu(HWND hWnd, const POINT* pt)
{
	HMENU hMenu = CreatePopupMenu();
	if (!hMenu)
	{
		ShowError(_T("Popup menu creation failed!"));
		PostQuitMessage(EXIT_FAILURE);
		return;
	}

	if (IsWindowVisible(hWnd))
		AppendMenu(hMenu, MF_STRING, ID_TRAY_HIDE, _T("Hide"));
	else
		AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, _T("Show"));
	AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, _T("Exit"));
		
	SetForegroundWindow(hWnd);
	
	if (!TrackPopupMenu(hMenu, NULL, pt->x, pt->y, 0, hWnd, NULL))
	{
		ShowError(_T("Popup menu tracking failed!"));
		PostQuitMessage(EXIT_FAILURE);
		return;
	}

	DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT info = (LPCREATESTRUCT)lParam;
		HINSTANCE hInst = info->hInstance;

		RECT rc;
		ZeroMemory(&rc, sizeof(RECT));
		rc.right = 300;
		rc.bottom = 300;
		AdjustWindowRect(&rc, info->style, FALSE);
		SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

		g_hBtnAdd = CreateWindow(_T("BUTTON"), _T("Add"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 300 - 60, 5, 50, 20, hWnd, IDC_ADD, hInst, NULL);
		g_hBtnRemove = CreateWindow(_T("BUTTON"), _T("Remove"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 5, 300 - 30, 300 - 15, 20, hWnd, IDC_REMOVE, hInst, NULL);
		g_hEdit = CreateWindow(_T("EDIT"), NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 5, 5, 300 - 15 - 55, 20, hWnd, NULL, hInst, NULL);
		g_hListBox = CreateWindow(_T("LISTBOX"), NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_WANTKEYBOARDINPUT, 5, 30, 300 - 15, 300 - 55, hWnd, IDC_LIST, hInst, NULL);

		SendMessage(g_hBtnAdd, WM_SETFONT, GetStockObject(DEFAULT_GUI_FONT), TRUE);
		SendMessage(g_hBtnRemove, WM_SETFONT, GetStockObject(DEFAULT_GUI_FONT), TRUE);
		SendMessage(g_hEdit, WM_SETFONT, GetStockObject(DEFAULT_GUI_FONT), TRUE);
		SendMessage(g_hListBox, WM_SETFONT, GetStockObject(DEFAULT_GUI_FONT), TRUE);

		TCHAR buffer[4096];
		ZeroMemory(&buffer, sizeof(buffer));
		GetPrivateProfileString(_T("Borderless"), _T("Windows"), _T(""), &buffer, ARRAYSIZE(buffer), _T("./settings.ini"));

		SIZE_T len = 0;
		SIZE_T delimiterCount = 0;
		while (buffer[len] != _T('\0'))
		{
			if (buffer[len] == _T(','))
				delimiterCount++;
			len++;
		}

		if (len == 0)
			break;

		SIZE_T count = delimiterCount + 1;
		LPTSTR* arr;
		arr = LocalAlloc(LPTR, sizeof(LPTSTR) * count);
		if (!arr)
		{
			ShowError(_T("Allocation failed!"));
			PostQuitMessage(EXIT_FAILURE);
			break;
		}
		for (SIZE_T i = 0; i < count; i++)
		{
			arr[i] = LocalAlloc(LPTR, sizeof(TCHAR) * MAX_PATH);
			if (!arr[i])
			{
				ShowError(_T("Allocation failed!"));
				PostQuitMessage(EXIT_FAILURE);
				return 0;
			}
		}

		SIZE_T start = 0, end = 0, segment = 0;
		while (buffer[end] != _T('\0'))
		{
			if (buffer[end] == _T(','))
			{
				SIZE_T length = end - start;
				_tcsnccpy_s(arr[segment], MAX_PATH, &buffer[start], length);
				segment++;
				start = end + 1;
			}
			end++;
		}

		if (start < end)
		{
			SIZE_T length = end - start;
			_tcsnccpy_s(arr[segment], MAX_PATH, &buffer[start], length);
		}

		for (SIZE_T i = 0; i < count; i++)
			ListBox_AddString(g_hListBox, arr[i]);

		for (SIZE_T i = 0; i < count; i++)
			LocalFree(arr[i]);
		LocalFree(arr);
		break;
	}

	case WM_DESTROY:
	{
		TCHAR buffer[4096];
		ZeroMemory(&buffer, sizeof(buffer));

		int count = ListBox_GetCount(g_hListBox);
		int offset = 0;
		for (int i = 0; i < count; i++)
		{
			TCHAR tmp[MAX_PATH];
			ZeroMemory(&buffer, sizeof(buffer));
			int len = ListBox_GetText(g_hListBox, i, &tmp);
			_tcsnccpy_s(&buffer[offset], ARRAYSIZE(buffer) - offset, tmp, len);
			offset += len;
			if (i < count - 1)
			{
				_tcsnccpy_s(&buffer[offset], ARRAYSIZE(buffer) - offset, _T(","), ARRAYSIZE(_T(",")));
				offset += ARRAYSIZE(_T(","));
			}
		}

		WritePrivateProfileString(_T("Borderless"), _T("Windows"), &buffer, _T("./settings.ini"));

		DestroyWindow(g_hListBox);
		DestroyWindow(g_hEdit);
		DestroyWindow(g_hBtnRemove);
		DestroyWindow(g_hBtnAdd);
		break;
	}

	case WM_TRAY_ICON:
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
			ShowWindow(hWnd, SW_SHOW);
			break;

		case WM_RBUTTONDOWN:
		{
			POINT pt;
			GetCursorPos(&pt);
			ShowContextMenu(hWnd, &pt);
			break;
		}
		}
		break;

	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 0;

	case WM_TIMER:
		if (wParam == IDT_TIMER1)
		{
			int count = SendMessage(g_hListBox, LB_GETCOUNT, NULL, NULL);
			for (int i = 0; i < count; i++)
			{
				TCHAR buffer[MAX_PATH];
				ZeroMemory(&buffer, sizeof(buffer));
				ListBox_GetText(g_hListBox, i, &buffer);

				HWND hTarget = FindWindow(NULL, buffer);
				if (!hTarget)
					continue;

				DWORD style = GetWindowLong(hTarget, GWL_STYLE);
				if ((style & WS_POPUP) == 0)
				{
					SetWindowLong(hTarget, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_MAXIMIZE | WS_VISIBLE);
					UpdateWindow(hTarget);
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TRAY_SHOW:
			ShowWindow(hWnd, SW_SHOW);
			break;

		case ID_TRAY_HIDE:
			ShowWindow(hWnd, SW_HIDE);
			break;

		case ID_TRAY_EXIT:
			PostQuitMessage(EXIT_SUCCESS);
			break;

		case IDC_ADD:
		{
			if (GetWindowTextLength(g_hEdit) > 0)
			{
				TCHAR buffer[MAX_PATH];
				ZeroMemory(&buffer, sizeof(buffer));
				GetWindowText(g_hEdit, &buffer, MAX_PATH);
				SetWindowText(g_hEdit, NULL);
				ListBox_AddString(g_hListBox, &buffer);
			}
			break;
		}

		case IDC_REMOVE:
		{
			int index = ListBox_GetCurSel(lParam);
			if (index == -1)
				break;
			ListBox_DeleteString(g_hListBox, index);
			break;
		}
		}
		break;

	case WM_VKEYTOITEM:
	{
		switch (LOWORD(wParam))
		{
		case VK_DELETE:
		{
			int index = ListBox_GetCurSel(lParam);
			if (index == -1)
				break;
			ListBox_DeleteString(lParam, index);
			break;
		}
		}
		break;
	}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

_Use_decl_annotations_
int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nCmdShow)
{
	HANDLE hMutex = CreateMutex(NULL, FALSE, WC_WINDOW);
	if (!hMutex)
	{
		ShowError(_T("Mutex creation error!"));
		return EXIT_FAILURE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		HWND hWindow = FindWindow(WC_WINDOW, NULL);
		ShowWindow(hWindow, SW_SHOW);
		CloseHandle(hMutex);
		return EXIT_SUCCESS;
	}

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	wc.lpszClassName = WC_WINDOW;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		ShowError(_T("Window class registration failed!"));
		return EXIT_FAILURE;
	}

	HWND hWindow = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, WC_WINDOW, _T("Borderless Gaming"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	if (!hWindow)
	{
		ShowError(_T("Window creation failed!"));
		return EXIT_FAILURE;
	}

	ZeroMemory(&g_nid, sizeof(NOTIFYICONDATA));
	g_nid.cbSize = sizeof(NOTIFYICONDATA);
	g_nid.hWnd = hWindow;
	g_nid.uID = 1;
	g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_nid.uCallbackMessage = WM_TRAY_ICON;
	g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	_tcsnccpy_s(g_nid.szTip, ARRAYSIZE(g_nid.szTip), _T("Borderless Gaming"), ARRAYSIZE(_T("Borderless Gaming")));

	Shell_NotifyIcon(NIM_ADD, &g_nid);

	SetTimer(hWindow, IDT_TIMER1, 5000, NULL);

	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(hWindow, IDT_TIMER1);
	Shell_NotifyIcon(NIM_DELETE, &g_nid);
	DestroyWindow(hWindow);
	UnregisterClass(WC_WINDOW, hInst);
	CloseHandle(hMutex);
	
	return msg.wParam;
}