#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "../Server/structs.h"
#include "../Server/defines.h"

BOOL createSharedMemoryAndInit(pData data) {
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nStarting operator configs...\n\n"));

	data->hFileMapFrogger = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_GAME);

	if (data->hFileMapFrogger == NULL) { // Map
		firstProcess = TRUE;
		data->hFileMapFrogger = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShmGame), SHM_NAME_GAME);

		if (data->hFileMapFrogger == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	} //create sharedMem for game

	data->sharedMemGame = (ShmGame*)MapViewOfFile(data->hFileMapFrogger, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ShmGame)); // Shared Memory
	if (data->sharedMemGame == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
		CloseHandle(data->hFileMapFrogger);
		return FALSE;
	} // map to pointer of sharedmemgame (assign space to var)

	data->hFileMapMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_MESSAGE);
	if (data->hFileMapMemory == NULL) {
		data->hFileMapMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShmCommand), SHM_NAME_MESSAGE);

		if (data->hFileMapMemory == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	data->sharedMemCmd = (pShmCommand)MapViewOfFile(data->hFileMapMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ShmCommand));
	if (data->sharedMemCmd == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d).\n"), GetLastError());
		UnmapViewOfFile(data->sharedMemCmd);
		CloseHandle(data->hFileMapMemory);
		CloseHandle(data->hFileMapMemory);
		return FALSE;
	}

	data->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (data->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d).\n"), GetLastError());
		UnmapViewOfFile(data->sharedMemGame);
		CloseHandle(data->hFileMapFrogger);
		CloseHandle(data->hFileMapMemory);
		UnmapViewOfFile(data->sharedMemCmd);
		return FALSE;
	}

	data->hWriteSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_WRITE_NAME);
	if (data->hWriteSem == NULL) {
		_tprintf(TEXT("\nError creating writting semaphore: (%d).\n"), GetLastError());
		UnmapViewOfFile(data->sharedMemGame);
		CloseHandle(data->hFileMapFrogger);
		CloseHandle(data->hMutex);
		CloseHandle(data->hFileMapMemory);
		UnmapViewOfFile(data->sharedMemCmd);
		return FALSE;
	}

	data->hReadSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_READ_NAME);
	if (data->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d).\n"), GetLastError());
		UnmapViewOfFile(data->sharedMemGame);
		CloseHandle(data->hFileMapFrogger);
		CloseHandle(data->hMutex);
		CloseHandle(data->hWriteSem);
		CloseHandle(data->hFileMapMemory);
		UnmapViewOfFile(data->sharedMemCmd);
		return FALSE;
	}

	data->hCmdEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME);
	if (data->hCmdEvent == NULL) {
		_tprintf(TEXT("\nError creating command event (%d).\n"), GetLastError());
		UnmapViewOfFile(data->sharedMemGame);
		CloseHandle(data->hFileMapFrogger);
		CloseHandle(data->hMutex);
		CloseHandle(data->hWriteSem);
		CloseHandle(data->hReadSem);
		CloseHandle(data->hFileMapMemory);
		UnmapViewOfFile(data->sharedMemCmd);
		return FALSE;
	}

	_tprintf(_T("\nEverything created successfully.\n"));
	return TRUE;
}

DWORD WINAPI sendCmdThread(LPVOID params) {
	pData data = (pData)params;

	TCHAR* token = NULL;

	TCHAR* next = NULL;

	TCHAR opt[BUFFER] = _T("");

	Command cmd;

	cmd.cmd = 0;

	cmd.parameter = 0;

	cmd.parameter1 = 0;

	DWORD i = 0;

	do {
		_getts_s(opt, _countof(opt)); // fetches user choice with limit of strlen(opt)

		token = _tcstok_s(opt, TEXT(" "), &next);

		if (_tcscmp(token, _T("stopcars")) == 0) {
			WaitForSingleObject(data->mutexCmd, INFINITE);

				//_tprintf(_T("%s"), token);

			if (i == BUFFERSIZE) {
				i = 0;
			}

			cmd.cmd = 1; // call to first command

			CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command));

			token = _tcstok_s(NULL, TEXT(" "), &next);
			if (token != NULL)
				cmd.parameter = _ttoi(token); // get value from token
			if (cmd.parameter != 0 && token != NULL) // if it read correctly
				CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command)); // send to server

			i++; // move to next cmd

			ReleaseMutex(data->mutexCmd); // unlock access
			SetEvent(data->hCmdEvent); // flag event as set
			ResetEvent(data->hCmdEvent); // unflag
			
		}

		else if (_tcscmp(token, _T("insert")) == 0) {
			WaitForSingleObject(data->mutexCmd, INFINITE);
			if (i == BUFFERSIZE) {
				i = 0;
			}

			cmd.cmd = 2; // call to second command

			token = _tcstok_s(NULL, TEXT(" "), &next);
			if (token != NULL) {
				cmd.parameter = _ttoi(token); // get value from token
				cmd.parameter1 = _ttoi(next);
			}

			if (cmd.parameter != 0 && token != NULL && cmd.parameter1 != 0) // if it read correctly
				CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command)); // send to server

			i++; // move to next cmd

			ReleaseMutex(data->mutexCmd); // unlock access
			SetEvent(data->hCmdEvent);
			ResetEvent(data->hCmdEvent);
		}

		else if (_tcscmp(token, _T("invert")) == 0) {
			
			WaitForSingleObject(data->mutexCmd, INFINITE);
			
			if (i == BUFFERSIZE) {
				i = 0;
			}

			cmd.cmd = 3; // call to third command

			CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command)); // send to server

			i++; // move to next cmd

			ReleaseMutex(data->mutexCmd); // unlock access
			SetEvent(data->hCmdEvent);
			ResetEvent(data->hCmdEvent);
		}
		else if (_tcscmp(token, _T("help")) == 0) {
			_tprintf(TEXT("\nLIST OF COMMANDS: \n"));
			_tprintf(TEXT("\n1 - stop 'amount (seconds)'- stops all cars existing in any track for x seconds"));
			_tprintf(TEXT("\n2 - insert 'row' 'column' - inserts an obstacle in that row/column\n"));
			_tprintf(TEXT("\n3 - invert - inverts the direction of all existing cars\n"));
			_tprintf(TEXT("\n4 - exit\n\n"));
		}

		else if (_tcscmp(token, _T("fim")) == 0) {
			_tprintf(_T("\nEnding commands.\n"));
			break;
		}
		else {
			_tprintf(TEXT("\nNo such command. Try again...\n"));
		}
	} while (_tcscmp(opt, _T("fim")) != 0);

	return 0;
}



DWORD WINAPI receiveGameData(LPVOID params) {
	pData data = (pData)params;

	while (!data->game[0].isShutdown) {
		WaitForSingleObject(data->hReadSem, INFINITE); // wait for sync mechanisms
		WaitForSingleObject(data->hMutex, INFINITE);
		CopyMemory(&data->game[0], &data->sharedMemGame->game[0], sizeof(Game)); // get data from sharedMemGame pointer that has info on game
		ReleaseMutex(data->hMutex); // release mutex
		ReleaseSemaphore(data->hReadSem, 1, NULL); // release read sem after its been used
	}
}

void showBoard(pData data) {
	WaitForSingleObject(data->hMutex, INFINITE);
	_tprintf(TEXT("\n\nTime: [%d]\n\n"), data->game[0].time);

	_tprintf(_T("%d %d"), data->game[0].rows, data->game[0].columns);
	for (DWORD i = 0; i < data->game[0].rows; i++)
	{
		_tprintf(_T("\n"));

		for (DWORD j = 0; j < data->game[0].columns; j++) {
			_tprintf(TEXT("%c"), data->game[0].board[i][j]);
		}
	}

	_tprintf(TEXT("\n\n"));
	_tprintf(_T("\nInsert cmd: "));
	ReleaseMutex(data->hMutex);
}



VOID screenClear() {
	system("cls");
}

DWORD WINAPI showBoardConstant(LPVOID params) {
	pData data = (pData)params;

	while (1) {
		Sleep(1000);
		screenClear();
		_tprintf(_T("\nA printar board...\n"));
		showBoard(data);
	}

	return 0;
}


int _tmain(TCHAR** argv, int argc) {
	srand(time(NULL));
	HANDLE cmdThread;
	HANDLE receiveGameInfoThread;
	HANDLE hPermaShowBoard;
	Data data;
	Game game[2] = { 0 };
	data.game[0] = game[0];
	data.game[1] = game[1];
	
	
	// vars de struct de sharedMM
#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif
	
	//verificação se o server está a correr 
	if (OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_WRITE_NAME) == NULL) {
		_tprintf(_T("\nCan't launch OPERATOR. Server isn't running.\n"));
		return 0;
	}

	if (!createSharedMemoryAndInit(&data)) {
		_tprintf(_T("\nCan't init shared memory...\n"));
		return -2;
	}

	cmdThread = CreateThread(NULL, 0, sendCmdThread, &data, 0, NULL);
	if (cmdThread == NULL) {
		_tprintf(_T("\nCouldn't create SENDCMDTHREAD. [%d]"), GetLastError());
		return -1;
	}

	receiveGameInfoThread = CreateThread(NULL, 0, receiveGameData, &data, 0, NULL);

	if (receiveGameInfoThread == NULL) {
		_tprintf(_T("\nCan't create RECEIVEGAMEINFOTHREAD. [%d]"), GetLastError());
		return -2;
	}

	hPermaShowBoard = CreateThread(NULL, 0, showBoardConstant, &data, 0, NULL);
	
	if (hPermaShowBoard == NULL) {
		_tprintf(_T("\nCan't create SHOWBOARDCONSTANT thread. [%d]\n"), GetLastError());
		return -3;
	} // comentada por agora porque senão está sempre a dar sleep e não recebe info nenhuma, corrigir isso depois

	while (1) { // always verify for game state
		if (data.game[0].isShutdown) {
			_tprintf(_T("\nGAME SHUTTING DOWN...\n"));
			break;
		}
	}

	WaitForSingleObject(cmdThread, INFINITE);
	WaitForSingleObject(receiveGameInfoThread, INFINITE);
	WaitForSingleObject(hPermaShowBoard, INFINITE);
	UnmapViewOfFile(data.sharedMemCmd);
	UnmapViewOfFile(data.sharedMemGame);
	CloseHandle(data.hFileMapFrogger);
	CloseHandle(data.hFileMapMemory);
	CloseHandle(data.hMutex);
	CloseHandle(data.hReadSem);
	
	return 0;
}