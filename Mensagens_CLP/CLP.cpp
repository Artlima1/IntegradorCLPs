
#include <windows.h>a
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 
#include <string.h>

#define N_CLP_MENSAGENS 2


/* ---------------------------------------  Declaracao das Funcoes ----------------------------------------- */
DWORD WINAPI Thread_CLP_Mensagens(int index);
DWORD WINAPI Thread_CLP_Monitoracao();
DWORD WINAPI Thread_Retirada_Mensagens();

int encode_msg(mensagem_t * dados, char * msg);
int pad_zeros(int n, int size, char * dest);

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
    int nseq; //N�mero sequencial da mensagem
	int id; //Identifica��o do CLP
	int diag; //Diagn�stico dos cart�es do CLP
	float pressao_interna; //Press�o interna na panela de gusa
	float pressao_injecao; //Press�o de inje��o do nitrog�nio
	float temp; //Temperatura na panela
	SYSTEMTIME timestamp; //Horas da mensagem
} mensagem_t;


/* ------------------------------------ Estrutura para os eventos -------------------------------------- */

HANDLE ThreadsCLP[N_CLP_MENSAGENS+1];
enum {
    INDEX_CLP_MENSAGENS = N_CLP_MENSAGENS-1,
    INDEX_CLP_MONITORACAO,
    INDEX_RETIRADA
};
HANDLE Tecla_Esc;

/* --------------------------------------- Definicoes de casting ----------------------------------------- */

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);	
typedef unsigned* CAST_LPDWORD;

/* --------------------------------------- Funcao Main ----------------------------------------- */

int main()
{
    DWORD Retorno;
    DWORD ThreadID;

    Tecla_Esc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	if (Tecla_Esc == NULL)
	{
		printf("ERROR : %d", GetLastError());
	}

    for(int i = 0; i < N_CLP_MENSAGENS; i++){
        ThreadsCLP[i] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Mensagens, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&ThreadID);
        if (ThreadsCLP[i] != (HANDLE)-1L)
        {
            printf("Thread Criada %d com sucesso, ID = %0x\n", i, ThreadID);
        }
        else
        {
            printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
            exit(0);
        }
    }

    ThreadsCLP[INDEX_CLP_MONITORACAO] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[INDEX_CLP_MONITORACAO] != (HANDLE)-1L)
    {
        printf("Thread Monitoracao Criada com sucesso, ID = %0x\n", ThreadID);
    }
    else
    {
        printf("Erro na criacao da thread Monitoracao! Codigo = %d\n", errno);
        exit(0);
    }

    ThreadsCLP[INDEX_RETIRADA] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[INDEX_RETIRADA] != (HANDLE)-1L)
    {
        printf("Thread Retirada Criada com sucesso, ID = %0x\n", ThreadID);
    }
    else
    {
        printf("Erro na criacao da thread Retirada! Codigo = %d\n", errno);
        exit(0);
    }

    Retorno = WaitForMultipleObjects(INDEX_RETIRADA+1, ThreadsCLP, TRUE, INFINITE); //Fun��o de espera do fechamento das threads
	if (Retorno != WAIT_OBJECT_0)//COLOCAR O CHECKFORERROR*****************
	{
		CheckForError(Retorno);
	}


    for(int i=0; i<INDEX_RETIRADA+1; i++)
    {
        CloseHandle(ThreadsCLP[i]);
    }
    CloseHandle(Tecla_Esc);
    printf("Finalizando Processos CLPs\n");
    Sleep(3000);
    

}

/* --------------------------------------- Threads ----------------------------------------- */

DWORD WINAPI Thread_CLP_Mensagens(int index)
{
    DWORD Retorno;
    HANDLE eventos[2]; 
    int evento_atual = -1;

    char BloqLeitura_Nome[9];
    snprintf(BloqLeitura_Nome, 9, "Leitura%d", index+1);

    eventos[0] = Tecla_Esc;
    eventos[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, BloqLeitura_Nome);
    
	if (eventos[0] == NULL) {
		printf("ERROR : %d", GetLastError());
	}

    do
    {
		printf("CLP Leitura %d\n", index);
        Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        if (evento_atual == 0)
        {
            break;
        }

	}while (evento_atual != 0);
    

    CloseHandle(eventos);
    printf("Thread Leitura%d foi finalizada\n", index + 1);
    return(0);
}

DWORD WINAPI Thread_CLP_Monitoracao(){
    DWORD Retorno;
    int evento_atual = -1;
    HANDLE eventos[2];
    eventos[0] = Tecla_Esc;
    eventos[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Monitoracao");
	if (eventos[0] == NULL) 
    {
		printf("ERROR : %d", GetLastError());
	}


   do
	{
        Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        printf("Monitoracao \n");
        if (evento_atual == 0)
        {
            break;
        }

	}while (evento_atual != 0);

    CloseHandle(eventos);
    printf("Thread Monitoracao foi finalizada\n");
    return(0);
}

DWORD WINAPI Thread_Retirada_Mensagens(){
    DWORD Retorno;
    HANDLE eventos[2];
    int evento_atual = -1;
    eventos[0] = Tecla_Esc;
    eventos[1] =  OpenEvent(EVENT_ALL_ACCESS, FALSE, "Retirada");
	if (eventos[1] == NULL) 
    {
		printf("ERROR : %d", GetLastError());
	}

   do
	{
        printf("Retirada Mensagens\n");
        Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
        evento_atual = Retorno - WAIT_OBJECT_0;
        if (evento_atual == 0)
        {
   
            break;
        }

   } while (evento_atual != 0);

  
    CloseHandle(eventos);
    printf("Thread Retirada foi finalizada\n");
    return(0);
}

/* --------------------------------------- Funcoes Auxiliares ----------------------------------------- */

int encode_msg(mensagem_t * dados, char * msg){
    char string_formato[20], string_dado[20];
    int pos_atual=0;

    snprintf(string_formato, 20, "%%0%dd", NSEQ_TAM);
    snprintf(string_dado, 20, string_formato, dados->nseq);
    strncpy(&msg[pos_atual], string_dado, NSEQ_TAM);
    pos_atual+= NSEQ_TAM;
    strncpy(&msg[pos_atual], DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%dd", ID_TAM);
    snprintf(string_dado, 20, string_formato, dados->nseq);
    strncpy(&msg[pos_atual], string_dado, ID_TAM);
    pos_atual+= ID_TAM;
    strncpy(&msg[pos_atual], DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%dd", DIAG_TAM);
    snprintf(string_dado, 20, string_formato, dados->diag);
    strncpy(&msg[pos_atual], string_dado, DIAG_TAM);
    pos_atual+= DIAG_TAM;
    strncpy(&msg[pos_atual], DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", PRES_INTERNA_TAM);
    snprintf(string_dado, 20, string_formato, dados->pressao_interna);
    strncpy(&msg[pos_atual], string_dado, PRES_INTERNA_TAM);
    pos_atual+= PRES_INTERNA_TAM;
    strncpy(&msg[pos_atual], DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", PRES_INJECAO_TAM);
    snprintf(string_dado, 20, string_formato, dados->pressao_injecao);
    strncpy(&msg[pos_atual], string_dado, PRES_INJECAO_TAM);
    pos_atual+= PRES_INJECAO_TAM;
    strncpy(&msg[pos_atual], DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%%0%d.1f", TEMP_TAM);
    snprintf(string_dado, 20, string_formato, dados->temp);
    strncpy(&msg[pos_atual], string_dado, TEMP_TAM);
    pos_atual+= TEMP_TAM;
    strncpy(&msg[pos_atual], DELIMITADOR_CAMPO, 1);
    pos_atual++;

    snprintf(string_formato, 20, "%02d:%02d:%02d");
    snprintf(string_dado, 20, string_formato, dados->timestamp.wHour, dados->timestamp.wMinute, dados->timestamp.wSecond);
    strncpy(&msg[pos_atual], string_dado, TIMESTAMP_TAM);
    pos_atual+= TIMESTAMP_TAM;

    return 1;
}
