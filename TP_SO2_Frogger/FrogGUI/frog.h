#pragma once

#ifndef FROG

#include "../Server/structs.h"
#include "../Server/defines.h"

typedef struct FROGCOMMS FrogData, * pFrogData;

typedef struct FROGCOMMS {
	Game* game;
	HANDLE hPipe;
	HANDLE hMutex;
	HWND hWnd;
};

#endif