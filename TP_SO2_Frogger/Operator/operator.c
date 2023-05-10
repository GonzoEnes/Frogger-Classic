#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "../Server/structs.h"
#include "../Server/game.h"
#include "../Server/defines.h"


BOOL createSharedMemoryAndInit(pData data) {
	// acabar shared memory para poder testar os comandos
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

	int i = 0;

	do {
		_getts_s(opt, _countof(opt)); // fetches user choice with limit of strlen(opt)
		token = _tcstok_s(opt, TEXT(" "), &next);

		if (_tcscmp(token, _T("stopcars")) == 0) {
			WaitForSingleObject(data->mutexCmd, INFINITE);
			if (i == BUFFERSIZE) {
				i = 0;
			}

			cmd.cmd = 1; // call to first command

			token = _tcstok_s(NULL, TEXT(" "), &next);
			if (token != NULL)
				cmd.parameter = _ttoi(token); // get value from token
			if (cmd.parameter != 0 && token != NULL) // if it read correctly
				CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command)); // send to server
			
			i++; // move to next cmd

			ReleaseMutex(data->mutexCmd); // unlock access
		}

		else if (_tcscmp(token, _T("insert")) == 0) {
			WaitForSingleObject(data->mutexCmd, INFINITE);
			if (i == BUFFERSIZE) {
				i = 0;
			}

			cmd.cmd = 2; // call to second command

			token = _tcstok_s(NULL, TEXT(" "), &next);
			if (token != NULL)
				cmd.parameter = _ttoi(token); // get value from token
			if (cmd.parameter != 0 && token != NULL) // if it read correctly
				CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command)); // send to server

			i++; // move to next cmd

			ReleaseMutex(data->mutexCmd); // unlock access
		}

		else if (_tcscmp(token, _T("invert")) == 0) {
			WaitForSingleObject(data->mutexCmd, INFINITE);
			if (i == BUFFERSIZE) {
				i = 0;
			}

			cmd.cmd = 3; // call to third command

			token = _tcstok_s(NULL, TEXT(" "), &next);
			if (token != NULL)
				cmd.parameter = _ttoi(token); // get value from token
			if (cmd.parameter != 0 && token != NULL) // if it read correctly
				CopyMemory(&(data->sharedMemCmd->operatorCmds[i]), &cmd, sizeof(Command)); // send to server

			i++; // move to next cmd

			ReleaseMutex(data->mutexCmd); // unlock access
		}
		else if (_tcscmp(token, _T("help")) == 0) {
			_tprintf(TEXT("\nLIST OF COMMANDS: \n"));
			_tprintf(TEXT("\n1 - stop 'amount (seconds)'- stops all cars existing in any track for x seconds"));
			_tprintf(TEXT("\n2 - insert 'row' 'column' - inserts an obstacle in that row/column\n"));
			_tprintf(TEXT("\n3 - invert - inverts the direction of all existing cars\n"));
			_tprintf(TEXT("\n4 - exit\n\nCommand: "));
		}
		else {
			_tprintf(TEXT("\nNo such command. Try again...\n\nCommand: "));
		}
	} while (_tcscmp(opt, _T("fim")) != 0);

	return 0;
}

int _tmain(TCHAR** argv, int argc) {
	HANDLE cmdThread;
	Data data;
	// vars de struct de sharedMM
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	cmdThread = CreateThread(NULL, 0, sendCmdThread, &data, 0, NULL);
	if (cmdThread == NULL) {
		_tprintf(_T("\nCouldn't create SENDCMDTHREAD. [%d]", GetLastError()));
		return -1;
	}
}