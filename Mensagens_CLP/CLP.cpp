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
#include <ostream>

using namespace std;
#include "CheckForError.h";

#define N_CLP_MENSAGENS 2

/* --------------------------------------- Definicoes Mensagens ----------------------------------------- */

#define DELIMITADOR_CAMPO ";"
#define NSEQ_MAX 99999

#define DIAG_FALHA 55
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
#define N_ALARMES 5

#define MSG_LIMITE 100

/* --------------------- Struct para mensagem de dados ----------------------------- */
typedef struct {
    int nseq;               // N mero sequencial da mensagem
    int id;                 // Identifica  o do CLP
    int diag;               // Diagn stico dos cart es do CLP
    float pressao_interna;  // Press o interna na panela de gusa
    float pressao_injecao;  // Press o de inje  o do nitrog nio
    float temp;             // Temperatura na panela
    SYSTEMTIME timestamp;   // Horas da mensagem
} mensagem_t;

int nseq_msg = 0;
int bloqueio = 0;
HANDLE hMutex_nseq_msg;
LPSTR tempo;

typedef struct {
    int nseq;                   // N mero sequencial da mensagem
    char id[ALARME_ID_TAM + 1];   // Identifica  o do CLP
    SYSTEMTIME timestamp;       // Horas da mensagem
} alarme_t;

int nseq_alarme = 0;

typedef struct {
    char id[ALARME_ID_TAM + 1];
    char descricao[100];
} alarme_code_t;

alarme_code_t lista_alarmes[N_ALARMES] = {
    {"A2", "Esteira Parou"},
    {"T1", "Temperatura baixa para a reacao"},
    {"Q2", "Baixa pressao de injecao do nitrogenio"},
    {"C4", "Concentracao baixa de carbono no sistema"},
    {"C6", "Concentracao baixa de silicio no sistema"},
};
/* ----------------------- Estrutura Lista Circular de Mensagens -------------------------------------- */

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

lista_circular_t listaCLPs;
lista_multithread ListaMsg1;

/* ------------------------ Declarações para Maislot --------------------------*/
typedef struct {
    BOOL disponivel;
    HANDLE hMailSlot;
    HANDLE hSem;
} mailslot_alarmes_t;

/* ------------------------------------- Estrutura Timers -------------------------------------- */
#define MS_PARA_10NS 10000
#define STARTUP_TIME_TIMERS 2000 //ms

#define INTERVAL_CLPS_TIMERS 500 // ms

#define INTERVAL_MONIT_TIMER_MIN 1000 // ms
#define INTERVAL_MONIT_TIMER_MAX 5000 // ms
#define INTERVAL_MONIT_TIMER (INTERVAL_MONIT_TIMER_MIN + (rand() % INTERVAL_MONIT_TIMER_MAX))

HANDLE hTimerCLP[N_CLP_MENSAGENS];
HANDLE hTimerMonit;

/* ------------------------------------ Estrutura para os eventos -------------------------------------- */

HANDLE ThreadsCLP[N_CLP_MENSAGENS + 1];
enum {
    INDEX_CLP_MENSAGENS = N_CLP_MENSAGENS - 1,
    INDEX_CLP_MONITORACAO,
    INDEX_RETIRADA
};
HANDLE msg_diag55;
HANDLE hMsg;

/* --------------------------------------- Definicoes de casting ----------------------------------------- */

typedef unsigned(WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

/* ---------------------------------------  Declaracao das Funcoes ----------------------------------------- */
DWORD WINAPI Thread_CLP_Mensagens(int index);
DWORD WINAPI Thread_CLP_Monitoracao();
DWORD WINAPI Thread_Retirada_Mensagens();

int encode_msg(mensagem_t* dados, char* msg);
int encode_alarme(alarme_t* dados, char* alarme);
int decode_msg(char* msg, mensagem_t* dados);
int decode_alarme(char* msg, alarme_t* dados);
DWORD wait_with_unbloqued_check(HANDLE* hEvents, char* threadName);
BOOL abrir_lista2(HANDLE * hSection, lista_multithread * Lista, HANDLE * outMut, HANDLE * outSem);
BOOL abrir_mailslot(mailslot_alarmes_t * MS, char * path, char * semName);

/* --------------------------------------- Funcao Main ----------------------------------------- */

int main() {
    DWORD Retorno;
    DWORD ThreadID;
    LARGE_INTEGER PresetTimerCLP, PresetTimerMonit;
    BOOL bRet;

    hMutex_nseq_msg = CreateMutex(NULL, FALSE, "MUTEX_NSEQ_MSG");
    CheckForError(hMutex_nseq_msg);

    ListaMsg1.hMutex = CreateMutex(NULL, FALSE, "MUTEX_FILA1");
    CheckForError(ListaMsg1.hMutex);

    ListaMsg1.hSemConsumir = CreateSemaphore(NULL, 0, MSG_LIMITE, "SEM_CONSUMIR_FILA1");
    CheckForError(ListaMsg1.hSemConsumir);

    ListaMsg1.hSemProduzir = CreateSemaphore(NULL, MSG_LIMITE, MSG_LIMITE, "SEM_PRODUZIR_FILA1");
    CheckForError(ListaMsg1.hSemProduzir);

    hMsg = CreateEvent(NULL, TRUE, FALSE, "msg");
    CheckForError(hMsg);

    ListaMsg1.lc = &listaCLPs;

    /* Criação dos CLPs de leitura */
    char sNomeTimer[15];
    for (int i = 0; i < N_CLP_MENSAGENS; i++) {

        /* Cria timer com reset autom�tico */
        snprintf(sNomeTimer, 15, "TimerCLP%d", i + 1);
        hTimerCLP[i] = CreateWaitableTimer(NULL, FALSE, sNomeTimer);
        CheckForError(hTimerCLP[i]);

        /* Cria Thread */
        ThreadsCLP[i] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Mensagens, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&ThreadID);
        if (ThreadsCLP[i] != (HANDLE)-1L) {
            printf("Thread CLP%d Criada com sucesso, ID = %0x\n", i+1, ThreadID);
        } else {
            printf("Erro na criacao da thread CLP%d Codigo = %d\n", i+1, errno);
            exit(0);
        }

        /* Dispara timer periodico após tempo de startup */
        PresetTimerCLP.QuadPart = -(STARTUP_TIME_TIMERS * MS_PARA_10NS);
        bRet = SetWaitableTimer(hTimerCLP[i], &PresetTimerCLP, INTERVAL_CLPS_TIMERS, NULL, NULL, FALSE);
        CheckForError(bRet);
    }

    /* Criacao do CLP de monitoracao */
    hTimerMonit = CreateWaitableTimer(NULL, FALSE, "TimerMonitoracao");
    CheckForError(hTimerMonit);
    ThreadsCLP[INDEX_CLP_MONITORACAO] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[INDEX_CLP_MONITORACAO] != (HANDLE)-1L) {
        printf("Thread Monitoracao Criada com sucesso, ID = %0x\n", ThreadID);
    } else {
        printf("Erro na criacao da ThreadMonitoracao! Codigo = %d\n", errno);
        exit(0);
    }
    /* Dispara timer de disparo único após tempo de startup */
    PresetTimerMonit.QuadPart = -(STARTUP_TIME_TIMERS * MS_PARA_10NS);
    bRet = SetWaitableTimer(hTimerMonit, &PresetTimerMonit, 0, NULL, NULL, FALSE);
    CheckForError(bRet);

    /* Criacao da thread retirada de mensagens */
    ThreadsCLP[INDEX_RETIRADA] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Retirada_Mensagens, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[INDEX_RETIRADA] != (HANDLE)-1L) {
        printf("Thread Retirada Criada com sucesso, ID = %0x\n", ThreadID);
    }
    else {
        printf("Erro na criacao da thread Retirada! Codigo = %d\n", errno);
        exit(0);
    }

    /* Espera do fechamento das threads */
    Retorno = WaitForMultipleObjects(INDEX_RETIRADA + 1, ThreadsCLP, TRUE, INFINITE); 
    if (Retorno != WAIT_OBJECT_0)                                                      
    {
        CheckForError(Retorno);
    }

    for (int i = 0; i < INDEX_RETIRADA + 1; i++) {
        CloseHandle(ThreadsCLP[i]);
    }
    for(int i = 0; i < N_CLP_MENSAGENS; i++){
        CloseHandle(hTimerCLP[i]);
    }
    CloseHandle(hTimerMonit);

    CloseHandle(hMutex_nseq_msg);
    CloseHandle(ListaMsg1.hMutex);
    CloseHandle(ListaMsg1.hSemProduzir);
    CloseHandle(ListaMsg1.hSemConsumir);
    CloseHandle(hMsg);

    printf("Processo CPLs encerrado\n");
}

/* --------------------------------------- Threads ----------------------------------------- */

DWORD WINAPI Thread_CLP_Mensagens(int index) {
    srand(GetCurrentThreadId());
    DWORD ret;
    mensagem_t msg;
    char msg_str[MSG_TAM_TOT + MSG_TAM_TOT];

    HANDLE hEsc, hSwitch;
    HANDLE hTimer[3];
    HANDLE hNSeq[3];
    HANDLE hListaMsg[3];
    HANDLE hProduzir[3];
    HANDLE msg_diag55;

    char sNomeThread[9];
    snprintf(sNomeThread, 9, "Leitura%d", index + 1);

    hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (hEsc == NULL) {
        printf("ERROR : %d", GetLastError());
    }
    hSwitch = OpenEvent(EVENT_ALL_ACCESS, FALSE, sNomeThread);
    if (hSwitch == NULL) {
        printf("ERROR : %d", GetLastError());
    }


    hTimer[0] = hEsc;
    hTimer[1] = hSwitch;
    hTimer[2] = hTimerCLP[index];
    
    hNSeq[0] = hEsc;
    hNSeq[1] = hSwitch;
    hNSeq[2] = hMutex_nseq_msg;

    hListaMsg[0] = hEsc;
    hListaMsg[1] = hSwitch;
    hListaMsg[2] = ListaMsg1.hMutex;

    hProduzir[0] = hEsc;
    hProduzir[1] = hSwitch;
    hProduzir[2] = ListaMsg1.hSemProduzir;

    while(1) {
        /* Aguarda Temporizacao */
        ret = wait_with_unbloqued_check(hTimer, sNomeThread);
        if(ret != 0) break; /* Esc pressionado */

        /* Verificar se há espaço disponivel na fila */
        ret = wait_with_unbloqued_check(hListaMsg, sNomeThread); /* Aguarda estar desbloqueado e ter o mutex da fila */
        if (ret != 0) break; /* Esc pressionado */
        if (ListaMsg1.lc->total == MSG_LIMITE) {
            printf("Lista 1 cheia\n");
        }
        ReleaseMutex(ListaMsg1.hMutex);
        ret = wait_with_unbloqued_check(hProduzir, sNomeThread); /* Aguarda estar desbloqueado e semáforo de producao */
        if (ret != 0) break; /* Esc pressionado */

        /* Produzir dados da mensagem */
        ret = wait_with_unbloqued_check(hNSeq, sNomeThread); /* Aguarda estar desbloqueado e ter o mutex do nSeq */
        if (ret != 0) break; /* Esc pressionado */

        msg.nseq = nseq_msg++;
        if (nseq_msg > NSEQ_MAX) nseq_msg = 0;
        ReleaseMutex(hMutex_nseq_msg);

        msg.id = index + 1;
        msg.diag = rand() % 99;
        msg.pressao_interna = (msg.diag == 55) ? 0 : (rand() % 99999 / 10.0);
        msg.pressao_injecao = (msg.diag == 55) ? 0 : (rand() % 99999 / 10.0);
        msg.temp = (msg.diag == 55) ? 0 : (rand() % 99999 / 10.0);
        GetLocalTime(&msg.timestamp);

        /* Transformar dados em string separada por ; */
        encode_msg(&msg, msg_str);
        printf("Mensagem Gerada: %s\n", msg_str);

        /* Inserir na Fila de Mensagens e notificar mensagem a ser consumida */
        ret = wait_with_unbloqued_check(hListaMsg, sNomeThread); /* Aguarda estar desbloqueado e ter o mutex da fila */
        if (ret != 0) break; /* Esc pressionado */
        ListaMsg1.lc->total++;
        memcpy(ListaMsg1.lc->lista[ListaMsg1.lc->pos_livre], msg_str, MSG_TAM_TOT);
        ListaMsg1.lc->pos_livre = (ListaMsg1.lc->pos_livre + 1) % MSG_LIMITE;
        ReleaseMutex(ListaMsg1.hMutex);
        ReleaseSemaphore(ListaMsg1.hSemConsumir, 1, NULL);
    }

    CloseHandle(hEsc);
    CloseHandle(hSwitch);



    printf("Thread Leitura%d foi finalizada\n", index + 1);
    return (0);
}

DWORD WINAPI Thread_CLP_Monitoracao()
{
    srand(GetCurrentThreadId());
    HANDLE hMS;
    DWORD ret;
    BOOL ms_envio;
    DWORD bytes = 0;
    HANDLE hEsc, hSwitch, hTimer[3];
    char sAlarme[ALARME_TAM_TOT + ALARME_TAM_TOT];
    char sNomeThread[] = "Thread de Monitoracao";
    alarme_t alarme;
    LARGE_INTEGER PresetTimerMonit;
    mailslot_alarmes_t MS;


    hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (hEsc == NULL)
    {
        printf("ERROR : %d", GetLastError());
    }
    hSwitch = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Monitoracao");
    if (hSwitch == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    hTimer[0] = hEsc;
    hTimer[1] = hSwitch;
    hTimer[2] = hTimerMonit;

    MS.disponivel = abrir_mailslot(&MS, "\\\\.\\mailslot\\ms_alarmes_criticos", "SEM_ALARME_CRITICO");

    while (1) {

        /* Caso mailsot estiver indisponível, tenta abri-lo */ 
        if(MS.disponivel == FALSE){
            MS.disponivel = abrir_mailslot(&MS, "\\\\.\\mailslot\\ms_alarmes_criticos", "SEM_ALARME_CRITICO");
        }

        /* Aguarda Temporizacao */
        ret = wait_with_unbloqued_check(hTimer, sNomeThread);
        if (ret != 0) break; /* Esc pressionado */

        alarme.nseq = nseq_alarme++;
        if (nseq_msg > NSEQ_MAX) nseq_alarme = 0;

        strcpy_s(alarme.id, ALARME_ID_TAM + 1, lista_alarmes[rand() % N_ALARMES].id);
        GetLocalTime(&alarme.timestamp);

        encode_alarme(&alarme, sAlarme);
        printf("Alarme Gerado: %s\n", sAlarme);

        /* Envia alarme se mailslot disponivel */
        if(MS.disponivel == TRUE){
            ms_envio = WriteFile(MS.hMailSlot, sAlarme, ALARME_TAM_TOT, &bytes, NULL);
            CheckForError(ms_envio);
            ret = ReleaseSemaphore(MS.hSem, 1, NULL);
        }

        /* Dispara timer de disparo único após tempo aleatório */
        PresetTimerMonit.QuadPart = -(INTERVAL_MONIT_TIMER * MS_PARA_10NS);
        ret = SetWaitableTimer(hTimerMonit, &PresetTimerMonit, 0, NULL, NULL, FALSE);
        CheckForError(ret);
    }

    CloseHandle(hEsc);
    CloseHandle(hSwitch);
    if(MS.disponivel == TRUE){
        CloseHandle(MS.hMailSlot);
        CloseHandle(MS.hSem);
    }

    printf("Thread Monitoracao de alarmes foi finalizada\n");
    return (0);
}

DWORD WINAPI Thread_Retirada_Mensagens() {
    DWORD ret;
    DWORD bytes;
    HANDLE hEsc, hSwitchRetirada, hListaMsg1[3], hListaMsg2[3], hConsumir[3], hProduzir[3];
    BOOL Lista2Disponivel;
    lista_multithread ListaMsg2;
    HANDLE hSection;
    mailslot_alarmes_t MS;
    LONG prev;

    char sMsg[MSG_TAM_TOT + 1];
    mensagem_t msg_data;
    char sNomeThread[] = "Thread de Retirada de Mensagens";
    BOOL ms_envio;

    hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (hEsc == NULL) {
        printf("ERROR : %d", GetLastError());
    }
    hSwitchRetirada = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Retirada");
    if (hSwitchRetirada == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    hListaMsg1[0] = hEsc;
    hListaMsg1[1] = hSwitchRetirada;
    hListaMsg1[2] = ListaMsg1.hMutex;

    hConsumir[0] = hEsc;
    hConsumir[1] = hSwitchRetirada;
    hConsumir[2] = ListaMsg1.hSemConsumir;

    hListaMsg2[0] = hEsc;
    hListaMsg2[1] = hSwitchRetirada;

    hProduzir[0] = hEsc;
    hProduzir[1] = hSwitchRetirada;

    printf("Thread Retirada Inicializada\n");

    MS.disponivel = abrir_mailslot(&MS, "\\\\.\\mailslot\\ms_alarmes_cartoes", "SEM_ALARME_CARTOES");
    Lista2Disponivel = abrir_lista2(&hSection, &ListaMsg2, &hListaMsg2[2], &hProduzir[2]);

    while (1) 
    {
        /* Caso lista 2 estiver indisponível, tenta abri-la */
        if(Lista2Disponivel == FALSE){
            Lista2Disponivel = abrir_lista2(&hSection, &ListaMsg2, &hListaMsg2[2], &hProduzir[2]);
        }

        /* Caso mailsot estiver indisponível, tenta abri-lo */ 
        if(MS.disponivel == FALSE){
            MS.disponivel = abrir_mailslot(&MS, "\\\\.\\mailslot\\ms_alarmes_cartoes", "SEM_ALARME_CARTOES");
        }

        /* Se lista 2 disponivel, Verifica se há espaço  */
        if(Lista2Disponivel == TRUE){
            ret = wait_with_unbloqued_check(hListaMsg2, sNomeThread); /* Aguarda estar desbloqueado e com Mutex da lista2 */
            if (ret != 0) break; /* Esc pressionado */
            if(ListaMsg2.lc->total == MSG_LIMITE){
                printf("Lista 2 cheia\n");
            }
            ReleaseMutex(ListaMsg2.hMutex);
            ret = wait_with_unbloqued_check(hProduzir, sNomeThread); /* Aguarda estar desbloqueado e semáforo de producao */
            if (ret != 0) break; /* Esc pressionado */
        }

        /* Aguarda haver mensagem a ser consumida */
        ret = wait_with_unbloqued_check(hConsumir, sNomeThread); /* Aguarda estar desbloqueado e semáforo de consumo */
        if (ret != 0) break; /* Esc pressionado */

        ret = wait_with_unbloqued_check(hListaMsg1, sNomeThread); /* Aguarda estar desbloqueado e semáforo de consumo */
        if (ret != 0) break; /* Esc pressionado */
        memcpy(sMsg, ListaMsg1.lc->lista[ListaMsg1.lc->pos_ocupada], MSG_TAM_TOT);
        ListaMsg1.lc->pos_ocupada = (ListaMsg1.lc->pos_ocupada + 1) % MSG_LIMITE;
        ListaMsg1.lc->total--;
        ReleaseMutex(ListaMsg1.hMutex);

        /* Libera producao */
        ReleaseSemaphore(ListaMsg1.hSemProduzir, 1, NULL);

        /* Trata mensagem */
        decode_msg(sMsg, &msg_data);
        if (msg_data.diag == 55) {
            ReleaseSemaphore(ListaMsg2.hSemProduzir, 1, NULL);
            if(MS.disponivel == TRUE){
                ms_envio = WriteFile(MS.hMailSlot, sMsg, MSG_TAM_TOT, &bytes, NULL);
                CheckForError(ms_envio);
                ReleaseSemaphore(MS.hSem, 1, &prev);
            }
        }
        else{
            /* Se lista 2 disponivel, deposita mensagem  */
            if(Lista2Disponivel == TRUE){
                wait_with_unbloqued_check(hListaMsg2, sNomeThread); /* Aguarda estar desbloqueado e com Mutex da lista2 */
                if (ret != 0) break; /* Esc pressionado */
                memcpy(ListaMsg2.lc->lista[ListaMsg2.lc->pos_livre], sMsg, MSG_TAM_TOT);
                ListaMsg2.lc->pos_livre = (ListaMsg2.lc->pos_livre + 1) % MSG_LIMITE;
                ListaMsg2.lc->total++;
                ReleaseMutex(ListaMsg2.hMutex);
                ReleaseSemaphore(ListaMsg2.hSemConsumir, 1, NULL);
            }
        }
    }

    // Elimina mapeamento
    if(Lista2Disponivel == TRUE){
        CloseHandle(ListaMsg2.hMutex);
        CloseHandle(ListaMsg2.hSemConsumir);
        CloseHandle(ListaMsg2.hSemProduzir);
        ret=UnmapViewOfFile(ListaMsg2.lc);
        CheckForError(ret);
    }
    if(MS.disponivel == TRUE){
        CloseHandle(MS.hMailSlot);
        CloseHandle(MS.hSem);
    }
    CloseHandle(hEsc);
    CloseHandle(hSwitchRetirada);
    printf("Thread Retirada foi finalizada\n");
    return (0);
}

/* --------------------------------------- Funcoes Auxiliares ----------------------------------------- */

int encode_msg(mensagem_t* dados, char* msg) {
    char string_formato[20], string_dado[20];
    int pos_atual = 0;

    snprintf(string_formato, 20, "%%0%dd", MSG_NSEQ_TAM);
    snprintf(string_dado, 20, string_formato, dados->nseq);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_NSEQ_TAM);
    pos_atual += MSG_NSEQ_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%dd", MSG_ID_TAM);
    snprintf(string_dado, 20, string_formato, dados->id);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_ID_TAM);
    pos_atual += MSG_ID_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%dd", MSG_DIAG_TAM);
    snprintf(string_dado, 20, string_formato, dados->diag);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_DIAG_TAM);
    pos_atual += MSG_DIAG_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", MSG_PRES_INTERNA_TAM);
    snprintf(string_dado, 20, string_formato, dados->pressao_interna);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_PRES_INTERNA_TAM);
    pos_atual += MSG_PRES_INTERNA_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", MSG_PRES_INJECAO_TAM);
    snprintf(string_dado, 20, string_formato, dados->pressao_injecao);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_PRES_INJECAO_TAM);
    pos_atual += MSG_PRES_INJECAO_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", MSG_TEMP_TAM);
    snprintf(string_dado, 20, string_formato, dados->temp);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_TEMP_TAM);
    pos_atual += MSG_TEMP_TAM;
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%02d:%%02d:%%02d");
    snprintf(string_dado, 20, string_formato, dados->timestamp.wHour, dados->timestamp.wMinute, dados->timestamp.wSecond);
    strncpy_s(&msg[pos_atual], MSG_TAM_TOT + 1, string_dado, MSG_TIMESTAMP_TAM);
    pos_atual += MSG_TIMESTAMP_TAM;

    msg[pos_atual] = '\0';

    return 1;
}

int encode_alarme(alarme_t* dados, char* alarme) {
    char string_formato[20], string_dado[20];
    int pos_atual = 0;
 
    
    snprintf(string_formato, 20, "%%0%dd", ALARME_NSEQ_TAM);
    snprintf(string_dado, 20, string_formato, dados->nseq);
    strncpy_s(&alarme[pos_atual], ALARME_TAM_TOT + 1, string_dado, ALARME_NSEQ_TAM);
    pos_atual += ALARME_NSEQ_TAM;
    strncpy_s(&alarme[pos_atual], ALARME_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;

    strncpy_s(&alarme[pos_atual], ALARME_TAM_TOT + 1, dados->id, ALARME_ID_TAM);
    pos_atual += ALARME_ID_TAM;
    strncpy_s(&alarme[pos_atual], ALARME_TAM_TOT + 1, DELIMITADOR_CAMPO, 1);
    pos_atual++;
    
    snprintf(string_formato, 20, "%%02d:%%02d:%%02d");
    snprintf(string_dado, 20, string_formato, dados->timestamp.wHour, dados->timestamp.wMinute, dados->timestamp.wSecond);
    strncpy_s(&alarme[pos_atual], ALARME_TAM_TOT + 1, string_dado, ALARME_TIMESTAMP_TAM);
    
    return 1;
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

BOOL abrir_lista2(HANDLE * hSection, lista_multithread * Lista, HANDLE * outMut, HANDLE * outSem){
    BOOL ret = FALSE;
    *hSection = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "LISTA_CIRCULAR_2");
    if(*hSection != NULL){
        Lista->lc = (lista_circular_t *) MapViewOfFile(*hSection, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(lista_circular_t));
        Lista->hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, "MUTEX_FILA2");
        CheckForError(Lista->hMutex);
        Lista->hSemConsumir = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, TRUE, "SEM_CONSUMIR_FILA2");
        CheckForError(Lista->hSemConsumir);
        Lista->hSemProduzir = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, TRUE, "SEM_PRODUZIR_FILA2");
        CheckForError(Lista->hSemProduzir);
        *outMut = Lista->hMutex;
        *outSem = Lista->hSemProduzir;

        ret = (
            (*hSection != NULL) &&
            (Lista->lc != NULL) &&
            (Lista->hMutex != NULL) &&
            (Lista->hSemConsumir != NULL) &&
            (Lista->hSemProduzir != NULL)
        );
    }

    return ret;
}

BOOL abrir_mailslot(mailslot_alarmes_t * MS, char * path, char * semName){
    DWORD ret = FALSE;
    DWORD LastError;

    MS->hMailSlot = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (MS->hMailSlot != INVALID_HANDLE_VALUE) {
        MS->hSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, TRUE, semName);
        ret = (MS->hSem != NULL);
    }

    if(ret == TRUE){
        printf("Mailslot aberto!\n");
    }

    return ret;
}

