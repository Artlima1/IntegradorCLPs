#include <conio.h>
#include <process.h>  // _beginthreadex() e _endthreadex()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <atlstr.h> 
#include <sstream>  
#include <cstring>
#include <format>
#include <cstdio>

using namespace std;
#include "CheckForError.h";

#define N_ALARMES 7


#define DIAG_FALHA 55
#define MSG_LIMITE2 50
#define MSG_TAM_TOT 40
#define MSG_NSEQ_TAM 5
#define MSG_ID_TAM 1
#define MSG_DIAG_TAM 2
#define MSG_PRES_INTERNA_TAM 6
#define MSG_PRES_INJECAO_TAM 6
#define MSG_TEMP_TAM 6
#define MSG_TIMESTAMP_TAM 8

#define ALARME_TAM_TOT 17
#define ALARME_NSEQ_TAM 5
#define ALARME_ID_TAM 2
#define ALARME_TIMESTAMP_TAM 8

typedef struct {
	int nseq;               // N mero sequencial da mensagem
	int id;                 // Identifica  o do CLP
	int diag;               // Diagn stico dos cart es do CLP
	float pressao_interna;  // Press o interna na panela de gusa
	float pressao_injecao;  // Press o de inje  o do nitrog nio
	float temp;             // Temperatura na panela
	SYSTEMTIME timestamp;   // Horas da mensagem
} mensagem_t;

typedef struct {
	char id[ALARME_ID_TAM + 1];
	char descricao[100];
} alarme_code_t;

typedef struct {
	int nseq;                   // N mero sequencial da mensagem
	char id[ALARME_ID_TAM + 1];   // Identifica  o do CLP
	SYSTEMTIME timestamp;       // Horas da mensagem
} alarme_t;

typedef struct {
	int nseq;                   // N mero sequencial da mensagem
	char id[ALARME_ID_TAM + 1];   // Identifica  o do CLP
	char timestamp[ALARME_TIMESTAMP_TAM+50];
}alarme_envio;

alarme_code_t lista_alarmes[N_ALARMES] = {
	{"A2", "Esteira Parou"},
	{"T1", "Temperatura baixa para a reacao"},
	{"Q2", "Baixa pressao de injecao do nitrogenio"},
	{"C4", "Concentracao baixa de carbono no sistema"},
	{"C6", "Concentracao baixa de silicio no sistema"},
	{"1", "FALHA DE HARDWARE CLP No. "},
	{"2", "FALHA DE HARDWARE CLP No."}
};

int decode_msg(char* msg, mensagem_t* dados);
int decode_alarme(char* msg, alarme_t* dados);

HANDLE hEventos[2];
HANDLE hSwitchAlarmes;
HANDLE hEsc;
HANDLE hMailslot;
HANDLE hMsg;
HANDLE hSemaforos[2];
HANDLE semaforo_alarme;
HANDLE semaforo_diag55;
int ret = 0;
BOOL MS;
char msg[150];
DWORD bytes;
alarme_t alarme;
mensagem_t diag55;
alarme_code_t texto;
int tempo;
char print_mensagem[20];

int main()
{

	hSwitchAlarmes = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Alarmes");
	if (hSwitchAlarmes == NULL) {
		printf("ERROR : %d", GetLastError());
	}
	hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	if (hEsc == NULL)
	{
		printf("ERROR : %d", GetLastError());
	}

	semaforo_alarme = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, "msg_alarme");
	if (semaforo_alarme == NULL)
	{
		printf("Erro na abertura do evento de alarmes : %d\n", GetLastError());
	}
	semaforo_diag55 = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, "msg_diag55");
	hSemaforos[0] = semaforo_alarme;
	hSemaforos[1] = semaforo_diag55;


	hEventos[0] = hEsc;
	hEventos[1] = hSwitchAlarmes;

	hMailslot = CreateMailslot("\\\\.\\mailslot\\ms_alarmes",
		0,
		5,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	hMsg = OpenEvent(EVENT_ALL_ACCESS, FALSE, "msg");
	if (hMsg == NULL)
	{
		printf("Error no evento do MS : %d", GetLastError());
	}


	printf("Thread de alarmes Inicializada\n");
	SetEvent(hMsg);
	while(1) 
	{
		ret = WaitForMultipleObjects(2, hEventos, FALSE, 0);
		if (ret == WAIT_OBJECT_0) break;
		if (ret == WAIT_OBJECT_0 + 1) {
			printf("Thread de alarmes Bloqueada\n");
			ret = WaitForMultipleObjects(2, hEventos, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0) {
				break;
			}
			printf("Thread de alarmes Desbloqueada\n");
		}
		
	
		
		ret = WaitForMultipleObjects(2, hSemaforos, FALSE, 0);
		if(ret == WAIT_OBJECT_0)
		{

			MS = ReadFile(hMailslot, &alarme, sizeof(alarme_t), &bytes, NULL);
			CheckForError(MS);
			for (int i = 0; i < N_ALARMES; i++)
			{
				if (strcmp(alarme.id, lista_alarmes[i].id) == 0) 
				{
					memcpy(&texto, &lista_alarmes[i], sizeof(alarme_code_t));
					break;
				}
			}
			
			printf("%02d:%02d:%02d NSEQ: %05d ID: %s %s\n", alarme.timestamp.wHour,alarme.timestamp.wMinute,alarme.timestamp.wSecond, alarme.nseq, alarme.id, texto.descricao);
			
		}
		if(ret == WAIT_OBJECT_0 + 1)
		{
			MS = ReadFile(hMailslot, &alarme, sizeof(alarme_envio), &bytes, NULL);
			CheckForError(MS);
			for (int i = 0; i < N_ALARMES; i++)
			{
				if (strcmp(alarme.id, lista_alarmes[i].id) == 0)
				{
					memcpy(&texto, &lista_alarmes[i], sizeof(alarme_code_t));
					break;
				}
			}
			printf("%02d:%02d:%02d NSEQ: %05d FALHA NO HARDWARE CLP No %s", alarme.timestamp.wHour, alarme.timestamp.wMinute, alarme.timestamp.wSecond, alarme.nseq, alarme.id);
		}

			
	}

	CloseHandle(hSwitchAlarmes);
	CloseHandle(hEsc);
	CloseHandle(hMsg);
	CloseHandle(hMailslot);
	CloseHandle(semaforo_alarme);
	CloseHandle(hSemaforos);
	CloseHandle(semaforo_diag55);


	printf("Processo de Alarmes encerrando execucao\n");
	Sleep(3000);
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

int decode_alarme(char* msg, alarme_t* dados) {
	char string_dado[20];
	int pos_atual = 0;


	strncpy_s(string_dado, 20, &msg[pos_atual], ALARME_NSEQ_TAM);
	string_dado[ALARME_NSEQ_TAM] = '\0';
	dados->nseq = atoi(string_dado);
	pos_atual += ALARME_NSEQ_TAM + 1;

	strncpy_s(string_dado, 20, &msg[pos_atual], ALARME_ID_TAM);
	string_dado[ALARME_ID_TAM] = '\0';
	strncpy_s(dados->id, ALARME_ID_TAM + 1, &msg[pos_atual], ALARME_ID_TAM);
	dados->id[ALARME_ID_TAM] = '\0';
	pos_atual += ALARME_ID_TAM + 1;

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


