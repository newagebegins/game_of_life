#include <windows.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>

static const int DEFAULT_WINDOW_WIDTH = 500;
static const int DEFAULT_WINDOW_HEIGHT = 500;
static const int CELL_WIDTH_PX = 5;
static const int CELL_HEIGHT_PX = 5;
static const int CELL_COLOR_ALIVE = 0xFFFF0000;
static const int CELL_COLOR_DEAD = 0xFF000000;
static const float TICK_DURATION = 0.1f;

static int globalWindowWidth;
static int globalWindowHeight;

struct GameState {
	int cellCols, cellRows;
	bool *cells1, *cells2;
	bool *cells, *newCells;
	float tickTimer;
	BITMAPINFO *bitmapInfo;
	int *bitmapBuffer;
	int bitmapWidth, bitmapHeight;
};

inline int getLiveNeighbourCount(GameState *state, const int row, const int col) {
	int count = 0;
	const int NEIGHBOUR_COUNT = 8;
	const int neighbours[NEIGHBOUR_COUNT][2] = {
		{ -1, -1 },{ -1, 0 },{ -1, 1 },
		{  0, -1 },          {  0, 1 },
		{  1, -1 },{  1, 0 },{  1, 1 },
	};

	for (int i = 0; i < NEIGHBOUR_COUNT; ++i) {
		int r = (row + neighbours[i][0]) % state->cellRows;
		if (r < 0) r += state->cellRows;
		int c = (col + neighbours[i][1]) % state->cellCols;
		if (c < 0) c += state->cellCols;
		bool *cell = state->cells + r*state->cellCols + c;
		if (*cell) ++count;
	}
	
	return count;
}

static void initGame(GameState *state, const int windowWidth, const int windowHeight) {
	state->bitmapWidth = (windowWidth / CELL_WIDTH_PX) * CELL_WIDTH_PX;
	state->bitmapHeight = (windowHeight / CELL_HEIGHT_PX) * CELL_HEIGHT_PX;

	state->cellCols = state->bitmapWidth / CELL_WIDTH_PX;
	state->cellRows = state->bitmapHeight / CELL_HEIGHT_PX;

	if (state->cells1) free(state->cells1);
	state->cells1 = (bool *)malloc(state->cellRows * state->cellCols * sizeof(bool));
	
	if (state->cells2) free(state->cells2);
	state->cells2 = (bool *)malloc(state->cellRows * state->cellCols * sizeof(bool));

	state->cells = state->cells1;
	state->newCells = state->cells2;

	// seed initial cells
	for (int row = 0; row < state->cellRows; ++row)
		for (int col = 0; col < state->cellCols; ++col)
			*(state->cells + row*state->cellCols + col) = (rand() % 2) != 0;

	state->tickTimer = 0.0f;

	if (state->bitmapInfo) free(state->bitmapInfo);
	state->bitmapInfo = (BITMAPINFO *)malloc(sizeof(BITMAPINFO));
	state->bitmapInfo->bmiHeader.biSize = sizeof(state->bitmapInfo->bmiHeader);
	state->bitmapInfo->bmiHeader.biWidth = state->bitmapWidth;
	state->bitmapInfo->bmiHeader.biHeight = -state->bitmapHeight;
	state->bitmapInfo->bmiHeader.biPlanes = 1;
	state->bitmapInfo->bmiHeader.biBitCount = 32;
	state->bitmapInfo->bmiHeader.biCompression = BI_RGB;

	if (state->bitmapBuffer) free(state->bitmapBuffer);
	state->bitmapBuffer = (int*)malloc(state->bitmapWidth * state->bitmapHeight * sizeof(int));
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		globalWindowWidth = LOWORD(lParam);
		globalWindowHeight = HIWORD(lParam);
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

	int windowWidth = globalWindowWidth = DEFAULT_WINDOW_WIDTH;
	int windowHeight = globalWindowHeight = DEFAULT_WINDOW_HEIGHT;

	RECT clientRect = { 0, 0, windowWidth, windowHeight };
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

	LARGE_INTEGER perfCounterFrequency = { 0 };
	QueryPerformanceFrequency(&perfCounterFrequency);
	LARGE_INTEGER perfCounter = { 0 };
	QueryPerformanceCounter(&perfCounter);
	LARGE_INTEGER prevPerfCounter = { 0 };
	
	float dt = 0.0f;
	float realDt = dt;
	const float targetFps = 60.0f;
	const float targetDt = 1.0f / targetFps;
	
	srand((unsigned int)time(NULL));

	GameState *gameState = (GameState *)malloc(sizeof(GameState));
	memset(gameState, 0, sizeof(GameState));
	initGame(gameState, windowWidth, windowHeight);

	bool gameIsRunning = true;

	while (gameIsRunning) {
		prevPerfCounter = perfCounter;
		QueryPerformanceCounter(&perfCounter);
		dt = (float)(perfCounter.QuadPart - prevPerfCounter.QuadPart) / (float)perfCounterFrequency.QuadPart;
		realDt = dt;
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

		//
		// UPDATE
		//

		// When the window is resized, reinitialize the game with the new window size.
		if (globalWindowWidth != windowWidth || globalWindowHeight != windowHeight) {
			windowWidth = globalWindowWidth;
			windowHeight = globalWindowHeight;
			initGame(gameState, globalWindowWidth, globalWindowHeight);
		}

		// Handle the left mouse button: make the clicked cell alive.
		if (GetKeyState(VK_LBUTTON) & (1 << 15)) {
			POINT mousePos;
			GetCursorPos(&mousePos);
			ScreenToClient(hWnd, &mousePos);

			int row = mousePos.y / CELL_HEIGHT_PX;
			int col = mousePos.x / CELL_WIDTH_PX;

			if (row < gameState->cellRows && col < gameState->cellCols) {
				bool *cell = gameState->cells + row*gameState->cellCols + col;
				*cell = true;
			}
		}

		gameState->tickTimer += dt;
		if (gameState->tickTimer > TICK_DURATION) {
			gameState->tickTimer = 0;

			for (int row = 0; row < gameState->cellRows; ++row) {
				for (int col = 0; col < gameState->cellCols; ++col) {
					bool *cell = gameState->cells + row*gameState->cellCols + col;
					bool *newCell = gameState->newCells + row*gameState->cellCols + col;
					*newCell = *cell;
					int liveNeighbours = getLiveNeighbourCount(gameState, row, col);
					if (*cell) {
						if (liveNeighbours < 2 || liveNeighbours > 3)
							*newCell = false;
					}
					else {
						if (liveNeighbours == 3)
							*newCell = true;
					}
				}
			}

			bool *tmp = gameState->cells;
			gameState->cells = gameState->newCells;
			gameState->newCells = tmp;
		}

		//
		// RENDER
		//

		for (int row = 0; row < gameState->cellRows; ++row) {
			for (int col = 0; col < gameState->cellCols; ++col) {
				bool *cell = gameState->cells + row*gameState->cellCols + col;
				int color = *cell ? CELL_COLOR_ALIVE : CELL_COLOR_DEAD;
				for (int r = 0; r < CELL_HEIGHT_PX; ++r) {
					for (int c = 0; c < CELL_WIDTH_PX; ++c) {
						int *pixel = gameState->bitmapBuffer + (row*CELL_HEIGHT_PX + r)*gameState->cellCols*CELL_WIDTH_PX + col*CELL_WIDTH_PX + c;
						*pixel = color;
					}
				}
			}
		}

		HDC hDC = GetDC(hWnd);
		StretchDIBits(hDC, 0, 0, gameState->bitmapWidth, gameState->bitmapHeight,
			0, 0, gameState->bitmapWidth, gameState->bitmapHeight,
			gameState->bitmapBuffer, gameState->bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(hWnd, hDC);

#if 0
		char textBuffer[256];
		sprintf(textBuffer, "dt: %f, fps: %f\n", realDt, 1.0f/realDt);
		OutputDebugStringA(textBuffer);
#endif
	}

	return 0;
}
