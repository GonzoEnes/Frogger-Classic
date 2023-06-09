// FrogGUI.c : Defines the entry point for the application.
//

#include <windows.h>
#include <tchar.h>
#include "framework.h"
#include "FrogGUI.h"
#include "frog.h"
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

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);

// Nome da classe da janela (para programas de uma s� janela, normalmente este nome � 
// igual ao do pr�prio programa) "szprogName" � usado mais abaixo na defini��o das 
// propriedades do objecto janela
TCHAR szProgName[] = TEXT("Base");

DWORD WINAPI frogThread(LPVOID params) {
    BOOL returnValue;
    DWORD n;
    pFrogData data = (pFrogData)params;

    while (!data->game->isShutdown) {
        returnValue = ReadFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
        if (!returnValue || &n == 0) {
            break;
        }
        WriteFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
    }
    return 1;
}

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
    wcApp.cbWndExtra = 0;				// Livre, para uso particular
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
        szProgName,			// Nome da janela (programa) definido acima
        TEXT("Frogger"),// Texto que figura na barra do t�tulo
        WS_OVERLAPPEDWINDOW,	// Estilo da janela (WS_OVERLAPPED= normal)
        CW_USEDEFAULT,		// Posi��o x pixels (default=� direita da �ltima)
        CW_USEDEFAULT,		// Posi��o y pixels (default=abaixo da �ltima)
        CW_USEDEFAULT,		// Largura da janela (em pixels)
        CW_USEDEFAULT,		// Altura da janela (em pixels)
        (HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de
                        // outra) ou HWND_DESKTOP se a janela for a primeira, 
                        // criada a partir do "desktop"
        (HMENU)NULL,			// handle do menu da janela (se tiver menu)
        (HINSTANCE)hInst,		// handle da inst�ncia do programa actual ("hInst" � 
                        // passado num dos par�metros de WinMain()
        0);				// N�o h� par�metros adicionais para a janela
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
    switch (messg) {
    case WM_DESTROY:	// Destruir a janela e terminar o programa 
                        // "PostQuitMessage(Exit Status)"		
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




/*#include "framework.h"
#include "FrogGUI.h"
#include "frog.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI frogThread(LPVOID params) {
    BOOL returnValue;
    DWORD n;
    pFrogData data = (pFrogData)params;

    while (!data->game->isShutdown) {
        returnValue = ReadFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
        if (!returnValue || &n == 0) {
            break;
        }
        WriteFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
    }
    return 1;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FROGGUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FROGGUI));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FROGGUI));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_FROGGUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}*/
