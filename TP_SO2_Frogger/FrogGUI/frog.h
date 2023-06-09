#pragma once

#ifndef FROG

#include "../Server/structs.h"

typedef struct FROGCOMMS FrogData, * pFrogData;

typedef struct FROGCOMMS {
	Game* game;
	HANDLE hPipe;
	HWND hWnd;
};

#endif