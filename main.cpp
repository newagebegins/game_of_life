#include <windows.h>

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"GameOfLifeWindowClass";

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, L"RegisterClass failed!", NULL, NULL);
		return 1;
	}

	const int gameWindowWidth = 960;
	const int gameWindowHeight = 540;

	RECT clientRect = { 0, 0, gameWindowWidth, gameWindowHeight };
	DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	AdjustWindowRect(&clientRect, windowStyle, NULL);
	HWND hWnd = CreateWindowEx(NULL, wc.lpszClassName, L"Game of Life", windowStyle, 300, 0,
		clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, NULL, NULL,	hInstance, NULL);

	if (!hWnd) {
		MessageBox(NULL, L"CreateWindowEx failed!", NULL, NULL);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//

	const int bitmapWidth = gameWindowWidth;
	const int bitmapHeight = gameWindowHeight;

	BITMAPINFO bitmapInfo = { 0 };
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = bitmapWidth;
	bitmapInfo.bmiHeader.biHeight = bitmapHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	const int bitmapBytesPerPixel = 4;
	const int bitmapMemorySize = bitmapWidth * bitmapHeight * bitmapBytesPerPixel;
	void *bitmapBuffer = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	{
		int *pixel = (int*)bitmapBuffer;
		for (int i = 0; i < bitmapMemorySize; i += bitmapBytesPerPixel) {
			*pixel = 0xFFFF0000;
			pixel++;
		}
	}

	//

	LARGE_INTEGER perfCounterFrequency = { 0 };
	QueryPerformanceFrequency(&perfCounterFrequency);
	LARGE_INTEGER perfCounter = { 0 };
	QueryPerformanceCounter(&perfCounter);
	LARGE_INTEGER prevPerfCounter = { 0 };
	
	float dt = 0.0f;
	const float targetFps = 60.0f;
	const float targetDt = 1.0f / targetFps;
	
	bool gameIsRunning = true;

	while (gameIsRunning) {
		prevPerfCounter = perfCounter;
		QueryPerformanceCounter(&perfCounter);
		dt = (float)(perfCounter.QuadPart - prevPerfCounter.QuadPart) / (float)perfCounterFrequency.QuadPart;
		if (dt > targetDt)
			dt = targetDt;

		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			switch (msg.message) {
			case WM_QUIT:
				gameIsRunning = false;
				break;
			case WM_KEYDOWN:
			case WM_KEYUP:
				switch (msg.wParam) {
				case VK_ESCAPE:
					gameIsRunning = false;
					break;
				}
				break;
			default:
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			}
		}

		HDC hDC = GetDC(hWnd);
		StretchDIBits(hDC, 0, 0, bitmapWidth, bitmapHeight, 0, 0, bitmapWidth, bitmapHeight,
			bitmapBuffer, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(hWnd, hDC);
	}

	return 0;
}
