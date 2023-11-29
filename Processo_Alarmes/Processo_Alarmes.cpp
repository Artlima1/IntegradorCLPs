#include <conio.h>
#include <process.h>  // _beginthreadex() e _endthreadex()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "CheckForError.h";

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
	int nseq;                   // N mero sequencial da mensagem
	char id[ALARME_ID_TAM + 1];   // Identifica  o do CLP
	SYSTEMTIME timestamp;       // Horas da mensagem
} alarme_t;

typedef struct {
	char id[ALARME_ID_TAM + 1];
	char descricao[100];
} alarme_code_t;

#define N_ALARMES 5
alarme_code_t lista_alarmes[N_ALARMES] = {
	{"A2", "Esteira Parou"},
	{"T1", "Temperatura baixa para a reacao"},
	{"Q2", "Baixa pressao de injecao do nitrogenio"},
	{"C4", "Concentracao baixa de carbono no sistema"},
	{"C6", "Concentracao baixa de silicio no sistema"}
};

int decode_msg(char* msg, mensagem_t* dados);
int decode_alarme(char* msg, alarme_t* dados);

DWORD wait_with_unbloqued_check(HANDLE* hEvents, int N, char* threadName);

int main()
{
	char sNomeThread[] = "Thread Exibicao de Alarmes";
	HANDLE hSwitchAlarmes;
	HANDLE hEsc;
	HANDLE hSemAlarmeCartoes;
	HANDLE hSemAlarmeCritico;
	HANDLE hEventos[4];
	HANDLE hMailslot;
	int ret;
	BOOL MS;
	DWORD bytes;
	alarme_t alarme;
	alarme_code_t alarme_texto;
	mensagem_t msg_data;

	hSwitchAlarmes = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Alarmes");
	CheckForError(hSwitchAlarmes);

	hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	CheckForError(hEsc);

	hSemAlarmeCritico = CreateSemaphore(NULL, 0, 10000, "SEM_ALARME_CRITICO");
	CheckForError(hSemAlarmeCritico);

	hSemAlarmeCartoes = CreateSemaphore(NULL, 0, 10000, "SEM_ALARME_CARTOES");
	CheckForError(hSemAlarmeCartoes);

	hEventos[0] = hEsc;
	hEventos[1] = hSwitchAlarmes;
	hEventos[2] = hSemAlarmeCritico;
	hEventos[3] = hSemAlarmeCartoes;

	hMailslot = CreateMailslot("\\\\.\\mailslot\\ms_alarmes", 0, 5, NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	printf("%s Inicializada\n", sNomeThread);
	while(1) 
	{
		ret = wait_with_unbloqued_check(hEventos, 4, sNomeThread);
		if(ret == 0) break;
		if(ret == 2) {
			MS = ReadFile(hMailslot, &alarme, sizeof(alarme_t), &bytes, NULL);
			CheckForError(MS);
			for (int i = 0; i < N_ALARMES; i++) {
				if (strcmp(alarme.id, lista_alarmes[i].id) == 0) {
					memcpy(&alarme_texto, &lista_alarmes[i], sizeof(alarme_code_t));
					break;
				}
			}
			printf("%02d:%02d:%02d NSEQ: %05d ID: %s %s\n", alarme.timestamp.wHour,alarme.timestamp.wMinute, alarme.timestamp.wSecond, alarme.nseq, alarme.id, alarme_texto.descricao);
		}
		if(ret == 3) {
			MS = ReadFile(hMailslot, &msg_data, sizeof(mensagem_t), &bytes, NULL);
			printf("%02d:%02d:%02d NSEQ: %05d FALHA NO HARDWARE CLP No %d\n", msg_data.timestamp.wHour, msg_data.timestamp.wMinute, msg_data.timestamp.wSecond, msg_data.nseq, msg_data.id);
		}

			
	}

	CloseHandle(hSwitchAlarmes);
	CloseHandle(hEsc);
	CloseHandle(hSemAlarmeCritico);
	CloseHandle(hSemAlarmeCartoes);

	printf("Processo de Alarmes encerrado\n");
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

DWORD wait_with_unbloqued_check(HANDLE* hEvents, int N, char* threadName) {
    DWORD ret = 0;

    do {
        ret = WaitForMultipleObjects(N, hEvents, FALSE, INFINITE);
		ret = ret - WAIT_OBJECT_0;
        if (ret == 0) return 0;
        if (ret == 1) { /* Tecla pressionada */
            printf("Thread %s Bloqueada\n", threadName);
            ret = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE); /* Aguarda pressionar tecla novamente */
            if (ret == WAIT_OBJECT_0) return 0;
            printf("Thread %s Desbloqueada\n", threadName);
        }
		if(ret >= N){
			printf("Error while waiting: %d\n", (int) GetLastError());
			return 0;
		}
    } while (!(ret > 1 && ret < N)); /* Volta a aguardar handle quando desbloqueada */

    return ret;
}
