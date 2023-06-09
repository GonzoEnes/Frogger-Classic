#pragma once
#ifndef PIPES

#include <Windows.h>
#define NUM 2
#define PIPE_NAME _T("\\\\.\\pipe\\FroggerGame")

typedef struct PIPES PipeInfo, * pPipeInfo;
typedef struct THREAD_DATA ThreadData, *pThreadData;

typedef struct PIPES {
	HANDLE hInstance;
	OVERLAPPED overlap;
	BOOL active;
};

typedef struct THREAD_DATA {
	PipeInfo hPipe[NUM];
	HANDLE hEvents[NUM];
	HANDLE hPipeMutex;
	DWORD end;
}; 

#endif