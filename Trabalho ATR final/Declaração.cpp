#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>	
#include <time.h>
#include "CheckForError.h"; 

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);	// Casting para terceiro e sexto par�metros da fun��o
typedef unsigned* CAST_LPDWORD;

#define Tecla1 0x31
#define Tecla2 0x32
#define Teclam 0x6D
#define Teclar 0x72
#define Teclaa 0x61
#define Teclap 0x70
#define Esc 0x1B


struct mensagem_dados_processo
{
	//LIMITAR CARACTERES

	int nseq; //N�mero sequencial da mensagem
	int id; //Identifica��o do CLP
	int diag; //Diagn�stico dos cart�es do CLP
	float pressao_interna; //Press�o interna na panela de gusa
	float pressao_injecao; //Press�o de inje��o do nitrog�nio
	float temp; //Temperatura na panela
	time_t timestamp; //Horas da mensagem

};

struct mensagem_alarme
{
	int nseq_alarme;
	char id[2];
	time_t timestamp_alarme;
};

//HANDLE ThreadLeituraTeclado;

HANDLE hin;
DWORD Retorno;
HANDLE Tecla_Esc;
DWORD Estado_Atual;

//Handle para eventos
HANDLE Bloq_Leitura1;
HANDLE Bloq_Leitura2;
HANDLE Bloq_Leitura3;
HANDLE Bloq_Dados;
HANDLE Bloq_Alarmes;
HANDLE Bloq_Retirada;



int tecla = 0;
int Comando = 0;
int evento_atual = -1;
int i = 0;


//CRIAR LISTA DE EVENTOS AQUI ************


BOOL retorno;
STARTUPINFO start;
PROCESS_INFORMATION PI;

BOOL Processo_Dados;
STARTUPINFO st_Dados;
PROCESS_INFORMATION PI_DADOS;

BOOL Processo_Leitura;
STARTUPINFO st_Leitura;
PROCESS_INFORMATION PI_LEITURA;

HANDLE ProcessesHandles[3];

BOOL Alarme_Ativo = TRUE;
BOOL Dados_Ativo = TRUE;
BOOL Retirada_Ativo = TRUE;
BOOL Leitura1_Ativo = TRUE;
BOOL Leitura2_Ativo = TRUE;
BOOL Leitura_dados = TRUE;

int main()
{

	DWORD dwThreadRetirada, dwThreadAlarme, dwThreadDados, dwThreadLeitura1, dwThreadLeitura2, dwThreadLeitura3, dwThreadTeclado;
	//VER PROGRAMA44 PARA FAZER AS MENSAGENS peri�dicas 
	Tecla_Esc = CreateEvent(NULL, TRUE, FALSE, "Esc");
	CheckForError(Tecla_Esc);
	Bloq_Retirada = CreateEvent(NULL, TRUE, Retirada_Ativo, "Retirada");
	CheckForError(Bloq_Retirada);
	Bloq_Alarmes = CreateEvent(NULL, TRUE, Alarme_Ativo, "Alarmes");
	CheckForError(Bloq_Alarmes);
	Bloq_Dados = CreateEvent(NULL, TRUE, Dados_Ativo, "Dados");
	CheckForError(Bloq_Dados);
	Bloq_Leitura1 = CreateEvent(NULL, TRUE, Leitura1_Ativo, "Leitura1");
	CheckForError(Bloq_Leitura1);
	Bloq_Leitura2 = CreateEvent(NULL, TRUE, Leitura2_Ativo, "Leitura2");
	CheckForError(Bloq_Leitura2);
	Bloq_Leitura3 = CreateEvent(NULL, TRUE, Leitura_dados, "Monitoracao"); // MUDAR NOME
	CheckForError(Bloq_Leitura3);




	ZeroMemory(&start, sizeof(start));
	start.cb = sizeof(start);

	retorno = CreateProcess(NULL,
		"Processo_Alarmes.exe",
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&start,
		&PI);
	if (retorno == 0 )
	{
		printf("Erro na criacao do console de alarmes! Codigo = %d\n", GetLastError());
	
	}
	else
	{
		printf("Processo de alarmes rodando\n");
	}
	ProcessesHandles[0] = PI.hProcess;


	ZeroMemory(&st_Dados, sizeof(st_Dados));
	st_Dados.cb = sizeof(st_Dados);

	Processo_Dados = CreateProcess(NULL,
		"Processo_Dados.exe",
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&st_Dados,
		&PI_DADOS);
	if (retorno == 0)
	{
		printf("Erro na criacao do console! Codigo = %d\n", GetLastError());
		
	}
	else
	{
		printf("Processo de dados rodando\n");
		
	}
	ProcessesHandles[1] = PI_DADOS.hProcess;

	ZeroMemory(&st_Leitura, sizeof(st_Leitura));
	st_Leitura.cb = sizeof(st_Leitura);

	Processo_Leitura = CreateProcess(NULL,
		"Mensagens_CLP.exe",
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL,
		NULL,
		&st_Leitura,
		&PI_LEITURA);
	if (retorno == 0)
	{
		printf("Erro na criacao do console! Codigo = %d\n", GetLastError());

	}
	else
	{
		printf("Processo de dados rodando\n");

	}
	ProcessesHandles[2] = PI_LEITURA.hProcess;
	
	do
	{
		tecla = _getch();
		printf("%d\n", tecla);
		if (tecla == Tecla1)
		{
			if (Leitura1_Ativo) {
				ResetEvent(Bloq_Leitura1);
			}
			else {
				SetEvent(Bloq_Leitura1);
			}
			Leitura1_Ativo = !Leitura1_Ativo;
	

		}
		if (tecla == Tecla2)
		{
			if (Leitura2_Ativo) {
				ResetEvent(Bloq_Leitura2);
			}
			else {
				SetEvent(Bloq_Leitura2);
			}
			Leitura2_Ativo = !Leitura2_Ativo;

		}

		if (tecla == Teclam)
		{
			if (Leitura_dados) {
				ResetEvent(Bloq_Leitura3);
			}
			else {
				SetEvent(Bloq_Leitura3);
			}
			Leitura_dados = !Leitura_dados;
		}
		if (tecla == Teclar)
		{
			if (Retirada_Ativo) {
				ResetEvent(Bloq_Retirada);
			}
			else {
				SetEvent(Bloq_Retirada);
			}
			Retirada_Ativo = !Retirada_Ativo;
		}
		if (tecla == Teclap)
		{
			if (Dados_Ativo) {
				ResetEvent(Bloq_Dados);
			}
			else {
				SetEvent(Bloq_Dados);
			}
			Dados_Ativo = !Dados_Ativo;
		}
		if (tecla == Teclaa)
		{
			if(Alarme_Ativo){
				ResetEvent(Bloq_Alarmes);
			}
			else {
				SetEvent(Bloq_Alarmes);
			}
			Alarme_Ativo = !Alarme_Ativo;
		}
		

	} while (tecla != Esc); 
	
	SetEvent(Tecla_Esc);
	printf("Processo de leitura do teclado encerrando sua execucao\n");
	Retorno = WaitForMultipleObjects(3, ProcessesHandles, TRUE, INFINITE);
	if (Retorno != WAIT_OBJECT_0)
	{
		CheckForError(Retorno);
	}

	//Fechamento dos handles dos eventos
	CloseHandle(Bloq_Dados);
	CloseHandle(Bloq_Leitura1);
	CloseHandle(Bloq_Leitura2);
	CloseHandle(Bloq_Leitura3);
	CloseHandle(Bloq_Retirada);
	CloseHandle(Bloq_Alarmes);
	CloseHandle(Tecla_Esc);
	CloseHandle(ProcessesHandles[0]);
	CloseHandle(ProcessesHandles[1]);
	CloseHandle(ProcessesHandles[2]);

	CloseHandle(hin);

}
/*


DWORD WINAPI Thread_Retirada()
{

	Eventos_Retirada[0] = Tecla_Esc;
	Eventos_Retirada[1] = Bloq_Retirada;
	int bloqueio_retirada = 0;
	do
	{
		//Colocar os comandos aqui
		printf("Rodando Retirada\n");


		Comando = WaitForMultipleObjects(2, Eventos_Retirada, FALSE, 100);
		evento_atual = Comando - WAIT_OBJECT_0;
		if (evento_atual == 0)
		{
			break;
		}
		if (evento_atual == 1)
		{
			printf("Dormindo Retirada\n");
			bloqueio_retirada = WaitForSingleObject(Bloq_Retirada, INFINITE);
			if (bloqueio_retirada == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Retirada);
			}


		}
	} while (evento_atual != 0);
	printf("Thread de Retirada encerrando sua execucao\n");
	return(0);
	
}

DWORD WINAPI Thread_Alarmes()
{

	Eventos_Alarmes[0] = Tecla_Esc;
	Eventos_Alarmes[1] = Bloq_Alarmes;
	int bloqueio_alarmes = 0;
	do
	{

		printf("Rodando Alarmes\n");
		//Colocar os comandos aqui

		Comando = WaitForMultipleObjects(2, Eventos_Alarmes, FALSE, 10);
		evento_atual = Comando - WAIT_OBJECT_0;
		if (evento_atual == 0)
		{
			break;
		}
		if (evento_atual == 1)
		{
			printf("Dormindo alarmes\n");
			bloqueio_alarmes = WaitForSingleObject(Bloq_Alarmes, INFINITE);
			if (bloqueio_alarmes == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Alarmes);
			}


		}
	} while (evento_atual != 0);
	printf("Thread de Alarmes encerrando sua execucao\n");
	return(0);

}

DWORD WINAPI Thread_Dados()
{
	Eventos_Dados[0] = Tecla_Esc;
	Eventos_Dados[1] = Bloq_Dados;
	int bloqueio_dados = 0;

	do
	{
		//Colocar os comandos aqui
		printf("Rodando Dados\n");


		Comando = WaitForMultipleObjects(2, Eventos_Dados, FALSE, 10);
		evento_atual = Comando - WAIT_OBJECT_0;
		if (evento_atual == 0)
		{
			break;
		}
		if (evento_atual == 1)
		{
			printf("Dormindo Dados\n");
			bloqueio_dados = WaitForSingleObject(Bloq_Dados, INFINITE);
			if (bloqueio_dados == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Dados);
			}
		}
	} while (evento_atual != 0);
	printf("Thread de Dados encerrando sua execucao\n");
	return(0);

}

DWORD WINAPI Thread_Leitura1CLP()
{

	Eventos_LCLP1[0] = Tecla_Esc;
	Eventos_LCLP1[1] = Bloq_Leitura1;
	int bloqueioCLP1 = 0;

	do
	{
		  printf("Rodando Leitura1\n");
		//Colocar os comandos aqui
		
	        
			Comando = WaitForMultipleObjects(2, Eventos_LCLP1, FALSE, 10);
			evento_atual = Comando - WAIT_OBJECT_0;
			if (evento_atual == 0)
			{
				//break;
			}
			if (evento_atual == 1)
			{
				printf("Dormindo Leitura1\n");
				bloqueioCLP1 = WaitForSingleObject(Bloq_Leitura1, INFINITE);
				if (bloqueioCLP1 == WAIT_OBJECT_0)
				{
					ResetEvent(Bloq_Leitura1);
				}
			
			
			}
			
		
	} while (evento_atual != 0);
	printf("Primeira Thread de Leitura do CLP encerrando sua execucao\n");

	return(0);
}
DWORD WINAPI Thread_Leitura2CLP()
{

	Eventos_LCLP2[0] = Tecla_Esc;
	Eventos_LCLP2[1] = Bloq_Leitura2;
	int bloqueio_CLP2 = 0;
	do
	{
		
		printf("Rodando Leitura2\n");
		//Colocar os comandos aqui


		Comando = WaitForMultipleObjects(2, Eventos_LCLP2, FALSE, 10);
		evento_atual = Comando - WAIT_OBJECT_0;
		if (evento_atual == 0)
		{
			break;
		}
		if (evento_atual == 1)
		{
			printf("Dormindo Leitura 2\n");
			bloqueio_CLP2 = WaitForSingleObject(Bloq_Leitura2, INFINITE);
			if (bloqueio_CLP2 == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Leitura2);
			}


		}
	} while (evento_atual != 0);
	printf("Segunda Thread de Leitura do CLP encerrando sua execucao\n");
	return(0);
}
DWORD WINAPI Thread_Leitura3CLP()
{
	Eventos_LCLP3[0] = Tecla_Esc;
	Eventos_LCLP3[1] = Bloq_Leitura3;
	int bloqueio_clp3 = 0;
	do
	{
		//Colocar os comandos aqui
		printf("Rodando Leitura3\n");


		Comando = WaitForMultipleObjects(2, Eventos_LCLP3, FALSE, 10);
		evento_atual = Comando - WAIT_OBJECT_0;
		if (evento_atual == 0)
		{
			break;
		}
		if (evento_atual == 1)
		{
			printf("Dormindo Leitura 3\n");
			bloqueio_clp3 = WaitForSingleObject(Bloq_Leitura3, INFINITE);
			if (bloqueio_clp3 == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Leitura3);
			}


		}
	} while (evento_atual != 0);
	printf("Thread de Leitura de Alarmes encerrando sua execucao\n");
	return(0);

}


DWORD WINAPI Thread_Leitura_Teclado()
{
	do
	{

		tecla = _getch();
		printf("%d\n", tecla);
		if (tecla == Tecla1)
		{

			Estado_Atual = WaitForSingleObject(Bloq_Leitura1, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				printf("Teste1\n");
				SetEvent(Bloq_Leitura1);
				printf("Objeto Bloqueado\n");
			}
			else
			{
				printf("Objeto desbloqueado\n");
				ResetEvent(Bloq_Leitura1);
			}
		}
		else if (tecla == Tecla2)
		{
			Estado_Atual = WaitForSingleObject(Bloq_Leitura2, 0);

			if (Estado_Atual == WAIT_OBJECT_0)
			{
				SetEvent(Bloq_Leitura2);
				printf("Objeto Bloqueado\n");
			}
			else
			{
				ResetEvent(Bloq_Leitura2);

			}
		}
		else if (tecla == Teclam)
		{
			Estado_Atual = WaitForSingleObject(Bloq_Leitura3, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				SetEvent(Bloq_Leitura3);
				printf("Objeto Bloqueado\n");
			}
			else
			{
				ResetEvent(Bloq_Leitura3);

			}
		}
		else if (tecla == Teclar)
		{
			Estado_Atual = WaitForSingleObject(Bloq_Retirada, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				SetEvent(Bloq_Retirada);
				printf("Objeto Bloqueado\n");
			}
			else
			{
				ResetEvent(Bloq_Retirada);

			}
		}
		else if (tecla == Teclap)
		{
			Estado_Atual = WaitForSingleObject(Bloq_Dados, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				SetEvent(Bloq_Dados);
				printf("Objeto Bloqueado\n");
			}
			else
			{
				ResetEvent(Bloq_Dados);

			}
		}
		else if (tecla == Teclaa)
		{
			Estado_Atual = WaitForSingleObject(Bloq_Alarmes, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				printf("Objeto Bloqueado\n");
				SetEvent(Bloq_Alarmes);
			}
			else
			{
				ResetEvent(Bloq_Alarmes);

			}
		}
		else if (tecla == Esc)SetEvent(Tecla_Esc);



	} while (tecla != Esc);
	printf("Thread do teclado encerrando sua execucao\n");
	return(0);
	*/

  
