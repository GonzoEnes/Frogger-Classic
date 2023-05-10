#pragma once
#include <Windows.h>
#include "structs.h"
#ifndef GAME

typedef struct GAME Game, * pGame;

typedef struct GAME {
	DWORD rows;
	TCHAR board[10][20]; // max columns = 20 (ver dps) e max rows = ir buscar ao reg
	DWORD nCars;
	pCars cars;
	DWORD suspended;
	DWORD time;
	DWORD points;
	BOOL isShutdown;
	BOOL isSuspended;
	DWORD gameType; // 1 for solo, 2 for duo
};


#endif