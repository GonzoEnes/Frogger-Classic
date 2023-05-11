#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>






void showBoard(ControlData* data) {
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
		for (int i = 0; i < 2; i++) {
			_tprintf(TEXT("\n\nTime: [%d]\n\n"), data->game[i].time);
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

int _tmain(int argc, TCHAR** argv) {
	_tprintf(_T("\nHi from frog.\n"));
	return 0;
}