#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "structs.h"

BOOL createSharedMemoryAndInit(pData p) {
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nConfigs for the game initializing...\n"));

	/*p->hFileMapFrogger = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_GAME);

	if (p->hMapFileGame == NULL) { // Map
		firstProcess = TRUE;
		p->hMapFileGame = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemGame), SHM_NAME_GAME);

		if (p->hMapFileGame == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMemGame = (SharedMemGame*)MapViewOfFile(p->hMapFileGame, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemGame)); // Shared Memory
	if (p->sharedMemGame == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
		CloseHandle(p->hMapFileGame);
		return FALSE;
	}*/

	p->hFileMapMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_MESSAGE);
	if (p->hFileMapMemory == NULL) {
		p->hFileMapMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShmCommand), SHM_NAME_MESSAGE);

		if (p->hFileMapMemory == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMemCmd = (ShmCommand*)MapViewOfFile(p->hFileMapMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ShmCommand));
	if (p->sharedMemCmd == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hFileMapMemory);
		return FALSE;
	}

	p->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (p->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d).\n"), GetLastError());
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	p->hWriteSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_WRITE_NAME);
	if (p->hWriteSem == NULL) {
		_tprintf(TEXT("\nError creating writting semaphore: (%d).\n"), GetLastError());
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	p->hReadSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_READ_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d).\n"), GetLastError());
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("\nTrying to run 2 servers at once, shutting down...\n"));
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	p->hCmdEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME);
	if (p->hCmdEvent == NULL) {
		_tprintf(TEXT("\nError creating command event (%d).\n"), GetLastError());
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->hFileMapMemory);
		return FALSE;
	}

	p->mutexCmd = CreateMutex(NULL, FALSE, COMMAND_MUTEX_NAME);
	if (p->mutexCmd == NULL) {
		_tprintf(TEXT("\nError creating command mutex.\n"));
		//UnmapViewOfFile(p->sharedMemGame);
		//CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		//CloseHandle(p->commandEvent);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	for (DWORD i = 0; i < BUFFERSIZE; i++) {
		p->sharedMemCmd->operatorCmds->parameter = 0;
		p->sharedMemCmd->operatorCmds->cmd = 0;
	}

	_tprintf(_T("\nEverything created successfully.\n"));
	return TRUE;
}

DWORD WINAPI decreaseTime(LPVOID params) {
	pData data = (pData)params;

	while (!data->game->isShutdown) {
		if (!data->game->isSuspended && data->time > 0) {
			data->time--;
			Sleep(1000);
		}
		else {
			// aqui voltar a meter os sapos na startLine
			return 1;
		}
	}
	return 0;
}

DWORD stopCars(pData data, int time) {
	_tprintf(_T("\nStopping cars for %d seconds.\n"), time);

	if (time < 0) {
		return -1;
	}

	if (data->game->nCars == 0) {
		return -2;
	}

	while (time > 0) {
		for (int i = 0; i < data->game->nCars; i++) {
			data->game->cars[i].isStopped = TRUE; // stop all cars within timeframe
		}
		time--;
		Sleep(1000);
	}

	return 1;
}

DWORD insertObstacle(pData data, int row, int column) {

	char obstacle = 'O'; // ver melhor

	if (data->game->board[row][column] != NULL /*|| row > MAX_ROWS && column > MAX_COLUMNS ver isto melhor*/) { // something is there
		return -1;
	}
	_tprintf(_T("\nInserting obstacle at position X: %d and Y: %d"), row, column); // tirar q isto é debug
	data->game->board[row][column] = obstacle; // insert obstacle in 
	return 1;
}

DWORD changeDirection(pData data) {
	data->game->nCars = 1; // debug tirar depois obv
	if (data->game->nCars == 0) {
		return -1;
	}

	_tprintf(_T("\nChanging direction of %d cars.\n"), data->game->nCars);

	for (int i = 0; i < data->game->nCars; i++) {
		switch (data->game->cars[i].direction) {
		case TRUE: // andar para a direita
			data->game->cars[i].direction = FALSE; // andar para a esquerda
			_tprintf(_T("\nDirections changed from LEFT - RIGHT to RIGHT - LEFT\n"));
		case FALSE:
			data->game->cars[i].direction = TRUE; // andar para a direita
			_tprintf(_T("\nDirections changed from RIGHT - LEFT to LEFT - RIGHT\n"));
		default:
			return -2;
		}
	}
}

DWORD WINAPI receiveCmdFromOperator(LPVOID params) {
	pData data = (pData)params;

	Command command; // command to receive

	int i = 0;

	do {
		if (data->game->isSuspended == FALSE) {
			WaitForSingleObject(data->hCmdEvent, INFINITE);
			WaitForSingleObject(data->mutexCmd, INFINITE);

			if (i == BUFFERSIZE)
				i = 0;

			CopyMemory(&command, &(data->sharedMemCmd->operatorCmds[i]), sizeof(Command)); // receive cmd from op
			
			i++;

			_tprintf(_T("\nReceived: %d"), command.cmd);

			ReleaseMutex(data->mutexCmd);

			if (command.cmd == 1) {
				stopCars(data, command.parameter); // received 1 from operator
			}
			if (command.cmd == 2) {
				insertObstacle(data, command.parameter, command.parameter1);
			}
			if (command.cmd == 3) {
				changeDirection(data);
			}
		}
	} while (data->game->isShutdown != TRUE);


	return 0;
}

	





	/*DWORD initEnvironment(Data* data) {

		for (DWORD i = 0; i < data->game[0].rows; i++) {
			for (DWORD j = 0; j < data->game[0].columns; j++) {
				if (data->game[0].gameType == 1) {
					_tcscpy_s(&data->game[0].board[i][j], sizeof(TCHAR), TEXT("-"));
				}
				else {
					for (int k = 0; k < 2; k++) {
						_tcscpy_s(&data->game[k].board[i][j], sizeof(TCHAR), TEXT("-"));
					}
				}
			}
		}

		return 1;
	}*/




int _tmain(int argc, TCHAR** argv) {

	// aqui são as vars
	HANDLE hReceiveCmdThread;
	Game game[2] = { 0 };
	Data data;

	data.game[0] = game[0];
	data.game[1] = game[1];

	data.game->cars = malloc(8 * sizeof(Car)); // debug tirar depois

	data.game[0].isShutdown = FALSE;
	data.game[1].isShutdown = FALSE;

	data.game[0].isSuspended = FALSE;
	data.game[1].isSuspended = FALSE;
	
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	if (!createSharedMemoryAndInit(&data)) {
		_tprintf(_T("\nCan't create shared memory.\n"));
		return 0;
	}

	hReceiveCmdThread = CreateThread(NULL, 0, receiveCmdFromOperator, &data, 0, NULL);
	if (hReceiveCmdThread == NULL) {
		_tprintf(_T("\nCan't create RECEIVECMDTHREAD [%d]"), GetLastError());
		return 0;
	}

	WaitForSingleObject(hReceiveCmdThread, INFINITE);
	
	CloseHandle(hReceiveCmdThread);
	
	return 0;
}