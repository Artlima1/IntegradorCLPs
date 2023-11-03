
#include <windows.h>a
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 
#include <string.h>

#define N_CLP_MENSAGENS 2

HANDLE ThreadsCLP[N_CLP_MENSAGENS+1];
DWORD WINAPI Thread_CLP_Mensagens(int index);
DWORD WINAPI Thread_CLP_Monitoracao();
DWORD WINAPI Thread_Retirada_Mensagens();

int Retorno = 0;

enum {
    INDEX_CLP_MENSAGENS = N_CLP_MENSAGENS,
    INDEX_CLP_Monitoracao,

}

int main()
{
    DWORD Retorno;

    DWORD ThreadID;
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

    ThreadsCLP[N_CLP_MENSAGENS] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[N_CLP_MENSAGENS] != (HANDLE)-1L)
    {
        printf("Thread Criada %d com sucesso, ID = %0x\n", i, ThreadID);
    }
    else
    {
        printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
        exit(0);
    }

    ThreadsCLP[N_CLP_MENSAGENS+1] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_CLP_Monitoracao, NULL, 0, (CAST_LPDWORD)&ThreadID);
    if (ThreadsCLP[N_CLP_MENSAGENS] != (HANDLE)-1L)
    {
        printf("Thread Criada %d com sucesso, ID = %0x\n", i, ThreadID);
    }
    else
    {
        printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
        exit(0);
    }

    Retorno = WaitForMultipleObjects(N_CLP_MENSAGENS+2, ThreadsCLP, TRUE, INFINITE); //Fun??o de espera do fechamento das threads
	if (Retorno != WAIT_OBJECT_0)//COLOCAR O CHECKFORERROR*****************
	{
		CheckForError(Retorno);
	}

    for(int i=0; i<N_CLP_MENSAGENS+1; i++){
        CloseHandle(ThreadsCLP[i]);
    }


}

DWORD WINAPI Thread_CLP_Mensagens(int index){
    DWORD Retorno;

    char BloqLeitura_Nome[8];
    sprintf(EventStopName,"Leitura%d", index+1);

    HANDLE Bloq_Leitura = OpenEvent(EVENT_ALL_ACCESS, FALSE, BloqLeitura_Nome);
	if (Bloq_Leitura == NULL) {
		printf("ERROR : %d", GetLastError());
	}

    while (1)
	{
		printf("CLP Leitura %d\n", index);
		Retorno = WaitForSingleObject(Bloq_Leitura, INFINITE);
		if (Retorno != WAIT_OBJECT_0)
		{
			CheckForError(Retorno);
		}
		Sleep(100);
	}

    CloseHandle(Bloq_Leitura);
}

DWORD WINAPI Thread_CLP_Monitoracao(){
    DWORD Retorno;

    HANDLE Bloq_Monitoracao = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Monitoracao");
	if (Bloq_Monitoracao == NULL) {
		printf("ERROR : %d", GetLastError());
	}

    while (1)
	{
		printf("CLP Monitoracao\n");
		Retorno = WaitForSingleObject(Bloq_Monitoracao, INFINITE);
		if (Retorno != WAIT_OBJECT_0)
		{
			CheckForError(Retorno);
		}
		Sleep(100);
	}

    CloseHandle(Bloq_Monitoracao);
}

DWORD WINAPI Thread_Retirada_Mensagens(){
    DWORD Retorno;

    HANDLE Bloq_Retirada = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Retirada");
	if (Bloq_Retirada == NULL) {
		printf("ERROR : %d", GetLastError());
	}

    while (1)
	{
		printf("Retirada de Mensagens\n");
		Retorno = WaitForSingleObject(Bloq_Retirada, INFINITE);
		if (Retorno != WAIT_OBJECT_0)
		{
			CheckForError(Retorno);
		}
		Sleep(100);
	}

    CloseHandle(Bloq_Retirada);
}

