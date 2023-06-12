#pragma once

#ifdef __cplusplus
#include "../Server/structs.h"
#include "../Server/defines.h"
#include "../Server/pipes.h"

extern "C" {
#endif
	
#ifdef FROGGER_EXPORTS
#define FROGGERDLL_API __declspec(dllexport)
#else
#define FROGGERDLL_API __declspec(dllimport)

#endif

	FROGGERDLL_API BOOL createSharedMemoryAndInitServer(pData p);

	FROGGERDLL_API BOOL createSharedMemoryAndInitOperator(pData data);

#ifdef __cplusplus
}

#endif