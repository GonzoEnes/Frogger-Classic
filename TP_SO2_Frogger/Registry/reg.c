#include <tchar.h>
//#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#define MAX 100
#define ERROR_CODE_CANT_CREATE_REG -1
#define REG_CREATED_NEW_KEY_SUCCESSFULLY 0
#define REG_KEY_DOESNT_EXIST -2

int _tmain(int argc, TCHAR** argv) {

	//BOOL firstTime = TRUE;
	
	HKEY regKey;

	TCHAR key_path[MAX] = _T("SOFTWARE\\TP_SO2\\Values");

	DWORD res; // resultado da criação da chave do registry

	TCHAR key_name_lanes[MAX] = _T("MaxLanes"); // esta string segura o nome da chave das faixas de rodagem

	TCHAR key_name_speed[MAX] = _T("MaxSpeed"); // esta string segura o nome da chave da velocidade

	DWORD key_value_lanes; // estas são os valores que vão estar coladas ao par nome-valor, esta refere-se às lanes (faixas de rodagem)

	DWORD key_value_speed; // esta refere-se à velocidade

	//pCars carros; talvez?

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	if (argv != NULL && argc == 3) {

		int convertLanes = _tstoi(argv[1]); // a primeira palavra da linha de comandos será o numero de faixas
		int convertSpeed = _tstoi(argv[2]); // e aqui a velocidade

		// meter os valores da linha de comandos dentro das keys para guardar no reg

		// ver melhor depois a parte de, quando é a primeira vez a executar o programa e não existe nada no reg e o 
		// user também não mete nada na linha de comandos, como tratar disso lá mais para a frente

		key_value_lanes = convertLanes;

		key_value_speed = convertSpeed;

		// criar a chave com os valores especificados:

		if (RegCreateKeyEx(
			HKEY_CURRENT_USER,
			key_path,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&regKey,
			&res
		) != ERROR_SUCCESS)
		{
			_tprintf(_T("\n[ERRO] - Não foi possível a criação da chave principal.\n"));

			return ERROR_CODE_CANT_CREATE_REG;
		}

		// se passar desse if significa que a chave foi criada com sucesso, portanto, atribuir os valores: (que já foram passados pela linha de comandos)

		if (res == REG_CREATED_NEW_KEY) 
		{

			if (RegSetValueEx( // dar o valor ao par nome-valor das LANES 
				regKey,
				key_name_lanes,
				0,
				REG_DWORD,
				(LPBYTE)&key_value_lanes,
				sizeof(key_value_lanes)) != ERROR_SUCCESS)
			{
				_tprintf(_T("\n[ERRO] - Não foi possível adicionar o atributo [%s]", key_name_lanes));
			}

			// a seguir, dar o valor à SPEED, exatamente a mesma coisa

			if (RegSetValueEx( // dar o valor ao par nome-valor das LANES 
				regKey,
				key_name_speed,
				0,
				REG_DWORD,
				(LPBYTE)&key_value_speed,
				sizeof(key_value_speed)) != ERROR_SUCCESS)
			{

				_tprintf(_T("\n[ERRO] - Não foi possível adicionar o atributo [%s]", key_name_speed));
			}


			// se passou por isto tudo, então conseguiu criar o par

			_tprintf(_T("\nChave criada no registry com sucesso!\n"));

			return REG_CREATED_NEW_KEY_SUCCESSFULLY;
		}
	}

	else
	{

		_tprintf(_T("\nNenhum argumento de linha de comandos inserido, a carregar valores do registry, se existirem.\n"));

		if (RegOpenKeyEx(
			HKEY_CURRENT_USER,
			key_path,
			0,
			KEY_READ,
			&regKey) != ERROR_SUCCESS)
		{

			_tprintf(_T("\nNão consegui abrir a chave do registry. Chave não existe."));
			return REG_KEY_DOESNT_EXIST;
		}

	}

	_gettch();
		//return 0;
}