#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <time.h>
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

void initBoard(pData data) {

	for (DWORD i = 0; i < data->game[0].rows; i++) {
		for (DWORD j = 0; j < data->game[0].columns; j++) {
			data->game[0].board[i][j] = _T('-'); // ver melhor
			//_tcscpy_s(&data->game[0].board[i][j], sizeof(TCHAR), _T("-")); // perguntar ao prof
		}
	}
}

DWORD insertFrog(pData data) { // from shared memory give to operator
	DWORD aux;

	for (DWORD i = 0; i < data->game[0].rows; i++) { //mudar depois
		for (DWORD j = 0; j < data->game[0].columns - 1; j++) {
			//data->game[0].frogs->symbol = _T('s');

			if (data->game[0].board[i][j] == _T('-')) {
				data->game[0].board[2][data->game[0].columns - 2] = _T('s');
				data->game[0].nFrogs++;
			}
			
			/*if (data->game[0].nFrogs == 0 && (i == data->game->rows - 1)) {
				while (data->game[0].nFrogs < 2) {
					aux = rand() % data->game->columns;
					data->game[0].frogs->symbol = _T('s');
					data->game[0].board[i][aux] = data->game[0].frogs->symbol;
					data->game[0].frogs->y = i;
					data->game[0].frogs->x = aux;
					data->game[0].nFrogs++;
				}
			}*/
		}
	}
}

void insertCars(pData data) {
	DWORD count = 0;
	
	data->game[0].nCars = rand() % (data->game[0].rows - 2) * 8;
	_tprintf(TEXT("\nNCARS: %d"), data->game[0].nCars);

	if (data->game[0].nCars == 0) {
		_tprintf(TEXT("\nImprimiu 0 carros :)\n"));
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

			if (carsInLine < 8 && i != 0 && i != (data->game[0].rows - 1) && j != data->game[0].columns - 1 && j != 0 && data->game[0].board[i][j] != _T('s')) { // verifica se ainda há espaço para mais carros na linha
				data->game[0].board[i][j] = _T('c');
				count++;
			}
		}
	}
}


DWORD stopCars(pData data, INT time) {
	if (data->game[0].isMoving == FALSE) {
		_tprintf(_T("\nCars are already stopped. Wait for a few seconds and try again.\n"));
		return -2;
	}

	if (time < 0) {
		_tprintf(_T("\nInsert a valid timeframe\n"));
		return -1;
	}

	_tprintf(_T("\nStopping cars for %d seconds.\n"), time);

	if (data->game[0].nCars == 0) {
		_tprintf(_T("\nNo cars to stop.\n"));
		return -2;
	}

	while (time > 0) {
		data->game->isMoving = FALSE;
		time--;
		Sleep(1000);
	}

	//when time runs out
	_tprintf(_T("\nTimer reached 0 seconds, back to moving cars.\n"));

	data->game[0].isMoving = TRUE;

	return 1;
}

DWORD insertObstacle(pData data, INT row, INT column) {

	TCHAR obstacle = _T('O'); // ver melhor

	if (row == 0 || row == data->game[0].rows-1) {
		_tprintf(_T("\nCan't insert obstacle in starting/finish line.\n"));
		return -2;
	}

	if (column > data->game[0].columns - 1 || row > data->game[0].rows - 1 || column < 0 || row < 0) {
		_tprintf(_T("\nTrying to insert obstacle out of bounds.\n"));
		return -3;
	}

	if (data->game->board[row][column] != _T('-')) { // something is there
		_tprintf(_T("\nThere is an element at position (%d,%d)"), row, column);
		
		return -1;
	}

	_tprintf(_T("\nInserting obstacle at position X: %d and Y: %d"), row, column);
	
	data->game[0].board[row][column] = obstacle; // insert obstacle in 
	
	return 1;
}


BOOL moveCars(pData data) {
	/*if (data->game[0].isMoving == TRUE) {
		if (data->game[0].direction == TRUE) { // RIGHT - LEFT
			for (DWORD i = 0; i < data->game[0].rows - 1; i++) {
				for (DWORD j = 0; j < data->game[0].columns - 1; j++) {
					if (data->game[0].board[i][j] == _T('c')) { // ver obstaculos depois
						TCHAR prevElement = data->game[0].board[i][j + 1]; // store prev element before moving to the first spot of row
						data->game[0].board[i][0] = data->game[0].board[i][j]; // give the last element of row to the first
						data->game[0].board[i][j] = prevElement; // give last element the prev element before switching
					}
				}
			}
		}
	}
	else {
		return FALSE;
	}*/
	
	if (data->game[0].isMoving == TRUE) {
		if (data->game[0].direction == FALSE) { // move from RIGHT - LEFT
			for (DWORD i = 1; i < data->game[0].rows - 1; i++) { // iterate rows
				for (DWORD j = 0; j < data->game[0].columns - 1; j++) { // iterate columns
					if (j == 0 && data->game[0].board[i][j] == _T('c')) { // if we are at last col and has car
						TCHAR prevElement = data->game[0].board[i][j + 1]; // store prev element before moving to the first spot of row
						data->game[0].board[i][0] = data->game[0].board[i][j]; // give the last element of row to the first
						data->game[0].board[i][j] = prevElement; // give last element the prev element before switching
					}

					else if (j == data->game[0].columns - 1 && data->game[0].board[i][j] == _T('c')) { // if we are at first element of row and its a car
						data->game[0].board[i][j + 1] = data->game[0].board[i][j]; // give next element its own value (move 'c' to right)
					}

					else if (data->game[0].board[i][j] == _T('c') && j != 0 && j != data->game[0].columns - 1) {
						TCHAR prevElement = data->game[0].board[i][j + 1]; // otherwise move normally 
						data->game[0].board[i][j - 1] = data->game[0].board[i][j];
						data->game[0].board[i][j] = prevElement;
					}
				}
			}
		}
		else { // if the way they are moving is LEFT RIGHT (ou seja, TRUE)
			BOOL firstTime = FALSE;
			for (DWORD i = 1; i < data->game[0].rows - 1; i++) { // iterate rows
				for (DWORD j = data->game[0].columns; j > 0; j--) { // iterate columns
					if (data->game[0].board[i][1] == _T('c')) {
						if (!firstTime) {
							data->game[0].board[i][0] = data->game[0].board[i][1];
							data->game[0].board[i][0] = _T('-');
							firstTime = TRUE;
						}
					}
					if (j == data->game[0].columns - 1 && data->game[0].board[i][j] == _T('c')) { // if we are at last col and has car
						TCHAR prevElement = data->game[0].board[i][j - 1]; // store prev element before moving to the first spot of row
						data->game[0].board[i][j] = prevElement; // give last element the prev element before switching
					}

					/*else if (j == 0 && data->game[0].board[i][j] == _T('c')) { // if we are at first element of row and its a car
						data->game[0].board[i][j + 1] = data->game[0].board[i][j];
						data->game[0].board[i][j] = _T('-');// give next element its own value (move 'c' to right)
						_tprintf(_T("\nCarro na faixa: %d"), i);
					}*/

					if (data->game[0].board[i][j] == _T('c') && j != 0 && j != data->game[0].columns - 1) {
						TCHAR prevElement = data->game[0].board[i][j - 1]; // otherwise move normally 
						data->game[0].board[i][j + 1] = data->game[0].board[i][j];
						data->game[0].board[i][j] = prevElement;
					}
				}
			}
		}
	}
	else {
		return FALSE;
	} 
}


DWORD changeDirection(pData data) {
	if (data->game[0].nCars == 0) {
		_tprintf(_T("\nNo cars to change direction!\n"));
		return -1;
	}

	_tprintf(_T("\nChanging direction of %d cars.\n"), data->game[0].nCars);

	/*switch (data->game[0].direction) {
		case TRUE: // andar para a direita
			data->game[0].direction = FALSE; // andar para a esquerda
			_tprintf(_T("\nDirections changed from LEFT - RIGHT to RIGHT - LEFT\n"));
		case FALSE:
			data->game[0].direction = TRUE; // andar para a direita
			_tprintf(_T("\nDirections changed from RIGHT - LEFT to LEFT - RIGHT\n"));
		default:
			return -2;
	}*/

	if (data->game[0].direction == FALSE) {
		data->game[0].direction = TRUE; // andar para a direita
		_tprintf(_T("\nDirections changed from RIGHT - LEFT to LEFT - RIGHT\n"));
		return 1;
	}

	data->game[0].direction = FALSE;
	_tprintf(_T("\nDirections changed from LEFT - RIGHT to RIGHT - LEFT\n"));
	return 1;
}

DWORD WINAPI decreaseTime(LPVOID params) {
	pData data = (pData)params;

	while (!data->game[0].isShutdown && !data->game[0].isSuspended) {
		if (!data->game->isSuspended && data->time > 0) {
			data->time--; // depois ver como fazer com a velocidade dos carros
			moveCars(data);
			Sleep(4000);
			_tprintf(_T("\n[%d] seconds remaining!\n"), data->time);
		}
		else if (data->time == 0) {
			// aqui voltar a meter os sapos na startLine
			_tprintf(_T("\nPerdeu! O tempo expirou!\n"));
			data->game[0].isShutdown = TRUE;
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
		if (data->game[0].isSuspended == FALSE) {
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
	} while (data->game[0].isShutdown != TRUE);

	return 0;
}

DWORD WINAPI sendGameData(LPVOID params) {
	pData data = (pData)params;

	while (!data->game[0].isShutdown) {
		WaitForSingleObject(data->hWriteSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);

		// check for multiplayer na prox meta, para já copiar apenas os dados

		data->game[0].time = data->time;

		CopyMemory(&data->sharedMemGame->game[0], &data->game[0], sizeof(Game)); // copia para dentro da shmMem para depois o op ler

		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hWriteSem, 1, NULL); // aqui é o sem de escrita porque vai escrever para dentro do pointer da shmMem
	}
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

				DWORD readLanes = RegQueryValueEx(reg->key, _T("Lanes"), NULL, NULL, (LPBYTE)&data->game[0].rows, &size);
				DWORD readSpeed = RegQueryValueEx(reg->key, _T("Speed"), NULL, NULL, (LPBYTE)&data->game[0].carSpeed, &size);

				if (readLanes != ERROR_SUCCESS || readSpeed != ERROR_SUCCESS) {
					_tprintf(_T("\n[ERROR] Can't read values from Registry.\n"));
					return FALSE;
				}

				return TRUE;

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

	return TRUE;
}

void startgame(pData data) {
	data->game[0].isShutdown = FALSE;
	//data->game[1].isShutdown = FALSE;
	data->game[0].isSuspended = FALSE;
	//data->game[1].isSuspended = FALSE;
	data->time = 100;
	data->game[0].isMoving = TRUE;
	data->game[0].columns = (DWORD)20;
	data->game[0].direction = TRUE;
	//data->game[1].columns = (DWORD)20;
	data->game[0].nFrogs = 0;
	data->game[0].frogs->nLives = 3;

	initBoard(data);
	insertFrog(data);
	insertCars(data);

}

BOOLEAN checkFrogCollisionTop(pData data) {
	for (DWORD i = 1; i < data->game[0].rows - 1; i++) {
		for (DWORD j = 0; j < data->game[0].columns; j++) {
			if (data->game[0].board[i][j] == _T('s')) {
				if (data->game[0].board[i-1][j] == _T('c') || data->game[0].board[i-1][j] == _T('O')) {
					_tprintf(_T("\nMatei o sapo @ [%d, %d]"), i, j);
					data->game[0].board[i][j] = _T('M'); // Marcar a posição onde o sapo morreu
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOLEAN checkFrogCollisionSide(pData data) {
	//check for how it's moving (LEFT _ RIGHT / RIGHT _ LEFT)
}

DWORD WINAPI threadFroggerSinglePlayer(LPVOID params) {

	pData data = (pData)params;
	BOOL win = FALSE;
	BOOL end = FALSE;
	//BOOL begin = FALSE;

	while (!end) { // enquanto o jogo não acabou
		if (!data->game[0].isSuspended) { // e não está suspenso/ver do tempo
			if (data->game[0].frogs->y == 0) {
				data->game[0].frogs->score += 100; // quando chega à meta ganha 100 pontos
				win = TRUE;
				end = TRUE;
				Sleep(2000);
				continue;
			}

			if (data->game[0].nFrogs == 0) {
				_tprintf(_T("\n[NAO HA SAPOS].\n"));
				end = TRUE;
				continue;
			}

			if (checkFrogCollisionTop(data)) {
				if (data->game[0].frogs->nLives != 0) {
					_tprintf(_T("\nVidas: %d"), data->game[0].frogs->nLives);
					data->game[0].frogs->nLives--; // lose 1 HP 
					_tprintf(_T("\nVidas: %d"), data->game[0].frogs->nLives);
					data->game[0].frogs->y = data->game[0].board[data->game[0].rows - 1][10];
					Sleep(2000);
					continue;
				}
				else {
					end = TRUE;
					continue;
				}
			}
			else {
				continue;
			}

			if (data->game[0].time == 0 && data->game[0].frogs->y != 0) { // se o tempo for 0 e não chegou à meta então
				if (data->game[0].frogs->nLives != 0) {
					data->game[0].frogs->nLives--;
					data->game[0].frogs->y = data->game[0].board[data->game[0].rows - 1][10]; // se ele perder, volta para a partida numa coluna do meio
					Sleep(2000);
					continue;
					// e não end = TRUE;
				}
				end = TRUE;
				continue;
			}
		}
	}
}


DWORD WINAPI threadFroggerMultiPlayer(LPVOID params) {
	pData data = (pData)params;


}

int _tmain(int argc, TCHAR** argv) {
	// aqui são as vars
	HANDLE hReceiveCmdThread;
	HANDLE hSendGameDataThread;
	HANDLE hDecreaseTimerThread;
	HANDLE hSinglePlayerThread;
	//HANDLE hMultiPlayerThread;
	Game game[2] = { 0 };
	RegConfig reg = {0};
	Data data;
	data.game[0] = game[0];
	data.game[1] = game[1];
	data.game->frogs = malloc(sizeof(Frog));
	srand(time(NULL));

#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif
	
	if (data.game->frogs == NULL) {
		_tprintf(_T("\nCouldn't allocate space for frogs.\n"));
		return 0;
	}

	if (!createSharedMemoryAndInit(&data)) {
		_tprintf(_T("\nCan't create shared memory.\n"));
		return 0;
	}

	if (!readRegConfigs(&data, &reg, argc, argv)) {
		_tprintf(_T("\nError reading configs from registry.\n"));
		return 0;
	}

	_tprintf(_T("\nValues set.\n"));

	_tprintf(_T("\nLanes = %d\n"), data.game->rows);

	_tprintf(TEXT("\nSpeed for cars = %d\n"), data.game->carSpeed);

	/*if (argc == 3) {
		data.game->rows = _ttoi(argv[1]);
		data.game->carSpeed = _ttoi(argv[2]);

		_tprintf(_T("Lanes = %d\n"), data.game->rows);

		_tprintf(TEXT("Speed of cars = %d"), data.game->carSpeed);
	}
	else {
		if (!readRegConfigs(&data, &reg, argc, argv)) {
			_tprintf(_T("\nError reading configs from registry.\n"));
			return 0;
		}

		_tprintf(_T("\nValues from reg are set.\n"));

		_tprintf(_T("\nLanes = %d\n"), data.game->rows);

		_tprintf(TEXT("\nSpeed for cars = %d\n"), data.game->carSpeed);
	}*/

	startgame(&data);

	hDecreaseTimerThread = CreateThread(NULL, 0, decreaseTime, &data, 0, NULL);
	if (hDecreaseTimerThread == NULL) {
		_tprintf(_T("\nCan't create DECREASETIMERTHREAD [%d]"), GetLastError());
		return -10;
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

	hSinglePlayerThread = CreateThread(NULL, 0, threadFroggerSinglePlayer, &data, 0, NULL);

	if (hSinglePlayerThread == NULL) {
		_tprintf(_T("\nCan't create SINGLEPLAYERTHREAD [%d]"), GetLastError());
		return -2;
	}

	/*if (data.game[0].gameType == 2) { // if it's multiplayer
		_tprintf(_T("\nCan't create MULTIPLAYERTHREAD.\n [%d]"), GetLastError());
		return -3;
	}*/

	WaitForSingleObject(hReceiveCmdThread, INFINITE);
	WaitForSingleObject(hSendGameDataThread, INFINITE);
	WaitForSingleObject(hDecreaseTimerThread, INFINITE);
	WaitForSingleObject(hSinglePlayerThread, INFINITE);
	//WaitForSingleObject(hMultiPlayerThread, INFINITE);
	RegCloseKey(reg.key);
	CloseHandle(hReceiveCmdThread);
	CloseHandle(hDecreaseTimerThread);
	CloseHandle(hSendGameDataThread);
	CloseHandle(hSinglePlayerThread);
	//CloseHandle(hMultiPlayerThread);
	
	return 0;
}

