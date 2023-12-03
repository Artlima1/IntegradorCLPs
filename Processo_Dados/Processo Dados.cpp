
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 

/* ----------------------- Estrutura Dados Mensagem -------------------------------------- */

#define DELIMITADOR_CAMPO ";"
#define DIAG_FALHA 55
#define MSG_TAM_TOT 40
#define MSG_NSEQ_TAM 5
#define MSG_ID_TAM 1
#define MSG_DIAG_TAM 2
#define MSG_PRES_INTERNA_TAM 6
#define MSG_PRES_INJECAO_TAM 6
#define MSG_TEMP_TAM 6
#define MSG_TIMESTAMP_TAM 8

typedef struct {
    int nseq;               // N mero sequencial da mensagem
    int id;                 // Identifica  o do CLP
    int diag;               // Diagn stico dos cart es do CLP
    float pressao_interna;  // Press o interna na panela de gusa
    float pressao_injecao;  // Press o de inje  o do nitrog nio
    float temp;             // Temperatura na panela
    SYSTEMTIME timestamp;   // Horas da mensagem
} mensagem_t;

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

/* ---------------------------- Declaracao Funções Auxiliares -------------------------------------- */

DWORD wait_with_unbloqued_check(HANDLE* hEvents, char* threadName);
int decode_msg(char* msg, mensagem_t* dados);

/* -------------------------------- Funcao Principal -------------------------------------- */

int main()
{
	lista_multithread ListaMsg;
	HANDLE hSwitchDados;
	HANDLE hEsc;
	HANDLE hSection;
	HANDLE hLista[3];
	HANDLE hConsumir[3];
	char msg[MSG_TAM_TOT + 1];
	mensagem_t msg_data;
	int ret;
	char sNomeThread[] = "Exibicao de Dados";

	ListaMsg.hMutex = CreateMutex(NULL, FALSE, "MUTEX_FILA2");
    CheckForError(ListaMsg.hMutex);

    ListaMsg.hSemConsumir = CreateSemaphore(NULL, 0, 50, "SEM_CONSUMIR_FILA2");
    CheckForError(ListaMsg.hSemConsumir);

    ListaMsg.hSemProduzir = CreateSemaphore(NULL, 50, MSG_LIMITE, "SEM_PRODUZIR_FILA2");
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

		decode_msg(msg, &msg_data);
		printf("%02d:%02d:%02d NSEQ: %05d PR INT: %04.1f PR N2: %04.1f TEMP: %04.1f\n", 
			msg_data.timestamp.wHour,
			msg_data.timestamp.wMinute,
			msg_data.timestamp.wSecond,
			msg_data.nseq, 
			msg_data.pressao_interna, 
			msg_data.pressao_injecao,
			msg_data.temp
		);
   	}

	CloseHandle(hSwitchDados);
	CloseHandle(hEsc);
	CloseHandle(hSection);
	ret=UnmapViewOfFile(ListaMsg.lc);
    CheckForError(ret);

	printf("Processo de exibicao de dados encerrado\n");
	Sleep(3000);
}


/* ---------------------------- Implementação Funções Auxiliares -------------------------------------- */

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

int decode_msg(char* msg, mensagem_t* dados) {
    char string_dado[20];
    int pos_atual = 0;

    strncpy_s(string_dado, 20, &msg[pos_atual], MSG_NSEQ_TAM);
    string_dado[MSG_NSEQ_TAM] = '\0';
    dados->nseq = atoi(string_dado);
    pos_atual += MSG_NSEQ_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], MSG_ID_TAM);
    string_dado[MSG_ID_TAM] = '\0';
    dados->id = atoi(string_dado);
    pos_atual += MSG_ID_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], MSG_DIAG_TAM);
    string_dado[MSG_DIAG_TAM] = '\0';
    dados->diag = atoi(string_dado);
    pos_atual += MSG_DIAG_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], MSG_PRES_INTERNA_TAM);
    string_dado[MSG_PRES_INTERNA_TAM] = '\0';
    dados->pressao_interna = atof(string_dado);
    pos_atual += MSG_PRES_INTERNA_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], MSG_PRES_INJECAO_TAM);
    string_dado[MSG_PRES_INJECAO_TAM] = '\0';
    dados->pressao_injecao = atof(string_dado);
    pos_atual += MSG_PRES_INJECAO_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], MSG_TEMP_TAM);
    string_dado[MSG_TEMP_TAM] = '\0';
    dados->temp = atof(string_dado);
    pos_atual += MSG_TEMP_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], 2);
    string_dado[MSG_TEMP_TAM] = '\0';
    dados->timestamp.wHour = atoi(string_dado);
    pos_atual += 3;

    strncpy_s(string_dado, 20, &msg[pos_atual], 2);
    string_dado[MSG_TEMP_TAM] = '\0';
    dados->timestamp.wMinute = atoi(string_dado);
    pos_atual += 3;

    strncpy_s(string_dado, 20, &msg[pos_atual], 2);
    string_dado[MSG_TEMP_TAM] = '\0';
    dados->timestamp.wSecond = atoi(string_dado);
    pos_atual += 3;
}