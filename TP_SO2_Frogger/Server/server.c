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

	p->hFileMapFrogger = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_GAME);

	if (p->hFileMapFrogger == NULL) { // Map
		firstProcess = TRUE;
		p->hFileMapFrogger = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShmGame), SHM_NAME_GAME);

		if (p->hFileMapFrogger == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMemGame = (pShmGame)MapViewOfFile(p->hFileMapFrogger, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ShmGame)); // Shared Memory
	if (p->sharedMemGame == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
		CloseHandle(p->hFileMapFrogger);
		return FALSE;
	}

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
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hFileMapFrogger);
		CloseHandle(p->hFileMapMemory);
		return FALSE;
	}

	p->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (p->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hFileMapFrogger);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	p->hWriteSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_WRITE_NAME);
	if (p->hWriteSem == NULL) {
		_tprintf(TEXT("\nError creating writting semaphore: (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hFileMapFrogger);
		CloseHandle(p->hMutex);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	p->hReadSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_READ_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hFileMapFrogger);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hFileMapMemory);
		UnmapViewOfFile(p->sharedMemCmd);
		return FALSE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) { // after failing to create sem, if the error provided is ALREADY_EXISTS assume 2 servers are being run
		_tprintf(TEXT("\nTrying to run 2 servers at once, shutting down...\n"));
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hFileMapFrogger);
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
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hFileMapFrogger);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->hCmdEvent);
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

DWORD stopCars(pData data, INT time) {
	_tprintf(_T("\nStopping cars for %d seconds.\n"), time);

	if (time < 0) {
		return -1;
	}

	if (data->game->nCars == 0) {
		return -2;
	}

	while (time > 0) {
		data->game->isMoving = FALSE;
		time--;
		Sleep(1000);
	}

	return 1;
}

DWORD insertObstacle(pData data, INT row, INT column) {

	TCHAR obstacle = _T('O'); // ver melhor

	if (data->game->board[row][column] != NULL /*|| row > MAX_ROWS && column > MAX_COLUMNS ver isto melhor*/) { // something is there
		return -1;
	}
	_tprintf(_T("\nInserting obstacle at position X: %d and Y: %d"), row, column); // tirar q isto é debug
	data->game->board[row][column] = obstacle; // insert obstacle in 
	return 1;
}

DWORD insertFrog(pData data) { // from shared memory give to operator
	DWORD aux;

	for (DWORD i = 0; i < data->game[0].rows; i++) {
		for (DWORD j = 0; j < data->game[0].columns; j++) {
			if (data->game[0].nFrogs == 0 && (i == data->game->rows - 1)) { //&& strcmp(data->game[0].board[i][j],_T("-")==0)) {
				while (data->game[0].nFrogs < 2) {
					aux = rand() % data->game->columns;
					data->game[0].board[i][aux] = _T('s');
					data->game[0].nFrogs++;
				}
			}
		}
	}
}

BOOL moveCars(pData data) {
	if(data->game[0].isMoving==TRUE){
	for (DWORD i = 1; i < data->game[0].rows - 1; i++) { // iterate rows
		for (DWORD j = data->game[0].columns - 1; j > 0; j--) { // iterate columns
			if (j == data->game[0].columns - 1 && data->game[0].board[i][j] == _T('c')) { // if we are at last col and has car
				TCHAR prevElement = data->game[0].board[i][j - 1]; // store prev element before moving to the first spot of row
				data->game[0].board[i][0] = data->game[0].board[i][j]; // give the last element of row to the first
				data->game[0].board[i][j] = prevElement; // give last element the prev element before switching
			}

			else if (j == 0 && data->game[0].board[i][j] == _T('c')) { // if we are at first element of row and its a car
				data->game[0].board[i][j + 1] = data->game[0].board[i][j]; // give next element its own value (move 'c' to right)
			}

			else if (data->game[0].board[i][j] == _T('c') && j != 0 && j != data->game[0].columns - 1) {
				TCHAR prevElement = data->game[0].board[i][j - 1]; // otherwise move normally 
				data->game[0].board[i][j + 1] = data->game[0].board[i][j];
				data->game[0].board[i][j] = prevElement;
				}
			}
		}
	}
	else { return FALSE; }

}


DWORD changeDirection(pData data) {
	data->game->nCars = 1; // debug tirar depois obv
	if (data->game->nCars == 0) {
		return -1;
	}

	_tprintf(_T("\nChanging direction of %d cars.\n"), data->game->nCars);

	for (int i = 0; i < data->game->nCars; i++) {/* 
		switch (data->game->cars[i].direction) {
		case TRUE: // andar para a direita
			data->game->cars[i].direction = FALSE; // andar para a esquerda
			_tprintf(_T("\nDirections changed from LEFT - RIGHT to RIGHT - LEFT\n"));
		case FALSE:
			data->game->cars[i].direction = TRUE; // andar para a direita
			_tprintf(_T("\nDirections changed from RIGHT - LEFT to LEFT - RIGHT\n"));
		default:
			return -2;
		}*/
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

DWORD WINAPI sendGameData(LPVOID params) {
	pData data = (pData)params;

	while (!data->game[0].isShutdown) {
		WaitForSingleObject(data->hWriteSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);

		// check for multiplayer na prox meta, para já copiar apenas os dados

		CopyMemory(&data->sharedMemGame->game[0], &data->game[0], sizeof(Game)); // copia para dentro da shmMem para depois o op ler

		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hWriteSem, 1, NULL); // aqui é o sem de escrita porque vai escrever para dentro do pointer da shmMem
	}
}

BOOL readRegConfigs(pData data, pRegConfig reg) {
	//check if it can open reg key

	DWORD size = SIZE_DWORD;
	//TCHAR key_path[MAX] = _T("SOFTWARE\\TP_SO2\\Values");

	_tcscpy_s(reg->keyPath, BUFFER, _T("SOFTWARE\\TP_SO2"));

	if (RegOpenKeyEx(HKEY_CURRENT_USER, reg->keyPath, 0, KEY_ALL_ACCESS, &reg->key) != ERROR_SUCCESS) {
		_tprintf(_T("\nKey doesn't exist. Creating...\n"));

		if (RegCreateKeyEx(HKEY_CURRENT_USER, reg->keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &reg->key, &reg->dposition) == ERROR_SUCCESS) {
			_tprintf(_T("\nKey created successfully.\n"));
			// atribuir os valores para dentro da struct

			DWORD lanes = 10;
			DWORD speed = 5; // change if necessary

			DWORD setLanes = RegSetValueEx(reg->key, _T("Lanes"), 0, REG_DWORD, (LPBYTE)&lanes, sizeof(DWORD), &size);
			DWORD setSpeed = RegSetValueEx(reg->key, _T("Speed"), 0, REG_DWORD, (LPBYTE)&speed, sizeof(DWORD), &size);

			if (setLanes != ERROR_SUCCESS || setSpeed != ERROR_SUCCESS) {
				_tprintf(_T("\nCan't set values for Frogger.\n"));
				return FALSE;
			}
		}
		else {
			_tprintf(TEXT("\nCouldn't create key in SOFTWARE\\TP_SO2.\n"));
			return FALSE;
		}
	}

	// after values are set, put them in the struct

	// check for gameType (solo/duo) later

	DWORD readLanes = RegQueryValueEx(reg->key, _T("Lanes"), NULL, NULL, (LPBYTE)&data->game[0].rows, &size);
	DWORD readSpeed = RegQueryValueEx(reg->key, _T("Speed"), NULL, NULL, (LPBYTE)&data->game[0].carSpeed, &size);

	if (readLanes != ERROR_SUCCESS || readSpeed != ERROR_SUCCESS) {
		_tprintf(_T("\n[ERROR] Can't read values from Registry.\n"));
		return FALSE;
	}

	return TRUE;
}

int _tmain(int argc, TCHAR** argv) {

	// aqui são as vars
	HANDLE hReceiveCmdThread;
	HANDLE hSendGameDataThread;
	Game game[2] = { 0 };
	RegConfig reg = {0};
	Data data;

	data.game[0] = game[0];
	data.game[1] = game[1];
 

	data.game[0].isShutdown = FALSE;
	data.game[1].isShutdown = FALSE;

	data.game[0].isSuspended = FALSE;
	data.game[1].isSuspended = FALSE;
	

	data.game->nCars = 8;

#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

	if (!createSharedMemoryAndInit(&data)) {
		_tprintf(_T("\nCan't create shared memory.\n"));
		return 0;
	}

	if (argc == 3) {
		data.game->rows = _ttoi(argv[1]);
		data.game->carSpeed = _ttoi(argv[2]);

		_tprintf(_T("Lanes = %d\n"), data.game->rows);

		_tprintf(TEXT("Speed of cars = %d"), data.game->carSpeed);
	}
	else {
		if (!readRegConfigs(&data, &reg)) {
			_tprintf(_T("\nError reading configs from registry.\n"));
			return 0;
		}

		_tprintf(_T("\nValues from reg are set.\n"));

		_tprintf(_T("\nLanes = %d\n"), data.game->rows);

		_tprintf(TEXT("\nSpeed for cars = %d\n"), data.game->carSpeed);
	}

	hReceiveCmdThread = CreateThread(NULL, 0, receiveCmdFromOperator, &data, 0, NULL);
	if (hReceiveCmdThread == NULL) {
		_tprintf(_T("\nCan't create RECEIVECMDTHREAD [%d]"), GetLastError());
		return 0;
	}

	hSendGameDataThread = CreateThread(NULL, 0, sendGameData, &data, 0, NULL);
	if (hSendGameDataThread == NULL) {
		_tprintf(_T("\nCan't create SENDGAMEDATATHREAD [%d]"), GetLastError());
		return -1;
	}

	WaitForSingleObject(hReceiveCmdThread, INFINITE);
	WaitForSingleObject(hSendGameDataThread, INFINITE);
	RegCloseKey(reg.key);
	CloseHandle(hReceiveCmdThread);
	
	return 0;
}