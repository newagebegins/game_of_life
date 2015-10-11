#include <windows.h>
#include <time.h>

const int gameWindowWidth = 700;
const int gameWindowHeight = 700;

const int cellWidthPx = 5;
const int cellHeightPx = 5;
const int cellCols = gameWindowWidth / cellWidthPx;
const int cellRows = gameWindowHeight / cellHeightPx;

int getLiveNeighbourCount(bool *cells, int row, int col) {
	int count = 0;
	const int maxNeighbourCount = 8;
	const int neighbours[maxNeighbourCount][2] = {
		{ -1, -1 },{ -1, 0 },{ -1, 1 },
		{  0, -1 },          {  0, 1 },
		{  1, -1 },{  1, 0 },{  1, 1 },
	};

	for (int i = 0; i < maxNeighbourCount; ++i) {
		int r = (row + neighbours[i][0]) % cellRows;
		if (r < 0)
			r += cellRows;
		int c = (col + neighbours[i][1]) % cellCols;
		if (c < 0)
			c += cellCols;
		bool *cell = cells + r*cellCols + c;
		if (*cell)
			++count;
	}
	
	return count;
}

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

	int *bitmapBuffer = (int*)malloc(bitmapWidth * bitmapHeight * sizeof(int));

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

	srand(time(NULL));

	bool *cells1 = (bool *)malloc(cellRows*cellCols*sizeof(bool));
	bool *cells2 = (bool *)malloc(cellRows*cellCols*sizeof(bool));

	bool *cells = cells1;
	bool *newCells = cells2;

	// Seed
	for (int row = 0; row < cellRows; ++row)
		for (int col = 0; col < cellCols; ++col)
			*(cells + row*cellCols + col) = rand() % 2;

	const float updateDuration = 0.1f;
	float updateTimer = 0.0f;

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

		updateTimer += dt;

		if (updateTimer > updateDuration) {
			updateTimer = 0;

			for (int row = 0; row < cellRows; ++row)
				for (int col = 0; col < cellCols; ++col) {
					*(newCells + row*cellCols + col) = *(cells + row*cellCols + col);
					int neighbourCount = getLiveNeighbourCount(cells, row, col);
					if (*(cells + row*cellCols + col)) {
						if (neighbourCount != 2 && neighbourCount != 3)
							*(newCells + row*cellCols + col) = false;
					}
					else {
						if (neighbourCount == 3)
							*(newCells + row*cellCols + col) = true;
					}
				}

			bool *tmp = cells;
			cells = newCells;
			newCells = tmp;
		}

		for (int row = 0; row < cellRows; ++row)
			for (int col = 0; col < cellCols; ++col) {
				int color = *(cells + row*cellCols + col) ? 0xFFFF0000 : 0xFF000000;
				for (int r = 0; r < cellHeightPx; ++r) {
					int *pixel = bitmapBuffer + (row*cellHeightPx + r)*cellCols*cellWidthPx + col*cellWidthPx;
					for (int c = 0; c < cellWidthPx; ++c) {
						*(pixel + c) = color;
					}
				}
			}

		HDC hDC = GetDC(hWnd);
		StretchDIBits(hDC, 0, 0, bitmapWidth, bitmapHeight, 0, 0, bitmapWidth, bitmapHeight,
			bitmapBuffer, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(hWnd, hDC);
	}

	return 0;
}
