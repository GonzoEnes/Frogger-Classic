#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "structs.h"


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

	data->game->board[row][column] = obstacle; // insert obstacle in 
	return 1;
}

DWORD changeDirection(pData data) {
	if (data->game->nCars == 0) {
		return -1;
	}

	for (int i = 0; i < data->game->nCars; i++) {
		switch (data->game->cars[i].direction) {
		case TRUE: // andar para a direita
			data->game->cars[i].direction = FALSE; // andar para a esquerda
		case FALSE:
			data->game->cars[i].direction = TRUE;
		default:
			return -2;
		}
	}
}

DWORD WINAPI receiveCmdFromOperator(LPVOID params) {
	pData data = (pData)params;

	Command command; // command to send
	
	int i = 0;
	
	do {
		if (data->game->isSuspended == FALSE) {
			//WaitForSingleObject(data->commandEvent, INFINITE);
			WaitForSingleObject(data->mutexCmd, INFINITE);
			
			if (i == BUFFERSIZE)
				i = 0;

			CopyMemory(&command, &(data->sharedMemCmd->operatorCmds[i]), sizeof(Command)); // receive cmd from op
			
			i++;
			
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
}

int _tmain(int argc, TCHAR** argv) {

	// aqui são as vars
	
	
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	 // aqui ficam as funcs
}