#pragma once
#ifndef STRUCTS
#include <windows.h>
#include "defines.h"
#include "game.h"

typedef struct FROG_STRUCT Frog, * pFrog;
typedef struct CAR_STRUCT Car, * pCars;
typedef struct SHAREDMEMCOMMAND ShmCommand, * pShmCommand;
typedef struct SHAREDMEMGAME ShmGame, * pShmGame;
typedef struct COMMAND Command, * pCommand;
typedef struct DATA Data, * pData;

struct DATA {
	HANDLE hFileMapFrogger; // shm mm for game
	HANDLE hFileMapMemory; // memory from cmd
	pShmCommand sharedMemCmd; // access to commands from operator
	HANDLE hMutex; // locks access
	HANDLE hWriteSem;
	HANDLE mutexCmd; // locks cmds
	HANDLE hReadSem;
	DWORD time;
	Game game[2]; // game 1 for singleplayer and game2 for multiplayer
};

struct SHAREDMEMCOMMAND {
	Command operatorCmds[BUFFER]; // holds all shared mem for server -> operator
};

struct FROG_STRUCT {
	int nLives;
	//pos 
	//status
};

struct SHAREDMEMGAME {
	Game game[2];
};

struct COMMAND {
	DWORD parameter;
	DWORD parameter1;
	DWORD cmd;
};


struct CAR_STRUCT {
	int speed, nCar;
	BOOL isStopped;
	BOOL direction; // bool false para a esquerda, true para a direita só para orientar +-
};


#endif