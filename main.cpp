#include <windows.h>
#include <time.h>

static const int GAME_WIDTH = 700;
static const int GAME_HEIGHT = 700;

static const int CELL_WIDTH_PX = 5;
static const int CELL_HEIGHT_PX = 5;

static const int CELL_COLS = GAME_WIDTH / CELL_WIDTH_PX;
static const int CELL_ROWS = GAME_HEIGHT / CELL_HEIGHT_PX;

static const int CELL_COLOR_ALIVE = 0xFFFF0000;
static const int CELL_COLOR_DEAD = 0xFF000000;

static const float TICK_DURATION = 0.1f;

inline int getLiveNeighbourCount(bool *cells, int row, int col) {
	int count = 0;
	const int NEIGHBOUR_COUNT = 8;
	int neighbours[NEIGHBOUR_COUNT][2] = {
		{ -1, -1 },{ -1, 0 },{ -1, 1 },
		{  0, -1 },          {  0, 1 },
		{  1, -1 },{  1, 0 },{  1, 1 },
	};

	for (int i = 0; i < NEIGHBOUR_COUNT; ++i) {
		int r = (row + neighbours[i][0]) % CELL_ROWS;
		if (r < 0)
			r += CELL_ROWS;
		int c = (col + neighbours[i][1]) % CELL_COLS;
		if (c < 0)
			c += CELL_COLS;
		bool *cell = cells + r*CELL_COLS + c;
		if (*cell)
			++count;
	}
	
	return count;
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

	RECT clientRect = { 0, 0, GAME_WIDTH, GAME_HEIGHT };
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

	const int bitmapWidth = GAME_WIDTH;
	const int bitmapHeight = GAME_HEIGHT;

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
	
	srand(time(NULL));

	bool *cells1 = (bool *)malloc(CELL_ROWS*CELL_COLS*sizeof(bool));
	bool *cells2 = (bool *)malloc(CELL_ROWS*CELL_COLS*sizeof(bool));

	bool *cells = cells1;
	bool *newCells = cells2;

	// Seed
	for (int row = 0; row < CELL_ROWS; ++row)
		for (int col = 0; col < CELL_COLS; ++col)
			*(cells + row*CELL_COLS + col) = rand() % 2;

	float tickTimer = 0.0f;
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

		// UPDATE

		tickTimer += dt;

		if (tickTimer > TICK_DURATION) {
			tickTimer = 0;

			for (int row = 0; row < CELL_ROWS; ++row)
				for (int col = 0; col < CELL_COLS; ++col) {
					bool *cell = cells + row*CELL_COLS + col;
					bool *newCell = newCells + row*CELL_COLS + col;
					*newCell = *cell;
					int liveNeighbours = getLiveNeighbourCount(cells, row, col);
					if (*cell) {
						if (liveNeighbours < 2 || liveNeighbours > 3)
							*newCell = false;
					}
					else {
						if (liveNeighbours == 3)
							*newCell = true;
					}
				}

			bool *tmp = cells;
			cells = newCells;
			newCells = tmp;
		}

		// RENDER

		for (int row = 0; row < CELL_ROWS; ++row) {
			for (int col = 0; col < CELL_COLS; ++col) {
				int color = *(cells + row*CELL_COLS + col) ? CELL_COLOR_ALIVE : CELL_COLOR_DEAD;
				for (int r = 0; r < CELL_HEIGHT_PX; ++r) {
					for (int c = 0; c < CELL_WIDTH_PX; ++c) {
						int *pixel = bitmapBuffer + (row*CELL_HEIGHT_PX + r)*CELL_COLS*CELL_WIDTH_PX + col*CELL_WIDTH_PX + c;
						*pixel = color;
					}
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
