
#include <conio.h>
#include <process.h>  // _beginthreadex() e _endthreadex()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>a

#include "CheckForError.h";

#define N_CLP_MENSAGENS 2

/* --------------------------------------- Definicoes Mensagens ----------------------------------------- */

#define DELIMITADOR_CAMPO ";"
#define DIAG_FALHA 55

#define MSG_TAM_TOT 40
#define NSEQ_TAM 5
#define ID_TAM 1
#define DIAG_TAM 2
#define PRES_INTERNA_TAM 6
#define PRES_INJECAO_TAM 6
#define TEMP_TAM 6
#define TIMESTAMP_TAM 8

typedef struct {
    int nseq;               // N�mero sequencial da mensagem
    int id;                 // Identifica��o do CLP
    int diag;               // Diagn�stico dos cart�es do CLP
    float pressao_interna;  // Press�o interna na panela de gusa
    float pressao_injecao;  // Press�o de inje��o do nitrog�nio
    float temp;             // Temperatura na panela
    SYSTEMTIME timestamp;   // Horas da mensagem
} mensagem_t;

int nseq_msg = 0;
HANDLE hMutex_nseq_msg;

/* ------------------------------------ Estrutura para os eventos -------------------------------------- */

HANDLE ThreadsCLP[N_CLP_MENSAGENS + 1];
enum {
    INDEX_CLP_MENSAGENS = N_CLP_MENSAGENS - 1,
    INDEX_CLP_MONITORACAO,
    INDEX_RETIRADA
};

/* --------------------------------------- Definicoes de casting ----------------------------------------- */

typedef unsigned(WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

/* ---------------------------------------  Declaracao das Funcoes ----------------------------------------- */
DWORD WINAPI Thread_CLP_Mensagens(int index);
DWORD WINAPI Thread_CLP_Monitoracao();
DWORD WINAPI Thread_Retirada_Mensagens();

int encode_msg(mensagem_t* dados, char* msg);

/* --------------------------------------- Funcao Main ----------------------------------------- */

int main() {
    DWORD Retorno;
    DWORD ThreadID;

    hMutex_nseq_msg = CreateMutex(NULL, FALSE, "NSEQ_MSG");
    CheckForError(hMutex_nseq_msg);

    for (int i = 0; i < N_CLP_MENSAGENS; i++) {
        ThreadsCLP[i] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Mensagens, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&ThreadID);
        if (ThreadsCLP[i] != (HANDLE)-1L) {
            printf("Thread Criada %d com sucesso, ID = %0x\n", i, ThreadID);
        } else {
            printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
            exit(0);
        }
    }

    ThreadsCLP[INDEX_CLP_MONITORACAO] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[INDEX_CLP_MONITORACAO] != (HANDLE)-1L) {
        printf("Thread Monitoracao Criada com sucesso, ID = %0x\n", ThreadID);
    } else {
        printf("Erro na criacao da thread Monitoracao! Codigo = %d\n", errno);
        exit(0);
    }

    ThreadsCLP[INDEX_RETIRADA] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[INDEX_RETIRADA] != (HANDLE)-1L) {
        printf("Thread Retirada Criada com sucesso, ID = %0x\n", ThreadID);
    } else {
        printf("Erro na criacao da thread Retirada! Codigo = %d\n", errno);
        exit(0);
    }

    Retorno = WaitForMultipleObjects(INDEX_RETIRADA + 1, ThreadsCLP, TRUE, INFINITE);  // Fun��o de espera do fechamento das threads
    if (Retorno != WAIT_OBJECT_0)                                                      
    {
        CheckForError(Retorno);
    }

    for (int i = 0; i < INDEX_RETIRADA + 1; i++) {
        CloseHandle(ThreadsCLP[i]);
    }
    printf("Finalizando Processos CLPs\n");
    Sleep(3000);
}

/* --------------------------------------- Threads ----------------------------------------- */

DWORD WINAPI Thread_CLP_Mensagens(int index) {
    srand(GetCurrentThreadId());
    DWORD Retorno;
    mensagem_t msg;
    char msg_str[MSG_TAM_TOT + 1];
    HANDLE h_bloq[2];
    HANDLE h_nseq[2];
    int evento_atual = -1;

    char BloqLeitura_Nome[9];
    snprintf(BloqLeitura_Nome, 9, "Leitura%d", index + 1);

    h_bloq[0] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (h_bloq[0] == NULL) {
        printf("ERROR : %d", GetLastError());
    }
    h_bloq[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, BloqLeitura_Nome);
    if (h_bloq[1] == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    h_nseq[0] = h_bloq[0];
    h_nseq[1] = hMutex_nseq_msg;

    do {
        Retorno = WaitForMultipleObjects(2, h_bloq, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        if (evento_atual == 0) {
            break;
        }

        Retorno  = WaitForMultipleObjects(2, h_nseq, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        if (evento_atual == 0) {
            break;
        }

        msg.nseq = nseq_msg++;
        if(nseq_msg > 99999) nseq_msg = 0;
        ReleaseMutex(hMutex_nseq_msg);

        msg.id = index + 1;
        msg.diag = rand() % 99;
        msg.pressao_interna = (msg.diag == 55) ? 0 : (rand() % 99999 / 10.0);
        msg.pressao_injecao = (msg.diag == 55) ? 0 : (rand() % 99999 / 10.0);
        msg.temp = (msg.diag == 55) ? 0 : (rand() % 99999 / 10.0);
        GetLocalTime(&msg.timestamp);

        encode_msg(&msg, msg_str);
        printf("Mensagem Gerada: %s\n", msg_str);

        Sleep(1000 + (rand() % 4000));

    } while (evento_atual != 0);

    CloseHandle(h_bloq[0]);
    CloseHandle(h_bloq[1]);
    printf("Thread Leitura%d foi finalizada\n", index + 1);
    return (0);
}

DWORD WINAPI Thread_CLP_Monitoracao() {
    DWORD Retorno;
    int evento_atual = -1;
    HANDLE eventos[2];
    eventos[0] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (eventos[0] == NULL) {
        printf("ERROR : %d", GetLastError());
    }
    eventos[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Monitoracao");
    if (eventos[1] == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    do {
        Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        printf("Monitoracao \n");
        if (evento_atual == 0) {
            break;
        }

    } while (evento_atual != 0);

    CloseHandle(eventos[0]);
    CloseHandle(eventos[1]);
    printf("Thread Monitoracao foi finalizada\n");
    return (0);
}

DWORD WINAPI Thread_Retirada_Mensagens() {
    DWORD Retorno;
    HANDLE eventos[2];
    int evento_atual = -1;

    eventos[0] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (eventos[0] == NULL) {
        printf("ERROR : %d", GetLastError());
    }
    eventos[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Retirada");
    if (eventos[1] == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    do {
        printf("Retirada Mensagens\n");
        Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        if (evento_atual == 0) {
            break;
        }

    } while (evento_atual != 0);

    CloseHandle(eventos[0]);
    CloseHandle(eventos[1]);
    printf("Thread Retirada foi finalizada\n");
    return (0);
}

/* --------------------------------------- Funcoes Auxiliares ----------------------------------------- */

int encode_msg(mensagem_t* dados, char* msg) {
    char string_formato[20], string_dado[20];
    int pos_atual = 0;

    snprintf(string_formato, 20, "%%0%dd", NSEQ_TAM);
    snprintf(string_dado, 20, string_formato, dados->nseq);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, NSEQ_TAM);
    pos_atual += NSEQ_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%dd", ID_TAM);
    snprintf(string_dado, 20, string_formato, dados->id);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, ID_TAM);
    pos_atual += ID_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%dd", DIAG_TAM);
    snprintf(string_dado, 20, string_formato, dados->diag);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, DIAG_TAM);
    pos_atual += DIAG_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", PRES_INTERNA_TAM);
    snprintf(string_dado, 20, string_formato, dados->pressao_interna);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, PRES_INTERNA_TAM);
    pos_atual += PRES_INTERNA_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", PRES_INJECAO_TAM);
    snprintf(string_dado, 20, string_formato, dados->pressao_injecao);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, PRES_INJECAO_TAM);
    pos_atual += PRES_INJECAO_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", TEMP_TAM);
    snprintf(string_dado, 20, string_formato, dados->temp);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, TEMP_TAM);
    pos_atual += TEMP_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%02d:%%02d:%%02d");
    snprintf(string_dado, 20, string_formato, dados->timestamp.wHour, dados->timestamp.wMinute, dados->timestamp.wSecond);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, TIMESTAMP_TAM);
    pos_atual += TIMESTAMP_TAM;

    msg[pos_atual] = '\0';

    return 1;
}
