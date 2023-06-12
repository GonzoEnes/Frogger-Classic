// FrogGUI.c : Defines the entry point for the application.
//

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "framework.h"
#include "FrogGUI.h"
#include "frog.h"
#include "Resource.h"
/* ===================================================== */
/* Programa base (esqueleto) para aplica��es Windows     */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
//  Composto por 2 fun��es: 
//	WinMain()     = Ponto de entrada dos programas windows
//			1) Define, cria e mostra a janela
//			2) Loop de recep��o de mensagens provenientes do Windows
//     TrataEventos()= Processamentos da janela (pode ter outro nome)
//			1) � chamada pelo Windows (callback) 
//			2) Executa c�digo em fun��o da mensagem recebida



// Nome da classe da janela (para programas de uma s� janela, normalmente este nome � 
// igual ao do pr�prio programa) "szprogName" � usado mais abaixo na defini��o das 
// propriedades do objecto janela

DWORD WINAPI frogThread(LPVOID params) {
    BOOL returnValue;
    BOOL returnValueWrite;
    DWORD n;
    pFrogData data = (pFrogData)params;

    while (!data->game->isShutdown) {
        WaitForSingleObject(data->hMutex, INFINITE);
        returnValue = ReadFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
        if (!returnValue || !n) {
            break;
        }
        
        //Sleep(1000);
        
        WriteFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
        
        InvalidateRect(data->hWnd, NULL, TRUE);

        Sleep(30);
    }

    ReleaseMutex(data->hMutex);
    
    return (1);
}

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base");
// ============================================================================
// FUN��O DE IN�CIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa come�a sempre a sua execu��o na fun��o WinMain()que desempenha
// o papel da fun��o main() do C em modo consola WINAPI indica o "tipo da fun��o" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as fun��es de
// processamento da janela)
// Par�metros:
//   hInst: Gerado pelo Windows, � o handle (n�mero) da inst�ncia deste programa 
//   hPrevInst: Gerado pelo Windows, � sempre NULL para o NT (era usado no Windows 3.1)
//   lpCmdLine: Gerado pelo Windows, � um ponteiro para uma string terminada por 0
//              destinada a conter par�metros para o programa 
//   nCmdShow:  Par�metro que especifica o modo de exibi��o da janela (usado em  
//        	   ShowWindow()

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    HWND hWnd;		// hWnd � o handler da janela, gerado mais abaixo por CreateWindow()
    MSG lpMsg;		// MSG � uma estrutura definida no Windows para as mensagens
    WNDCLASSEX wcApp;	// WNDCLASSEX � uma estrutura cujos membros servem para 
    Game game;
    FrogData data;
    HANDLE hFroggerThread;
    HANDLE hMutex;

    hMutex = CreateMutex(NULL, FALSE, NULL);

    data.hMutex = hMutex;

    game.isShutdown = FALSE;
    game.isSuspended = FALSE;

    data.game = &game;

    data.hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    hFroggerThread = CreateThread(NULL, 0, frogThread, &data, 0, NULL);

    if (hFroggerThread == NULL) {
        _tprintf(_T("\nError creating thread for frog.\n"));
        exit(-100);
    }

              // definir as caracter�sticas da classe da janela

    // ============================================================================
    // 1. Defini��o das caracter�sticas da janela "wcApp" 
    //    (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
    // ============================================================================
    wcApp.cbSize = sizeof(WNDCLASSEX);      // Tamanho da estrutura WNDCLASSEX
    wcApp.hInstance = hInst;		         // Inst�ncia da janela actualmente exibida 
                                   // ("hInst" � par�metro de WinMain e vem 
                                         // inicializada da�)
    wcApp.lpszClassName = szProgName;       // Nome da janela (neste caso = nome do programa)
    wcApp.lpfnWndProc = TrataEventos;       // Endere�o da fun��o de processamento da janela
                                            // ("TrataEventos" foi declarada no in�cio e
                                            // encontra-se mais abaixo)
    wcApp.style = CS_HREDRAW | CS_VREDRAW;  // Estilo da janela: Fazer o redraw se for
                                            // modificada horizontal ou verticalmente

    wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);   // "hIcon" = handler do �con normal
                                           // "NULL" = Icon definido no Windows
                                           // "IDI_AP..." �cone "aplica��o"
    wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION); // "hIconSm" = handler do �con pequeno
                                           // "NULL" = Icon definido no Windows
                                           // "IDI_INF..." �con de informa��o
    wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);	// "hCursor" = handler do cursor (rato) 
                              // "NULL" = Forma definida no Windows
                              // "IDC_ARROW" Aspecto "seta" 
    wcApp.lpszMenuName = NULL;			// Classe do menu que a janela pode ter
                              // (NULL = n�o tem menu)
    wcApp.cbClsExtra = 0;				// Livre, para uso particular
    wcApp.cbWndExtra = sizeof(pFrogData);				// Livre, para uso particular
    wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    // "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
    // "GetStockObject".Neste caso o fundo ser� branco

    // ============================================================================
    // 2. Registar a classe "wcApp" no Windows
    // ============================================================================
    if (!RegisterClassEx(&wcApp))
        return(0);

    // ============================================================================
    // 3. Criar a janela
    // ============================================================================
    hWnd = CreateWindow(
        szProgName, // Nome da janela (programa) definido acima
        TEXT("Frogger - SO2"), // Texto que figura na barra do t�tulo
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, // Estilo da janela (WS_OVERLAPPED= normal)
        CW_USEDEFAULT, // Posi��o x pixels (default=� direita da �ltima)
        CW_USEDEFAULT, // Posi��o y pixels (default=abaixo da �ltima)
        1600, // Largura da janela (em pixels)
        800, // Altura da janela (em pixels)
        (HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
        // outra) ou HWND_DESKTOP se a janela for a primeira, 
        // criada a partir do "desktop"
        (HMENU)NULL, // handle do menu da janela (se tiver menu)
        (HINSTANCE)hInst, // handle da inst�ncia do programa actual ("hInst" � 
        // passado num dos par�metros de WinMain()
        0); // N�o h� par�metros adicionais para a janela


    data.hWnd = hWnd;

    SetWindowLongPtr(hWnd, 0, (LONG_PTR)&data);

    if (data.hPipe == INVALID_HANDLE_VALUE) {
        DestroyWindow(hWnd);
    }

    HWND hwndButtonQuit = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Quit",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        0,         // x position 
        0,         // y position 
        100,        // Button width
        75,        // Button height
        hWnd,     // Parent window
        (HMENU)BTN_QUIT,       // No menu.
        NULL,
        NULL);
      // ============================================================================
      // 4. Mostrar a janela
      // ============================================================================
    ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela, devolvido por 
                      // "CreateWindow"; "nCmdShow"= modo de exibi��o (p.e. 
                      // normal/modal); � passado como par�metro de WinMain()
    UpdateWindow(hWnd);		// Refrescar a janela (Windows envia � janela uma 
                      // mensagem para pintar, mostrar dados, (refrescar)� 
    // ============================================================================
    // 5. Loop de Mensagens
    // ============================================================================
    // O Windows envia mensagens �s janelas (programas). Estas mensagens ficam numa fila de
    // espera at� que GetMessage(...) possa ler "a mensagem seguinte"	
    // Par�metros de "getMessage":
    // 1)"&lpMsg"=Endere�o de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no  
    //   in�cio de WinMain()):
    //			HWND hwnd		handler da janela a que se destina a mensagem
    //			UINT message		Identificador da mensagem
    //			WPARAM wParam		Par�metro, p.e. c�digo da tecla premida
    //			LPARAM lParam		Par�metro, p.e. se ALT tamb�m estava premida
    //			DWORD time		Hora a que a mensagem foi enviada pelo Windows
    //			POINT pt		Localiza��o do mouse (x, y) 
    // 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
    //   receber as mensagens para todas as
    // janelas pertencentes � thread actual)
    // 3)C�digo limite inferior das mensagens que se pretendem receber
    // 4)C�digo limite superior das mensagens que se pretendem receber

    // NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
    // 	  terminando ent�o o loop de recep��o de mensagens, e o programa 

    while (GetMessage(&lpMsg, NULL, 0, 0)) {
        TranslateMessage(&lpMsg);	// Pr�-processamento da mensagem (p.e. obter c�digo 
                       // ASCII da tecla premida)
        DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
                       // aguarda at� que a possa reenviar � fun��o de 
                       // tratamento da janela, CALLBACK TrataEventos (abaixo)
    }

    // ============================================================================
    // 6. Fim do programa
    // ============================================================================
    return((int)lpMsg.wParam);	// Retorna sempre o par�metro wParam da estrutura lpMsg
}

// ============================================================================
// FUN��O DE PROCESSAMENTO DA JANELA
// Esta fun��o pode ter um nome qualquer: Apenas � neces�rio que na inicializa��o da
// estrutura "wcApp", feita no in�cio de // WinMain(), se identifique essa fun��o. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pr�-processadas
// no loop "while" da fun��o WinMain()
// Par�metros:
//		hWnd	O handler da janela, obtido no CreateWindow()
//		messg	Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
//		wParam	O par�metro wParam da estrutura messg (a mensagem)
//		lParam	O par�metro lParam desta mesma estrutura
//
// NOTA:Estes par�metros est�o aqui acess�veis o que simplifica o acesso aos seus valores
//
// A fun��o EndProc � sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar.
// Estas mensagens s�o identificadas por constantes (p.e. 
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
// ============================================================================

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
    DWORD xPos;
    DWORD yPos;
    RECT rect;
    HDC hdc;
    pFrogData data;
    PAINTSTRUCT ps;
    DWORD totalPixels = 0;
    DWORD n;
    data = (pFrogData)GetWindowLongPtr(hWnd, 0);
    static HBITMAP hBitmap;
    static HBITMAP hBitmapBeginEnd;
    static HBITMAP hBitmapCar;
    static HBITMAP hBitmapFrog;
    static HBITMAP hBitmapObstacle;
    static BITMAP bitmap = { 0 };
    static BITMAP bitmapBeginEnd = { 0 };
    static BITMAP bitmapCar = { 0 };
    static BITMAP bitmapFrog = { 0 };
    static BITMAP bitmapObstacle = { 0 };
    static HDC bitmapDC = NULL;
    static HDC bitmapBeginEndDC = NULL;
    static HDC bitmapCarDC = NULL;
    static HDC bitmapFrogDC = NULL;
    static HDC bitmapObstacleDC = NULL;
    static int xBitmap;
    static int yBitmap;
    static int limDir;
    static HDC memDC = NULL;
    static int speed;
    HANDLE hMutex;

    switch (messg) {
    case WM_CREATE:
        hBitmap = (HBITMAP)LoadImage(NULL, TEXT("black.bmp"), IMAGE_BITMAP, 55, 55, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
        hBitmapBeginEnd = (HBITMAP)LoadImage(NULL, TEXT("azul.bmp"), IMAGE_BITMAP, 55, 55, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
        hBitmapCar = (HBITMAP)LoadImage(NULL, TEXT("carro.bmp"), IMAGE_BITMAP, 55, 55, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
        hBitmapFrog = (HBITMAP)LoadImage(NULL, TEXT("sapo.bmp"), IMAGE_BITMAP, 55, 55, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
        hBitmapObstacle = (HBITMAP)LoadImage(NULL, TEXT("obstacle.bmp"), IMAGE_BITMAP, 55, 55, LR_LOADFROMFILE | LR_LOADTRANSPARENT);

        GetObject(hBitmap, sizeof(bitmap), &bitmap);
        GetObject(hBitmapBeginEnd, sizeof(bitmapBeginEnd), &bitmapBeginEnd);
        GetObject(hBitmapCar, sizeof(bitmapCar), &bitmapCar);
        GetObject(hBitmapFrog, sizeof(bitmapFrog), &bitmapFrog);
        GetObject(hBitmapObstacle, sizeof(bitmapObstacle), &bitmapObstacle);

        hdc = GetDC(hWnd);
        bitmapDC = CreateCompatibleDC(hdc);
        bitmapBeginEndDC = CreateCompatibleDC(hdc);
        bitmapCarDC = CreateCompatibleDC(hdc);
        bitmapFrogDC = CreateCompatibleDC(hdc);
        bitmapObstacleDC = CreateCompatibleDC(hdc);


        SelectObject(bitmapDC, hBitmap);
        SelectObject(bitmapBeginEndDC, hBitmapBeginEnd);
        SelectObject(bitmapCarDC, hBitmapCar);
        SelectObject(bitmapFrogDC, hBitmapFrog);
        SelectObject(bitmapObstacleDC, hBitmapObstacle);

        ReleaseDC(hWnd, hdc);
        GetClientRect(hWnd, &rect);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rect);
        FillRect(hdc, &rect, CreateSolidBrush(RGB(0, 0, 255)));
        totalPixels = 55 * data->game->columns;		// colocar colunas que le do pipe no futuro
        xBitmap = (800 - (totalPixels / 2));
        yBitmap = 150;

        for (int i = 0; i < data->game->rows; i++) { // colocar linhas que le do pipe no futuro
            for (int j = 0; j < data->game->columns; j++) {	// colocar colunas que le do pipe no futuro
                if (i == data->game->rows - 1 || i == 0) { // se estivermos na primeira/ultima coluna, pinta de azul
                    BitBlt(hdc, xBitmap, yBitmap, bitmapBeginEnd.bmWidth, bitmapBeginEnd.bmHeight, bitmapBeginEndDC, 0, 0, SRCCOPY);
                }
                else { //sen�o pinta de preto
                    BitBlt(hdc, xBitmap, yBitmap, bitmap.bmWidth, bitmap.bmHeight, bitmapDC, 0, 0, SRCCOPY);
                }

                /*if (data->game->board[i][j] == _T('s')) {
                    BitBlt(hdc, xBitmap, yBitmap, bitmapFrog.bmWidth, bitmapFrog.bmHeight, bitmapFrogDC, 0, 0, SRCCOPY);
                }*/

                if (i == data->game->player1.y && j == data->game->player1.x) {
                    BitBlt(hdc, xBitmap, yBitmap, bitmapFrog.bmWidth, bitmapFrog.bmHeight, bitmapFrogDC, 0, 0, SRCCOPY);
                }


                else if (data->game->board[i][j] == _T('c')) {
                    BitBlt(hdc, xBitmap, yBitmap, bitmapCar.bmWidth, bitmapCar.bmHeight, bitmapCarDC, 0, 0, SRCCOPY);
                }

                else if (data->game->board[i][j] == _T('O')) {
                    //tratar do bitmap do obst�culo
                }

                xBitmap = xBitmap + 55;
            }
            xBitmap = (800 - (totalPixels / 2));

            yBitmap = yBitmap + 55;
        }

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

        EndPaint(hWnd, &ps);
        
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:
            // Move frog up
            hdc = GetDC(hWnd);
            data->game->player1.y--;
            //data->game->nCars = 0;
            //MessageBox(hWnd, TEXT("Movi - me"), TEXT("BOAS"), MB_OK | MB_ICONEXCLAMATION)
            //InvalidateRect(hWnd, NULL, TRUE);
            ReleaseDC(hWnd, hdc);
            break;
        case VK_DOWN:
            // Move frog down
            hdc = GetDC(hWnd);

            data->game->player1.y++;
            ReleaseDC(hWnd, hdc);
            // InvalidateRect(hWnd, NULL, TRUE);
            break;
        case VK_LEFT:
            hdc = GetDC(hWnd);
            // Move frog left
            data->game->player1.x--;
            //InvalidateRect(hWnd, NULL, TRUE);
            ReleaseDC(hWnd, hdc);
            break;
        case VK_RIGHT:
            hdc = GetDC(hWnd);
            // Move frog right
            data->game->player1.x++;
            // InvalidateRect(hWnd, NULL, TRUE);
            ReleaseDC(hWnd, hdc);
            break;
        }
        break;

        case WM_COMMAND:
            if (LOWORD(wParam) == BTN_QUIT) {
                data->game->isShutdown = TRUE;
                Sleep(2000);
                DestroyWindow(hWnd);
            }
        break;
        case WM_CLOSE:
            if (MessageBox(hWnd, _T("Are you sure you want to leave?"), _T("Leave"), MB_YESNO) == IDYES)
                DestroyWindow(hWnd);
            break;
        case WM_DESTROY:	// Destruir a janela e terminar o programa 
                            // "PostQuitMessage(Exit Status)"		
            DeleteDC(memDC);
            //DeleteObject(hBitmapBuffer);
            DeleteObject(hBitmap);
            DeleteObject(hBitmapBeginEnd);
            DeleteObject(hBitmapCar);
            DeleteObject(hBitmapFrog);
            DeleteObject(hBitmapObstacle);
            PostQuitMessage(0);
            break;
        default:
            // Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
            // n�o � efectuado nenhum processamento, apenas se segue o "default" do Windows
            return(DefWindowProc(hWnd, messg, wParam, lParam));
            break;  // break tecnicamente desnecess�rio por causa do return
        }
    return(0);
}
