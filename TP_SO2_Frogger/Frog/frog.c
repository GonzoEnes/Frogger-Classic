#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "../Server/structs.h"
#include "../Server/defines.h"


void showGame(pGame game) {
	for (DWORD i = 0; i < game->rows; i++) {
		_tprintf(_T("\n"));
		for (DWORD j = 0; j < game->columns; j++) {
			_tprintf(_T("%c "), game->board[i][j]);
		}
	}
	_tprintf(_T("\n\n"));
}

void playFrogger(pGame game, HANDLE hPipeComms) {
    BOOL ret;
    DWORD nBytes;
    TCHAR opt = 0;

    while (!game->isShutdown) {
        ret = ReadFile(hPipeComms, game, sizeof(Game), &nBytes, NULL);

        game->isSuspended = FALSE;

        showGame(game);

        _tprintf(_T("\nTEMPO JOGO: %d"), game->time);

        _tprintf(_T("\n"));

        if (!ret || !nBytes) { // if ReadFile failed or read 0 bytes, then:
            _tprintf(_T("\n[ERROR] Frogger game shutting down...\n"));
            game->isShutdown = TRUE;
            return;
        }

        _tprintf(_T("\nData read successfully. Starting game...\n"));

        _tprintf(_T("\nMove (PAUSE to pause the game): "));
        
        _tcscanf_s(_T("%c"), &opt);

        if (opt == _T('W')) {
            game->player1.y -= 1;
        }
        else if (opt == _T('A')) {
            game->player1.x -= 1;
        }
        else if (opt == _T('S')) {
            game->player1.y += 1;
        }
        else if (opt == _T('D')) {
            game->player1.x += 1;
        }
        else {
            _tprintf(_T("\nInsert a valid option!\n"));
            continue; 
        }

        if (!WriteFile(hPipeComms, game, sizeof(Game), &nBytes, NULL)) {
            _tprintf(_T("\n[ERROR] Can't write back to server. Failed writing to pipe.\n"));
        }
        else {
            _tprintf(_T("\nData successfully sent to server...\n"));
        }

        if (game->isSuspended) {
            break;
        }
    }
}

int _tmain(int argc, TCHAR** argv) {

	Game game;

	HANDLE hPipeComms;

	game.isShutdown = FALSE;
	game.isSuspended = FALSE;

	TCHAR opt[BUFFER];


#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

	_tprintf(_T("\n--------------FROG-------------\n"));

	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(_T("\nCan't connect to named pipe. Server isn't running. [%d]\n"), GetLastError());
		return -1;
	}

	hPipeComms = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hPipeComms == NULL) {
		_tprintf(_T("\nCan't connect to named pipe. [%d]\n"), GetLastError());
		return -2;
	}

	_tprintf(_T("\nSuccessfully connected.\n"));

	while (!game.isShutdown) {
		if (!game.isSuspended) {
			playFrogger(&game, hPipeComms);
		}

		else {
			_tprintf(_T("\nTo unpause the game, press any key..."));
			_fgetts(opt, 256, stdin);
			game.isSuspended = FALSE;
		}
	}

	CloseHandle(hPipeComms);
	return 0;
}