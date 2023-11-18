
#include <conio.h>
#include <process.h>  // _beginthreadex() e _endthreadex()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

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
    int nseq;               // N�mero sequencial da mensagem
    int id;                 // Identifica��o do CLP
    int diag;               // Diagn�stico dos cart�es do CLP
    float pressao_interna;  // Press�o interna na panela de gusa
    float pressao_injecao;  // Press�o de inje��o do nitrog�nio
    float temp;             // Temperatura na panela
    SYSTEMTIME timestamp;   // Horas da mensagem
} mensagem_t;

int nseq_msg = 0;
int bloqueio = 0;
HANDLE hMutex_nseq_msg;

typedef struct {
    int nseq;                   // N�mero sequencial da mensagem
    char id[ALARME_ID_TAM+1];   // Identifica��o do CLP
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

typedef struct {char msg[MSG_TAM_TOT+1];} msg_na_fila_t;

msg_na_fila_t fila_msg[MSG_LIMITE];  // Lista Circular
int pos_livre=0; int pos_ocupada=0;	// Contadores para lista circular
HANDLE hMutexFilaMsg;
HANDLE hSemConsumirMsg;
HANDLE hSemProduzirMsg;
int total_mensagem = 0;

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
int encode_alarme(alarme_t* dados, char* alarme);
int decode_msg(char * msg, mensagem_t * dados);
int decode_alarme(char * msg, alarme_t * dados);
DWORD wait_with_unbloqued_check(HANDLE * hEvents, char * threadName);

/* --------------------------------------- Funcao Main ----------------------------------------- */

int main() {
    DWORD Retorno;
    DWORD ThreadID;

    hMutex_nseq_msg = CreateMutex(NULL, FALSE, "MUTEX_NSEQ_MSG");
    CheckForError(hMutex_nseq_msg);

    hMutexFilaMsg = CreateMutex(NULL, FALSE, "MUTEX_FILA");
    CheckForError(hMutex_nseq_msg);

    hSemConsumirMsg = CreateSemaphore(NULL, 0, MSG_LIMITE, "SEM_CONSUMIR");
    CheckForError(hSemConsumirMsg);

    hSemProduzirMsg = CreateSemaphore(NULL, MSG_LIMITE, MSG_LIMITE, "SEM_PRODUZIR");
    CheckForError(hSemProduzirMsg);


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

    ThreadsCLP[INDEX_RETIRADA] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Retirada_Mensagens, NULL, 0, (CAST_LPDWORD)&ThreadID);
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

    CloseHandle(hMutex_nseq_msg);
    CloseHandle(hMutexFilaMsg);
    CloseHandle(hSemConsumirMsg);
    CloseHandle(hSemProduzirMsg);

    printf("Finalizando Processos CLPs\n");
}

/* --------------------------------------- Threads ----------------------------------------- */

DWORD WINAPI Thread_CLP_Mensagens(int index) {
    srand(GetCurrentThreadId());
    DWORD ret;
    mensagem_t msg;
    char msg_str[MSG_TAM_TOT + MSG_TAM_TOT];
    HANDLE hEsc, hSwitch;
    HANDLE hNSeq[3];
    HANDLE hFila[3];
    HANDLE hProduzir[3];
    int evento_atual = -1;
    
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

    hNSeq[0] = hEsc;
    hNSeq[1] = hSwitch;
    hNSeq[2] = hMutex_nseq_msg;

    hFila[0] = hEsc;
    hFila[1] = hSwitch;
    hFila[2] = hMutexFilaMsg;

    hProduzir[0] = hEsc;
    hProduzir[1] = hSwitch;
    hProduzir[2] = hSemProduzirMsg;

    while(1) {
        /* Verificar se há espaço disponivel na fila */
        ret = wait_with_unbloqued_check(hFila, sNomeThread); /* Aguarda estar desbloqueado e ter o mutex da fila */
        if(ret != 0) break; /* Esc pressionado */
        if (total_mensagem == MSG_LIMITE){
            printf("Fila cheia\n");
        }
        ReleaseMutex(hMutexFilaMsg);
        ret = wait_with_unbloqued_check(hProduzir, sNomeThread); /* Aguarda estar desbloqueado e semáforo de producao */
        if(ret != 0) break; /* Esc pressionado */

        /* Produzir dados da mensagem */
        ret = wait_with_unbloqued_check(hNSeq, sNomeThread); /* Aguarda estar desbloqueado e ter o mutex do nSeq */
        if(ret != 0) break; /* Esc pressionado */

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
        ret = wait_with_unbloqued_check(hFila, sNomeThread); /* Aguarda estar desbloqueado e ter o mutex da fila */
        if(ret != 0) break; /* Esc pressionado */
        total_mensagem++;
        memcpy(&fila_msg[pos_livre].msg, msg_str, MSG_TAM_TOT);
        pos_livre = (pos_livre + 1) % MSG_LIMITE;
        ReleaseMutex(hMutexFilaMsg);
        ReleaseSemaphore(hSemConsumirMsg, 1, NULL);

        /* Dormir por 1 a 5 segundos (aleatorio) */
        Sleep(1000 + (rand() % 4000));

    }

    CloseHandle(hEsc);
    CloseHandle(hSwitch);

    printf("Thread Leitura%d foi finalizada\n", index + 1);
    return (0);
}

DWORD WINAPI Thread_CLP_Monitoracao() 
{
    srand(GetCurrentThreadId());
    DWORD ret;
    int evento_atual = -1;
    alarme_t alarme;
    char alarme_str[ALARME_TAM_TOT+ALARME_TAM_TOT];
    alarme_code_t exemplo_cod; /* RETIRAR DEPOIS */
    alarme_t exemplo_data; /* RETIRAR DEPOIS */
    
    HANDLE hBloq[2];
    hBloq[0] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (hBloq[0] == NULL) 
    {
        printf("ERROR : %d", GetLastError());
    }
    hBloq[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Monitoracao");
    if (hBloq[1] == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    while(1) {

        ret = WaitForMultipleObjects(2, hBloq, FALSE, 0);
		if (ret == WAIT_OBJECT_0) break;
		if(ret == WAIT_OBJECT_0 + 1){
			printf("Thread de monitoração Bloqueada\n");
			ret = WaitForMultipleObjects(2, hBloq, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0){
				break;
			}
			printf("Thread de monitoração Desbloqueada\n");
		}

        alarme.nseq = nseq_alarme++;
        if (nseq_msg > NSEQ_MAX) nseq_alarme = 0;

        strcpy_s(alarme.id, ALARME_ID_TAM + 1, lista_alarmes[rand() % N_ALARMES].id);
        GetLocalTime(&alarme.timestamp);

        encode_alarme(&alarme, alarme_str);
        printf("Alarme Gerado: %s\n", alarme_str);

        /* Exemplo decode RETIRAR DEPOIS*/
        decode_alarme(alarme_str, &exemplo_data);
        printf("Decoded: %d %s %d:%d:%d\n", exemplo_data.nseq, exemplo_data.id, exemplo_data.timestamp.wHour, exemplo_data.timestamp.wMinute, exemplo_data.timestamp.wSecond);
        for(int i=0; i<N_ALARMES; i++){
            if(strcmp(exemplo_data.id, lista_alarmes[i].id) == 0){
                memcpy(&exemplo_cod, &lista_alarmes[i], sizeof(alarme_code_t));
            }
        }
        printf("ALARME %s, %s\n", exemplo_cod.id, exemplo_cod.descricao);

        Sleep(1000 + (rand() % 4000));

    }

    CloseHandle(hBloq[0]);
    CloseHandle(hBloq[1]);
    printf("Thread Monitoracao foi finalizada\n");
    return (0);
}

DWORD WINAPI Thread_Retirada_Mensagens() {
    DWORD ret;
    HANDLE hEsc, hSwitchRetirada;
    HANDLE hFila[3];
    HANDLE hConsumir[3];
    msg_na_fila_t msg;
    mensagem_t msg_data;
    int evento_atual = -1;
    char sNomeThread[] = "RetiradaMensagens";

    hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
    if (hEsc == NULL) {
        printf("ERROR : %d", GetLastError());
    }
    hSwitchRetirada = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Retirada");
    if (hSwitchRetirada == NULL) {
        printf("ERROR : %d", GetLastError());
    }

    hFila[0] = hEsc;
    hFila[1] = hSwitchRetirada;
    hFila[2] = hMutexFilaMsg;

    hConsumir[0] = hEsc;
    hConsumir[1] = hSwitchRetirada;
    hConsumir[2] = hSemConsumirMsg;


    while(1) {
        /* Aguarda haver mensagem a ser consumida */
        ret = wait_with_unbloqued_check(hConsumir, sNomeThread); /* Aguarda estar desbloqueado e semáforo de consumo */
        if(ret != 0) break; /* Esc pressionado */
    
        ret = wait_with_unbloqued_check(hFila, sNomeThread); /* Aguarda estar desbloqueado e semáforo de consumo */
        if(ret != 0) break; /* Esc pressionado */
        memcpy(&msg.msg, &fila_msg[pos_ocupada].msg, MSG_TAM_TOT);
        pos_ocupada = (pos_ocupada + 1) % MSG_LIMITE;
        total_mensagem--;
        ReleaseMutex(hMutexFilaMsg);

        /* Trata mensagem */
        decode_msg(msg.msg, &msg_data);
        printf("Mensagem Retirada: %d %d %d %f %f %f %d %d %d\n",
            msg_data.nseq,
            msg_data.id,
            msg_data.diag,
            msg_data.pressao_interna,
            msg_data.pressao_injecao,
            msg_data.temp,
            msg_data.timestamp.wHour,
            msg_data.timestamp.wMinute,
            msg_data.timestamp.wSecond
        );

        /* Libera producao */
        ReleaseSemaphore(hSemProduzirMsg, 1, NULL);
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

int encode_alarme(alarme_t* dados, char* alarme){
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
    pos_atual += ALARME_TIMESTAMP_TAM;

    alarme[pos_atual] = '\0';

    return 1;
}

int decode_msg(char * msg, mensagem_t * dados){
    char string_dado[20];
    int pos_atual=0;
    
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

int decode_alarme(char * msg, alarme_t * dados){
    char string_dado[20];
    int pos_atual=0;
    
    strncpy_s(string_dado, 20, &msg[pos_atual], ALARME_NSEQ_TAM);
    string_dado[ALARME_NSEQ_TAM] = '\0';
    dados->nseq = atoi(string_dado);
    pos_atual += ALARME_NSEQ_TAM + 1;

    strncpy_s(string_dado, 20, &msg[pos_atual], ALARME_ID_TAM);
    string_dado[ALARME_ID_TAM] = '\0';
    strncpy_s(dados->id, ALARME_ID_TAM+1, &msg[pos_atual], ALARME_ID_TAM);
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

DWORD wait_with_unbloqued_check(HANDLE * hEvents, char * threadName){
    DWORD ret = 0;

    do{
        ret = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        if (ret == WAIT_OBJECT_0) break; /* Esc pressionado */
        if(ret == WAIT_OBJECT_0 + 1){ /* Tecla pressionada */
            printf("Thread %s Bloqueada\n", threadName);
            ret = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE); /* Aguarda pressionar tecla novamente */
            if (ret == WAIT_OBJECT_0){
                break;
            }
            printf("Thread %s Desbloqueada\n", threadName);
        }
    } while (ret != WAIT_OBJECT_0 + 2); /* Volta a aguardar handle quando desbloqueada */

    if(ret == WAIT_OBJECT_0 + 2){
        return 0;
    }
    else {
        return 1;
    }
}




