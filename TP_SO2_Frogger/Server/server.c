#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <time.h>
#include "structs.h"
#include "../DLL/FroggerDLL.h"

extern __declspec(dllimport) BOOL createSharedMemoryAndInitServer(pData data);


void showBoard(pData data) {
	WaitForSingleObject(data->hMutex, INFINITE);
	if (data->game[0].gameType == 1) {
		_tprintf(TEXT("\n\nTime: [%d]\n\n"), data->game[0].time);
		for (DWORD i = 0; i < data->game[0].rows; i++)
		{
			_tprintf(TEXT("\n"));
			for (DWORD j = 0; j < data->game[0].columns; j++)
				_tprintf(TEXT("%c "), data->game[0].board[i][j]);
		}
		_tprintf(TEXT("\n\n"));
	}
	else {
		for (DWORD i = 0; i < 2; i++) {
			for (DWORD j = 0; j < data->game[i].rows; j++)
			{
				_tprintf(TEXT("\n"));

				for (DWORD k = 0; k < data->game[i].columns; k++)

					_tprintf(TEXT("%c "), data->game[i].board[j][k]);
			}
			_tprintf(TEXT("\n\n"));
		}
	}
	ReleaseMutex(data->hMutex);
}

BOOL createSharedMemoryAndInitServer(pData p) {
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

void initBoard(pData data) {

	for (DWORD i = 0; i < data->game[0].rows; i++) {
		for (DWORD j = 0; j < data->game[0].columns; j++) {
			if (data->game[0].gameType == 1) {
				data->game[0].board[i][j] = _T('-');
			}
			else {
				for (DWORD k = 0; k < 2; k++) {
					data->game[k].board[i][j] = _T('-');
				}
			}
		}
	}
}

DWORD insertFrog(pData data) { // from shared memory give to operator SO METE UM FROG
	if (data->game->gameType == 1) {
		data->game->board[data->game->player1.y][data->game->player1.x] = _T('s');
		data->game->nFrogs++;
	}
	else {
		data->game->board[data->game->player1.y][data->game->player1.x] = _T('s');
		data->game->board[data->game->player2.y][data->game->player2.x + 2] = _T('s');
		data->game->nFrogs = 2;
	}
	
	return 1;
	// meter dois sapos lado a lado, ver isso
	

	/*for (DWORD i = 0; i < data->game->rows; i++) { //mudar depois
		for (DWORD j = 0; j < data->game->columns - 1; j++) {

			//data->game[0].board[2][10] = _T('s');
			//data->game[0].nFrogs++;
			//data->game[0].frogX = 10;
			//data->game[0].frogY = 2;
			if (data->game[0].nFrogs == 0 && (i == data->game->rows-1)) {
				while (data->game[0].nFrogs <= 1) {
					//aux = rand() % data->game->columns;
					data->game[0].board[i][10] = _T('s');
					data->game[0].player1.y = i;
					data->game[0].player1.x = 10;
					data->game[0].nFrogs++;
					_tprintf(TEXT("\n o sapo ta aqui %d %d\n"), data->game[0].player1.y, data->game[0].player1.x);
					return 1;
				}
			}
		}
	}*/
}

void insertCars(pData data) {
	
	DWORD count = 0;
	
	data->game[0].nCars = rand() % (data->game[0].rows - 2) * 8;
	
	_tprintf(TEXT("\nNCARS: %d"), data->game[0].nCars);

	if (data->game[0].nCars == 0) {
		_tprintf(TEXT("\nImprimiu 0 carros.\n"));
		return;
	}

	while (data->game[0].nCars != count) { // insere carros até o número desejado ser alcançado
		DWORD i = rand() % (data->game[0].rows - 2) + 1; // escolhe uma linha aleatória (exceto a primeira e a última)
		DWORD j = rand() % data->game[0].columns; // escolhe uma coluna aleatória

		if (data->game[0].board[i][j] != _T('c') && data->game[0].board[i][j] != _T('s')) { // verifica se a posição está livre
			
			DWORD carsInLine = 0;

			for (DWORD k = 0; k < data->game[0].columns; k++) { // conta o número de carros na linha atual
				if (data->game[0].board[i][k] == _T('c')) {
					carsInLine++;
				}
			}

			if (carsInLine < 8 && j != 0 && data->game[0].board[i][j] != _T('s')) { // verifica se ainda há espaço para mais carros na linha
				data->game[0].board[i][j] = _T('c');
				count++;
			}
		}
	}
}

DWORD stopCars(pData data, INT time) {
	
	if (data->game->isMoving == FALSE) {
		_tprintf(_T("\nCars are already stopped. Wait for a few seconds and try again.\n"));
		return -2;
	}

	if (time < 0) {
		_tprintf(_T("\nInsert a valid timeframe\n"));
		return -1;
	}

	_tprintf(_T("\nStopping cars for %d seconds.\n"), time);

	if (data->game->nCars == 0) {
		_tprintf(_T("\nNo cars to stop.\n"));
		return -2;
	}

	while (time > 0) {
		if (data->game->gameType == 1) {
			data->game[0].isMoving = FALSE;
			time--;
			Sleep(1000);
		}
		else {
			for (DWORD i = 0; i < 2; i++){
				data->game[i].isMoving = FALSE;
				time--;
				Sleep(1000);
			}
		}
	}

	//when time runs out
	_tprintf(_T("\nTimer reached 0 seconds, back to moving cars.\n"));

	data->game[0].isMoving = TRUE;

	return 1;
}

DWORD insertObstacle(pData data, INT row, INT column) {

	TCHAR obstacle = _T('O'); // ver melhor

	if (row == 0 || row == data->game->rows-1) {
		_tprintf(_T("\nCan't insert obstacle in starting/finish line.\n"));
		return -2;
	}

	if (column > data->game->columns - 1 || row > data->game->rows - 1 || column < 0 || row < 0) {
		_tprintf(_T("\nTrying to insert obstacle out of bounds.\n"));
		return -3;
	}

	if (data->game->board[row][column] != _T('-')) { // something is there
		_tprintf(_T("\nThere is an element at position (%d,%d)"), row, column);
		
		return -1;
	}

	_tprintf(_T("\nInserting obstacle at position X: %d and Y: %d"), row, column);

	WaitForSingleObject(data->hMutex, INFINITE);

	if (data->game->gameType == 1) {
		data->game[0].board[row][column] = obstacle; // insert obstacle in
	}
	
	else {
		for (DWORD i = 0; i < 2; i++) {
			data->game[i].board[row][column] = obstacle;
		}
	}

	ReleaseMutex(data->hMutex);
	
	return 1;
}

BOOLEAN checkFrogSide(pData data) {
	//_tprintf(_T("\n coordenadas %d %d esta o simb: %c"), data->game[0].player1.y - 1, data->game[0].player1.x, data->game[0].board[data->game[0].player1.y - 1][data->game[0].player1.x]);
		
	return data->game[0].board[data->game[0].player1.y][data->game[0].player1.x] == _T('c') || data->game[0].board[data->game[0].player1.y][data->game[0].player1.x] == _T('O');
}

BOOL moveFrog(pData data) {
	

}

BOOL moveCars(pData data) {
	if (data->game[0].isMoving == TRUE) {
		if (data->game[0].direction == FALSE) { // move from RIGHT - LEFT
			for (DWORD i = 1; i < data->game[0].rows - 1; i++) { // iterate rows
				for (DWORD j = 0; j < data->game[0].columns - 1; j++) { // iterate columns
					if (j == 0 && data->game[0].board[i][j] == _T('c')) { // if we are at last col and has car
						TCHAR prevElement = data->game[0].board[i][j + 1]; // store prev element before moving to the first spot of row
						data->game[0].board[i][j] = prevElement; // give last element the prev element before switching
						if (rand() % 10 < 9) {
							DWORD randomRow = 1 + (rand() % (data->game[0].rows - 2));
							data->game[0].board[randomRow][data->game[0].columns - 1] = _T('c');
							continue;
						}

					}
					if (data->game[0].board[i][j] == _T('c') && j != 0 && j != data->game[0].columns - 1) {
						TCHAR prevElement = data->game[0].board[i][j + 1];
						if (data->game[0].board[i][j - 1] == _T('O')) {
							data->game[0].board[i][j - 1] = _T('O');
							data->game[0].board[i][j] = prevElement;
							continue;
						}
						if (prevElement == _T('s')) {

							data->game[0].board[i][j - 1] = data->game[0].board[i][j];
							data->game[0].board[i][j] = _T('-'); // depois mudar isto PORQUE nÂO SABEMOS PARA ONDE O SAPO VAI SE È PARA CIMA BAIXO ESQ OU IDIRETA
							 
						}
						else if (prevElement == _T('O')) {
							data->game[0].board[i][j - 1] = data->game[0].board[i][j];
							data->game[0].board[i][j] = _T('-');
							 
						}
						else
						{
							data->game[0].board[i][j - 1] = data->game[0].board[i][j];
							data->game[0].board[i][j] = prevElement;
							 
						}
					}
					if (data->game[0].board[i][data->game[0].columns - 1] == _T('c')) {
						data->game[0].board[i][data->game[0].columns - 1] = _T('-');
						data->game[0].board[i][data->game[0].columns - 2] = _T('c'); //fix
						 
					}

				}
			}
		}
		else { // if the way they are moving is LEFT RIGHT (ou seja, TRUE)
			for (DWORD i = 1; i < data->game[0].rows - 1; i++) { // iterate rows
				for (DWORD j = data->game[0].columns - 1; j > 0; j--) { // iterate columns
					if (j == data->game[0].columns - 1 && data->game[0].board[i][j] == _T('c')) {
						TCHAR prevElement = data->game[0].board[i][j - 1]; // store prev element before moving to the first spot of row
						data->game[0].board[i][j] = prevElement; // give last element the prev element before switching
						if (rand() % 10 < 9) {
							DWORD randomRow = 1 + (rand() % (data->game[0].rows - 2));
							data->game[0].board[randomRow][0] = _T('c');
							continue;
						}
					}
					if (data->game[0].board[i][j] == _T('c') && j != 0 && j != data->game[0].columns - 1) {
						TCHAR prevElement = data->game[0].board[i][j - 1];
						if (data->game[0].board[i][j + 1] == _T('O')) {
							//_tprintf(_T("\nEntrei aqui oaodasopidawoidj\n"));
							data->game[0].board[i][j + 1] = _T('O');
							data->game[0].board[i][j] = prevElement;
							continue;
						}

						if (prevElement == _T('s')) {
							data->game[0].board[i][j + 1] = data->game[0].board[i][j];
							data->game[0].board[i][j] = _T('-'); // depois mudar isto PORQUE nÂO SABEMOS PARA ONDE O SAPO VAI SE È PARA CIMA BAIXO ESQ OU IDIRETA
							 
						}
						else if (prevElement == _T('O')) {
							data->game[0].board[i][j + 1] = data->game[0].board[i][j];
							data->game[0].board[i][j] = _T('-');
							 
						}
						else
						{
							data->game[0].board[i][j + 1] = data->game[0].board[i][j];
							data->game[0].board[i][j] = prevElement;
							 
						}
					}
					if (data->game[0].board[i][0] == _T('c')) {
						data->game[0].board[i][0] = _T('-');
						data->game[0].board[i][1] = _T('c'); //fix
						 
					}
				}
			}
		}
	}
	else {
		return FALSE;
	}
	return TRUE;
}

DWORD changeDirection(pData data) {
	if (data->game->nCars == 0) {
		_tprintf(_T("\nNo cars to change direction!\n"));
		return -1;
	}

	_tprintf(_T("\nChanging direction of %d cars.\n"), data->game->nCars);

	if (data->game->direction == FALSE) {
		if (data->game->gameType == 1) {
			data->game[0].direction = TRUE; // andar para a direita
		}
		else {
			for (DWORD i = 0; i < 2; i++) {
				data->game[i].direction = TRUE;
			}
		}
		
		_tprintf(_T("\nDirections changed from RIGHT - LEFT to LEFT - RIGHT\n"));
		return 1;
	}

	if (data->game->gameType == 1) {
		data->game[0].direction = FALSE; // andar para a direita
	}
	else {
		for (DWORD i = 0; i < 2; i++) {
			data->game[i].direction = FALSE;
		}
	}
	_tprintf(_T("\nDirections changed from LEFT - RIGHT to RIGHT - LEFT\n"));
	return 1;
}

DWORD WINAPI decreaseTime(LPVOID params) {
	pData data = (pData)params;

	while (!data->game->isShutdown && !data->game->isSuspended) {
		 
		if (!data->game->isSuspended && data->time > 0) {
			data->time--; // depois ver como fazer com a velocidade dos carros
			//showBoard(data);
			Sleep(1000);
		}
		else if (data->time == 0) {
			data->game->isShutdown = TRUE;
			return 1;
		}

		 
	}
	return 0;
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
				/*if (WaitForSingleObject(data->hCmdEvent, INFINITE) == WAIT_OBJECT_0) {
					_tprintf(_T("\nCars are already stopped. Please wait for %d seconds.\n"), command.parameter - (data->time - data->game[0].time));
				}*/
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

	while (!data->game->isShutdown) {
		WaitForSingleObject(data->hWriteSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);

		if (data->game->gameType == 1) {
			data->game[0].time = data->time;
			CopyMemory(&data->sharedMemGame->game[0], &data->game[0], sizeof(Game)); // copia para dentro da shmMem para depois o op ler
		}

		else {
			for (DWORD i = 0; i < 2; i++) {
				data->game[i].time = data->time;
				CopyMemory(&data->sharedMemGame->game[i], &data->game[i], sizeof(Game));
			}
		}
		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hWriteSem, 1, NULL); // aqui é o sem de escrita porque vai escrever para dentro do pointer da shmMem
	}

	return 1;
}

BOOL readRegConfigs(pData data, pRegConfig reg, INT argc, TCHAR** argv) {
	//check if it can open reg key

	DWORD size = SIZE_DWORD;

	_tcscpy_s(reg->keyPath, BUFFER, _T("SOFTWARE\\TP_SO2"));

	if (RegOpenKeyEx(HKEY_CURRENT_USER, reg->keyPath, 0, KEY_ALL_ACCESS, &reg->key) != ERROR_SUCCESS) {
		_tprintf(_T("\nKey doesn't exist. Creating...\n"));

		if (RegCreateKeyEx(HKEY_CURRENT_USER, reg->keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &reg->key, &reg->dposition) == ERROR_SUCCESS) {
			_tprintf(_T("\nKey created successfully.\n"));
			// atribuir os valores para dentro da struct

			if (argc == 3) // cria a chave do reg com os valores da linha de comandos. 
			{
				DWORD lanes = _ttoi(argv[1]);
				DWORD speed = _ttoi(argv[2]);

				DWORD setLanes = RegSetValueEx(reg->key, _T("Lanes"), 0, REG_DWORD, (LPBYTE)&lanes, sizeof(DWORD), &size);
				DWORD setSpeed = RegSetValueEx(reg->key, _T("Speed"), 0, REG_DWORD, (LPBYTE)&speed, sizeof(DWORD), &size);

				if (setLanes != ERROR_SUCCESS || setSpeed != ERROR_SUCCESS) {
					_tprintf(_T("\nCan't set values for Frogger.\n"));
					return FALSE;
				}

				if (data->game[0].gameType == 1) {
					DWORD readLanes = RegQueryValueEx(reg->key, _T("Lanes"), NULL, NULL, (LPBYTE)&data->game[0].rows, &size);
					DWORD readSpeed = RegQueryValueEx(reg->key, _T("Speed"), NULL, NULL, (LPBYTE)&data->game[0].carSpeed, &size);

					if (readLanes != ERROR_SUCCESS || readSpeed != ERROR_SUCCESS) {
						_tprintf(_T("\n[ERROR] Can't read values from Registry.\n"));
						return FALSE;
					}
					return TRUE;

				}

				else {
					for (DWORD i = 0; i < 2; i++) {
						DWORD readLanes = RegQueryValueEx(reg->key, _T("Lanes"), NULL, NULL, (LPBYTE)&data->game[i].rows, &size);
						DWORD readSpeed = RegQueryValueEx(reg->key, _T("Speed"), NULL, NULL, (LPBYTE)&data->game[i].carSpeed, &size);

						if (readLanes != ERROR_SUCCESS || readSpeed != ERROR_SUCCESS) {
							_tprintf(_T("\n[ERROR] Can't read values from Registry.\n"));
							return FALSE;
						}
						return TRUE;
					}
				}
			}

			// ele só entra aqui se os args forem 0, ou seja,
			// se não houver args de linha de comand
			// se ele não vir nenhum, ele vai ler do reg e, se lá estiver dentro alguma coisa
			// então ele mete dentro das vars, senão ele vai meter os valores hardcoded

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

	if (argc == 3) { // if key is created and can open

		DWORD lanes = _ttoi(argv[1]);
		DWORD speed = _ttoi(argv[2]);


		DWORD setLanes = RegSetValueEx(reg->key, _T("Lanes"), 0, REG_DWORD, (LPBYTE)&lanes, sizeof(DWORD), &size);
		DWORD setSpeed = RegSetValueEx(reg->key, _T("Speed"), 0, REG_DWORD, (LPBYTE)&speed, sizeof(DWORD), &size);

		if (setLanes != ERROR_SUCCESS || setSpeed != ERROR_SUCCESS) {
			_tprintf(_T("\nCan't set values for Frogger.\n"));
			return FALSE;
		}

		DWORD readLanes = RegQueryValueEx(reg->key, _T("Lanes"), NULL, NULL, (LPBYTE)&data->game[0].rows, &size);
		DWORD readSpeed = RegQueryValueEx(reg->key, _T("Speed"), NULL, NULL, (LPBYTE)&data->game[0].carSpeed, &size);

		if (readLanes != ERROR_SUCCESS || readSpeed != ERROR_SUCCESS) {
			_tprintf(_T("\n[ERROR] Can't read values from Registry.\n"));
			return FALSE;
		}
	}

	DWORD readLanes = RegQueryValueEx(reg->key, _T("Lanes"), NULL, NULL, (LPBYTE)&data->game[0].rows, &size);
	DWORD readSpeed = RegQueryValueEx(reg->key, _T("Speed"), NULL, NULL, (LPBYTE)&data->game[0].carSpeed, &size);

	_tprintf(_T("\nChave já criada com: %d %d"), data->game[0].rows, data->game[0].carSpeed);

	if (readLanes != ERROR_SUCCESS || readSpeed != ERROR_SUCCESS) {
		_tprintf(_T("\n[ERROR] Can't read values from Registry.\n"));
		return FALSE;
	}

	RegCloseKey(reg->key);
	return TRUE;
}

void startgame(pData data) {
	data->game[0].isShutdown = FALSE;
	data->game[1].isShutdown = FALSE;
	data->game[0].isSuspended = FALSE;
	data->game[1].isSuspended = FALSE;
	//data->game[0].player1.hasMoved = FALSE;
	data->time = 100;
	data->game[0].isMoving = TRUE;
	data->game[1].isMoving = TRUE;
	data->game[0].columns = (DWORD)20;
	//data->game[0].gameType = 1;
	data->game[0].direction = TRUE;
	data->game[1].columns = (DWORD)20;
	data->game[0].nFrogs = 0;
	// check for gametype
	data->game->player1.nLives = 3;
	data->game->player1.score = 0;
	if (data->game->gameType == 1) {
		data->game->player1.x = 10;
		data->game->player1.y = data->game->rows - 1;


	}
	else {
		data->game->player1.x = 10;
		data->game->player1.y = data->game->rows - 1;
		data->game->player2.x = 11;
		data->game->player2.y = data->game->rows - 1;
	}



	initBoard(data);
	insertCars(data);
	insertFrog(data);
}


DWORD WINAPI threadFroggerSinglePlayer(LPVOID params) {
	pData data = (pData)params;
	BOOL win = FALSE;
	BOOL end = FALSE;

	while (!end) {
		if (!data->game[0].isSuspended) {
			if (data->game[0].time == 0) {
				if (data->game[0].player1.y == 0) {
					_tprintf(_T("\nChegou à meta!\n"));
					win = TRUE;
					end = TRUE;
					
				}
			}
			else {
				if (checkFrogSide(data)) {
					
					data->game[0].player1.x = 10;
					data->game[0].player1.y = data->game[0].rows - 1; //tens
					//data->game[0].board[data->game[0].player1.y][data->game[0].player1.x] = _T('s');
					data->game[0].player1.nLives--;
					_tprintf(_T("\nMorreu!\n"));
					_tprintf(_T("\nVidas: %d"), data->game[0].player1.nLives);



				}

				if (data->game[0].player1.nLives <= 0) {
					data->game->isMoving = FALSE;

					//end = TRUE;
				}

				

				if (data->game[0].player1.y == 0) {
					data->game[0].player1.x = 10;
					data->game[0].player1.y = data->game[0].rows - 1;
					data->game->isMoving = FALSE;
					_tprintf(_T("\nChegou à meta!\n"));
					data->game[0].player1.score += 100;
					//win = TRUE;
					//end = TRUE;
					
				}

				
				
			}
		}
	
	}


	if (end) {
		_tprintf(_T("\nPerdeu!\n"));
		data->game->isMoving = FALSE;

		Sleep(1000);
		data->game[0].isShutdown = TRUE;
		return 1;
	}

	if (win) {
		_tprintf(_T("\nGanhou!"));
		Sleep(1000);
		data->game[0].isShutdown = TRUE;
		return 1;
	}

}

DWORD WINAPI threadFroggerMultiPlayer(LPVOID params) {
	pData data = (pData)params;
	BOOL win = FALSE;
	BOOL end = FALSE;

	while (!end) {
		if (!data->game[1].isSuspended) {
			if (data->game[1].time == 0) {
				if (data->game[1].player1.y == 0) {
					_tprintf(_T("\nChegou à meta!\n"));
					win = TRUE;
					end = TRUE;

				}
			}
			else {

				if (data->game[1].player1.nLives <= 0) {
					data->game->isMoving = FALSE;

					//end = TRUE;
				}

				if (checkFrogSide(data)) {
					data->game[1].player1.nLives--;
					data->game[1].player1.x = 10;
					data->game[1].player1.y = data->game[1].rows - 1; //tens
					data->game[1].board[data->game[1].player1.y][data->game[1].player1.x] = _T('s');
					_tprintf(_T("\nMorreu!\n"));
					_tprintf(_T("\nVidas: %d"), data->game[1].player1.nLives);



				}

				if (data->game[1].player1.y == 0) {
					data->game[1].player1.x = 10;
					data->game[1].player1.y = data->game[1].rows - 1;
					data->game->isMoving = FALSE;
					_tprintf(_T("\nChegou à meta!\n"));
					data->game[0].player1.score += 100;
					//win = TRUE;
					//end = TRUE;

				}



			}
		}

	}


	if (end) {
		_tprintf(_T("\nPerdeu!\n"));
		data->game->isMoving = FALSE;

		Sleep(1000);
		data->game[1].isShutdown = TRUE;
		return 1;
	}

	if (win) {
		_tprintf(_T("\nGanhou!"));
		Sleep(1000);
		data->game[1].isShutdown = TRUE;
		return 1;
	}

}

DWORD WINAPI pipeReadAndWriteThread(LPVOID params) {
	DWORD n;
	BOOL returnValue;

	pData data = (pData)params;

	 do{
		 
		 for (DWORD i = 0; i < NUM; i++) {
			 WaitForSingleObject(data->threadData->hPipeMutex, INFINITE);

			 if (data->threadData->hPipe[i].active) {
				 if (!WriteFile(data->threadData->hPipe[i].hInstance, &data->game[i], sizeof(Game), &n, NULL)) {
					 _tprintf(_T("\nError writing on frog pipe.\n"));
					 data->game[0].isShutdown = TRUE;
				 }
				 else {
					 data->game[i].board[data->game[i].player1.y][data->game[i].player1.x] = _T('-');

					 returnValue = ReadFile(data->threadData->hPipe[i].hInstance, &data->game[i], sizeof(Game), &n, NULL);
					 if (!returnValue || !n) {
						 _tprintf(_T("\nError reading from pipe!\n"));
						 break;
					 }
					 // é a jogada que vai fazer
					 
					
					 data->game[i].board[data->game[i].player1.y][data->game[i].player1.x] = _T('s');
					 moveCars(data);
				 }
			 }
			 ReleaseMutex(data->threadData->hPipeMutex);

		 }

	 } while (!data->game[0].isShutdown);

	data->threadData->end = 1;

	for (DWORD i = 0; i < NUM; i++) {
		SetEvent(data->threadData->hEvents[i]);
	}

	return 0;
}

int _tmain(int argc, TCHAR** argv) {
	// aqui são as vars
	HANDLE hReceiveCmdThread;
	HANDLE hSendGameDataThread;
	HANDLE hDecreaseTimerThread;
	HANDLE hTempEvent; // para os pipes
	HANDLE hPipe;
	HANDLE hPipeThread;
	HANDLE hSinglePlayerThread;
	HANDLE hMultiPlayerThread;
	
	Game game[2] = { 0 };

	TCHAR opt[256];
	
	RegConfig reg = { 0 };

	DWORD gameType = 0;
	
	ThreadData threadData = { 0 };
	
	Data data = {0};

	DWORD nBytes; // for pipe read/writes

	data.threadData = &threadData;
	
	data.game[0] = game[0];
	
	data.game[1] = game[1];

	data.game->isShutdown = FALSE;

	data.game->isSuspended = FALSE;

	DWORD nClients = 0, i;
	
	srand(time(NULL));

#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

	if (!createSharedMemoryAndInitServer(&data)) {
		_tprintf(_T("\nCan't create shared memory.\n"));
		return 0;
	}

	while (gameType < 1 || gameType > 2) {
		_tprintf(_T("\nWhat type of configuration do you want?\n\n1 - Singleplayer\n2 - Multiplayer\n\nOption: "));
		_fgetts(opt, 256, stdin);
		if (_tcslen(opt) == 2)
			gameType = _ttoi(opt);
	}

	if (gameType == 1)
		data.game[0].gameType = gameType;
	else {
		data.game[0].gameType = gameType;
		data.game[1].gameType = gameType;
	}

	if (!readRegConfigs(&data, &reg, argc, argv)) {
		_tprintf(_T("\nError reading configs from registry.\n"));
		return 0;
	}

	_tprintf(_T("\nValues set.\n"));

	_tprintf(_T("\nLanes = %d\n"), data.game->rows);

	_tprintf(TEXT("\nSpeed for cars = %d\n"), data.game->carSpeed);

	data.threadData->end = 0;

	data.threadData->hPipeMutex = CreateMutex(NULL, FALSE, NULL);

	if (data.threadData->hPipeMutex == NULL) {
		_tprintf(_T("\nError creating PIPE_MUTEX. [%d]"), GetLastError());
		exit(-100);
	}

	for (DWORD i = 0; i < NUM; i++) {
		_tprintf(_T("\nServer creating pipe: '%s'...\n"), PIPE_NAME);

		hTempEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (hTempEvent == NULL) {
			_tprintf(_T("\nError creating pipe event [%d]"), GetLastError());
			exit(-20);
		}

		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, NUM, sizeof(Game), sizeof(Game), 1000, NULL);

		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(_T("\nError creating named pipe. [%d]"), GetLastError());
			exit(-10);
		}

		ZeroMemory(&data.threadData->hPipe[i].overlap, sizeof(data.threadData->hPipe[i].overlap)); // clear memory of overlapped struct
		
		data.threadData->hPipe[i].hInstance = hPipe;
		data.threadData->hPipe[i].overlap.hEvent = hTempEvent;
		data.threadData->hEvents[i] = hTempEvent;
		data.threadData->hPipe[i].active = FALSE;

		if (ConnectNamedPipe(hPipe, &data.threadData->hPipe[i].overlap)) {
			_tprintf(_T("\nError while connecting to the client...\n"));
			exit(-1);
		}
	}

	hPipeThread = CreateThread(NULL, 0, pipeReadAndWriteThread, &data, 0, NULL);
	if (hPipeThread == NULL) {
		_tprintf(_T("\nError creating pipe thread! [%d]"), GetLastError());
		exit(-100);
	}

	startgame(&data);

	_tprintf(_T("\nCriei a thread dos pipes e do move cars!\n"));

	hReceiveCmdThread = CreateThread(NULL, 0, receiveCmdFromOperator, &data, 0, NULL);
	if (hReceiveCmdThread == NULL) {
		_tprintf(_T("\nCan't create RECEIVECMDTHREAD [%d]"), GetLastError());
		return 0;
	}

	hDecreaseTimerThread = CreateThread(NULL, 0, decreaseTime, &data, CREATE_SUSPENDED, NULL);
	if (hDecreaseTimerThread == NULL) {
		_tprintf(_T("\nCan't create DECREASETIMERTHREAD [%d]"), GetLastError());
		return -10;
	}

	hSendGameDataThread = CreateThread(NULL, 0, sendGameData, &data, 0, NULL);
	if (hSendGameDataThread == NULL) {
		_tprintf(_T("\nCan't create SENDGAMEDATATHREAD [%d]"), GetLastError());
		return -1;
	}

	hSinglePlayerThread = CreateThread(NULL, 0, threadFroggerSinglePlayer, &data, 0, NULL);
	if (hSinglePlayerThread == NULL) {
		_tprintf(_T("\nCan't create SINGLEPLAYERTHREAD [%d]"), GetLastError());
		return -2;
	}

	if (data.game[0].gameType == 2) {
		hMultiPlayerThread = CreateThread(NULL, 0, threadFroggerMultiPlayer, &data, 0, NULL);

		if (hMultiPlayerThread == NULL) {
			_tprintf(_T("\nCan't create MULTIPLAYER THREAD [%d]"), GetLastError());
			exit(-190);
		}
	}
	// fazer uma verificação para abrir o pipe um cliente se for gameType 1 ou abrir o pipe para os dois clientes se for gameType 2

	while ((!data.game[0].isShutdown || !data.game[1].isShutdown) && nClients < NUM + 1) {
		DWORD result = WaitForMultipleObjects(NUM, data.threadData->hEvents, FALSE, 1000);
		i = result - WAIT_OBJECT_0;
		if (i >= 0 && i < NUM) {
			_tprintf(_T("\nFrog connected...\n"));
			if (data.game[0].gameType == 1) {
				if (!data.game[0].suspended) {
					ResumeThread(hDecreaseTimerThread);
				}
			}
			else {
				if (!data.game[0].suspended && nClients == 1) {
					ResumeThread(hDecreaseTimerThread);
				}
			}
			if (GetOverlappedResult(data.threadData->hPipe[i].hInstance, &data.threadData->hPipe[i].overlap, &nBytes, FALSE)) {
				ResetEvent(data.threadData->hEvents[i]);
				WaitForSingleObject(data.threadData->hPipeMutex, INFINITE);
				data.threadData->hPipe[i].active = TRUE;
				ReleaseMutex(data.threadData->hPipeMutex);
			}
			nClients++;
		}
	}
	for (i = 0; i < NUM; i++) {
		_tprintf(_T("\nCLIENT DISCONNECTED! Shutting down %d pipe...\n"), i);
		if (!DisconnectNamedPipe(data.threadData->hPipe[i].hInstance)) {
			_tprintf(_T("\nError shutting down the pipe (DisconnectNamedPipe) %d.\n"), GetLastError());
			exit(-1);
		}
		CloseHandle(data.threadData->hPipe[i].hInstance);
	}


	WaitForMultipleObjects(5, hReceiveCmdThread, hDecreaseTimerThread, hSendGameDataThread, hSinglePlayerThread, hPipeThread, TRUE, 2000);


	RegCloseKey(reg.key);
	UnmapViewOfFile(data.sharedMemGame);
	UnmapViewOfFile(data.sharedMemCmd);

	CloseHandle(hReceiveCmdThread);
	CloseHandle(hDecreaseTimerThread);
	CloseHandle(hSendGameDataThread);
	CloseHandle(hSinglePlayerThread);
	CloseHandle(hPipeThread);
	CloseHandle(data.mutexCmd);
	CloseHandle(data.hCmdEvent);
	CloseHandle(data.hFileMapFrogger);
	CloseHandle(data.hFileMapMemory);
	CloseHandle(data.hMutex);
	CloseHandle(data.hWriteSem);
	CloseHandle(data.hReadSem);
	//WaitForSingleObject(hMultiPlayerThread, INFINITE);
	//CloseHandle(hMultiPlayerThread);
	Sleep(2000);
	return 0;
}
