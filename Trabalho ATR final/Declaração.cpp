#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	
#include <conio.h>	
#include <time.h>
#include "CheckForError.h"; 

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);	
typedef unsigned* CAST_LPDWORD;

#define Tecla1 0x31
#define Tecla2 0x32
#define Teclam 0x6D
#define Teclar 0x72
#define Teclaa 0x61
#define Teclap 0x70
#define Esc 0x1B







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
int evento_atual = -1;
//Informações para iniacilização de um novo processo
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
//Variáveis de estado
BOOL Alarme_Ativo = TRUE;
BOOL Dados_Ativo = TRUE;
BOOL Retirada_Ativo = TRUE;
BOOL Leitura1_Ativo = TRUE;
BOOL Leitura2_Ativo = TRUE;
BOOL Leitura_dados = TRUE;

int main()
{
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
	Bloq_Leitura3 = CreateEvent(NULL, TRUE, Leitura_dados, "Monitoracao");
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
		printf("Processo de leitura e monitoramento rodando\n");
	}
	ProcessesHandles[2] = PI_LEITURA.hProcess;

	do
	{
		tecla = _getch();
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
			if (Leitura2_Ativo) 
			{
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

	

}

  
