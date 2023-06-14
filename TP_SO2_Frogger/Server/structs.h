#pragma once
#ifndef STRUCTS
#include <windows.h>
#include "defines.h"
#include "pipes.h"

typedef struct COMMAND Command, * pCommand;
typedef struct SHAREDMEMCOMMAND ShmCommand, * pShmCommand;
typedef struct SHAREDMEMGAME ShmGame, * pShmGame;
typedef struct DATA Data, * pData;
typedef struct FROG Player, * pPlayer;
typedef struct GAME Game, * pGame;
typedef struct REGISTRYCONFIG RegConfig, * pRegConfig;

struct REGISTRYCONFIG {
	HKEY key;
	TCHAR keyPath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
};

struct FROG {
	DWORD score;
	DWORD x, y;
	DWORD nLives;
	BOOL hasMoved; // for when he doens't move for 10 + secs
};

struct GAME {
    DWORD rows; // aka lanes
	TCHAR board[10][20];
	DWORD columns;// max columns = 20 (ver dps) e max rows = ir buscar ao reg
	DWORD nCars;
	BOOL direction;
	BOOL isMoving;
	Player player1;
	Player player2;
	DWORD nFrogs;
	DWORD level;
	DWORD carSpeed; // ver com o prof
	DWORD suspended;
	DWORD time; // o tempo de jogo
	BOOL isShutdown;
	BOOL isSuspended;
	DWORD gameType; // 1 for solo, 2 for duo
};

struct COMMAND {
	DWORD parameter;
	DWORD parameter1;
	DWORD cmd;
};

struct DATA {
	HANDLE hFileMapFrogger; // shm mm for game
	HANDLE hFileMapMemory; // memory from cmd
	pShmCommand sharedMemCmd; // access to commands from operator
	pShmGame sharedMemGame;
	HANDLE hMutex; // locks access
	HANDLE hWriteSem; // write semaphore
	HANDLE mutexCmd; // locks cmds
	HANDLE hReadSem; // read semaphore
	DWORD time; // tempo que o sv está a correr
	HANDLE hCmdEvent;
	Game game[2]; // game 1 for singleplayer and game2 for multiplayer
	pThreadData threadData;
	//Frog frog;
};

struct SHAREDMEMCOMMAND {
	Command operatorCmds[BUFFER]; // holds all shared mem for server -> operator
};

struct SHAREDMEMGAME {
	Game game[2];
};

#endif