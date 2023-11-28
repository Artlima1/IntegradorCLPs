
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 

/* ----------------------- Estrutura Lista Circular de Mensagens -------------------------------------- */

#define MSG_TAM_TOT 40
#define MSG_LIMITE 100

typedef struct {
    int pos_livre;
    int pos_ocupada;
    int total;
    char lista[MSG_LIMITE][MSG_TAM_TOT + 1];
} lista_circular_t;

typedef struct {
    HANDLE hMutex;
    HANDLE hSemConsumir;
    HANDLE hSemProduzir;
    lista_circular_t * lc;
} lista_multithread;

DWORD wait_with_unbloqued_check(HANDLE* hEvents, char* threadName);

int main()
{
	lista_multithread ListaMsg;
	HANDLE hSwitchDados;
	HANDLE hEsc;
	HANDLE hSection;
	HANDLE hLista[3];
	HANDLE hConsumir[3];
	char msg[MSG_TAM_TOT + 1];
	int ret;
	char sNomeThread[] = "Thread Exibição de Dados";

	ListaMsg.hMutex = CreateMutex(NULL, FALSE, "MUTEX_FILA2");
    CheckForError(ListaMsg.hMutex);

    ListaMsg.hSemConsumir = CreateSemaphore(NULL, 0, MSG_LIMITE, "SEM_CONSUMIR_FILA2");
    CheckForError(ListaMsg.hSemConsumir);

    ListaMsg.hSemProduzir = CreateSemaphore(NULL, MSG_LIMITE, MSG_LIMITE, "SEM_PRODUZIR_FILA2");
    CheckForError(ListaMsg.hSemProduzir);

	hSection = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(lista_circular_t), "LISTA_CIRCULAR_2");
	CheckForError(hSection);

	ListaMsg.lc = (lista_circular_t *) MapViewOfFile( hSection, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(lista_circular_t));	
	CheckForError(ListaMsg.lc);

	hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	CheckForError(hEsc);

	hSwitchDados = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Dados");
	CheckForError(hSwitchDados);

	hLista[0] = hEsc;
	hLista[1] = hSwitchDados;
	hLista[2] = ListaMsg.hMutex;

	hConsumir[0] = hEsc;
	hConsumir[1] = hSwitchDados;
	hConsumir[2] = ListaMsg.hSemConsumir;

	printf("Thread de dados Inicializada\n");
   	while(1)
	{
		ret = wait_with_unbloqued_check(hConsumir, sNomeThread); // Aguarda haver mensagem a consumir
		if (ret != 0) break; /* Esc pressionado */

		ret = wait_with_unbloqued_check(hLista, sNomeThread); // Aguarda mutex de manipulação da fila
		if (ret != 0) break; /* Esc pressionado */
		memcpy(msg, ListaMsg.lc->lista[ListaMsg.lc->pos_ocupada], MSG_TAM_TOT);
		ListaMsg.lc->pos_ocupada = (ListaMsg.lc->pos_ocupada + 1) % MSG_LIMITE;
		ListaMsg.lc->total--;
		ReleaseMutex(ListaMsg.hMutex);
        ReleaseSemaphore(ListaMsg.hSemProduzir, 1, NULL);

		msg[MSG_TAM_TOT] = '\0';
		printf("Recebida: %s\n", msg);
   	}

	CloseHandle(hSwitchDados);
	CloseHandle(hEsc);
	CloseHandle(hSection);
	ret=UnmapViewOfFile(ListaMsg.lc);
    CheckForError(ret);

	printf("Processo de exibicao de dados encerrado\n");
	Sleep(3000);
}


DWORD wait_with_unbloqued_check(HANDLE* hEvents, char* threadName) {
    DWORD ret = 0;

    do {
        ret = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        if (ret == WAIT_OBJECT_0) break; /* Esc pressionado */
        if (ret == WAIT_OBJECT_0 + 1) { /* Tecla pressionada */
            printf("Thread %s Bloqueada\n", threadName);
            ret = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE); /* Aguarda pressionar tecla novamente */
            if (ret == WAIT_OBJECT_0) {
                break;
            }
            printf("Thread %s Desbloqueada\n", threadName);
        }
    } while (ret != WAIT_OBJECT_0 + 2); /* Volta a aguardar handle quando desbloqueada */

    if (ret == WAIT_OBJECT_0 + 2) {
        return 0;
    }
    else {
        return 1;
    }
}