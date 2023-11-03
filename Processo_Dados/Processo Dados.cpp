
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 



typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);	// Casting para terceiro e sexto parâmetros da função
typedef unsigned* CAST_LPDWORD;
HANDLE Bloq_Dados;
HANDLE Tecla_Esc;
int Retorno = 0;
int evento_atual = -1;

HANDLE eventos[2];

struct mensagem_dados_processo
{
	//LIMITAR CARACTERES

	int nseq; //Número sequencial da mensagem
	int id; //Identificação do CLP
	int diag; //Diagnóstico dos cartões do CLP
	float pressao_interna; //Pressão interna na panela de gusa
	float pressao_injecao; //Pressão de injeção do nitrogênio
	float temp; //Temperatura na panela
	time_t timestamp; //Horas da mensagem

};



int main()
{
	Tecla_Esc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	if (Tecla_Esc == NULL)
	{
		printf("ERROR : %d", GetLastError());
	}

	Bloq_Dados = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Dados");
	if (Bloq_Dados == NULL) {
		printf("ERROR : %d", GetLastError());
	}
	eventos[0] = Tecla_Esc;
	eventos[1] = Bloq_Dados;
   	do
	{
		printf("Thread dados \n");
		printf("Aguardando evento\n");
		Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
		evento_atual = Retorno - WAIT_OBJECT_0;
		if (evento_atual == 0)
		{
			break;
		}

		printf("Thread Desbloqueada\n");



   } while (evento_atual != 0);

	CloseHandle(Bloq_Dados);
	CloseHandle(Tecla_Esc);
	CloseHandle(eventos);

	printf("Processo de exibicao de dados encerrando sua execucao\n");
	Sleep(3000);
}

