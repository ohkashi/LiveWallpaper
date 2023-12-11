// LiveWallpaper.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include "LiveWallpaper.h"
#include "MFPVideoPlayer.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
MFPVideoPlayer* g_pPlayer = nullptr;

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
HWND InitWindow(HWND hParent, int nCmdShow, int width, int height);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	HWND hwnd = FindWindowEx(hWnd, NULL, L"SHELLDLL_DefView", NULL);
	if (hwnd) {
		HWND* ret = (HWND*)lParam;
		*ret = FindWindowEx(NULL, hWnd, L"WorkerW", NULL);
	}
	return TRUE;
}

void RestoreWallPaper()
{
	WCHAR szWallPaper[MAX_PATH] = { 0 };
	SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID)szWallPaper, 0);
	SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)szWallPaper, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	(void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
		return 0;
	}

	hInst = hInstance; // Store instance handle in our global variable

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LIVE_WALLPAPER, szWindowClass, MAX_LOADSTRING);

	HWND hWorker = NULL;
	EnumWindows(EnumWindowsProc, (LPARAM)&hWorker);
	if (hWorker) {
		HWND hWnd = FindWindowEx(hWorker, NULL, szWindowClass, NULL);
		if (hWnd) {
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
	}

	if (__argc < 2) {
		RestoreWallPaper();
		return 0;
	}

	// Find Program Manager window
	HWND progman = FindWindow(L"Progman", NULL);

	//SendMessage(progman, SPI_GETTOGGLEKEYS, 4, 0);

	// Send 0x052C to Progman. This message directs Progman to spawn a 
	// WorkerW behind the desktop icons. If it is already there, nothing 
	// happens.
	SendMessageTimeout(progman, 0x052C, NULL, NULL, SMTO_NORMAL, 1000, NULL);

	// We enumerate all Windows, until we find one, that has the SHELLDLL_DefView 
	// as a child. 
	// If we found that window, we take its next sibling and assign it to workerw.
	hWorker = NULL;
	EnumWindows(EnumWindowsProc, (LPARAM)&hWorker);
	if (!hWorker) {
		return 1;
	}

	RECT rect;
	GetWindowRect(hWorker, &rect);
	int width = rect.right - rect.left, height = rect.bottom - rect.top;

    // Perform application initialization:
	MyRegisterClass(hInstance);
	HWND hWnd = InitWindow(hWorker, nCmdShow, width, height);
    if (!hWnd)
        return 0;

	g_pPlayer = new MFPVideoPlayer();
	HRESULT hr = g_pPlayer->PlayMediaFile(hWnd, __targv[1]);
	if (FAILED(hr)) {
		RestoreWallPaper();
		return 0;
	}

	// Message loop
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LIVE_WALLPAPER));
	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	delete g_pPlayer;
	g_pPlayer = nullptr;

	CoUninitialize();
    return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIVE_WALLPAPER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LIVE_WALLPAPER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = wcex.hIcon;
    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitWindow(HWND hParent, int nCmdShow, int width, int height)
{
   HWND hWnd = CreateWindowExW(
	   WS_EX_NOACTIVATE,
	   szWindowClass,
	   szTitle,
	   WS_CHILD | WS_VISIBLE,
	   0, 0,
	   width, height,
	   hParent,
	   NULL,
	   NULL,
	   NULL
   );

   if (!hWnd)
      return NULL;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			if (g_pPlayer)
				g_pPlayer->UpdateVideo();
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
