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

HANDLE hSwitchLeituraCLP1;
HANDLE hSwitchLeituraCLP2;
HANDLE hSwitchLeituraCLP3;
HANDLE hSwitchDados;
HANDLE hSwitchAlarmes;
HANDLE hSwitchRetirada;

int tecla = 0;
int evento_atual = -1;

BOOL Processo_Alarmes;
STARTUPINFO st_alarmes;
PROCESS_INFORMATION PI_ALARMES;

BOOL Processo_Dados;
STARTUPINFO st_Dados;
PROCESS_INFORMATION PI_DADOS;

BOOL Processo_Leitura;
STARTUPINFO st_Leitura;
PROCESS_INFORMATION PI_LEITURA;

HANDLE ProcessesHandles[3];

int main()
{
	/* Criacao dos eventos de notificação */
	Tecla_Esc = CreateEvent(NULL, TRUE, FALSE, "Esc");
	CheckForError(Tecla_Esc);
	hSwitchRetirada = CreateEvent(NULL, FALSE, FALSE, "Retirada");
	CheckForError(hSwitchRetirada);
	hSwitchAlarmes = CreateEvent(NULL, FALSE, FALSE, "Alarmes");
	CheckForError(hSwitchAlarmes);
	hSwitchDados = CreateEvent(NULL, FALSE, FALSE, "Dados");
	CheckForError(hSwitchDados);
	hSwitchLeituraCLP1 = CreateEvent(NULL, FALSE, FALSE, "Leitura1");
	CheckForError(hSwitchLeituraCLP1);
	hSwitchLeituraCLP2 = CreateEvent(NULL, FALSE, FALSE, "Leitura2");
	CheckForError(hSwitchLeituraCLP2);
	hSwitchLeituraCLP3 = CreateEvent(NULL, FALSE, FALSE, "Monitoracao");
	CheckForError(hSwitchLeituraCLP3);

	/* Criacao do processo de exibição de alarmes */
	ZeroMemory(&st_alarmes, sizeof(st_alarmes));
	st_alarmes.cb = sizeof(st_alarmes);
	Processo_Alarmes = CreateProcess(NULL, "Processo_Alarmes.exe", NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &st_alarmes, &PI_ALARMES);
	if (Processo_Alarmes == 0 ) {
		printf("Erro na criacao do console de alarmes! Codigo = %d\n", GetLastError());
	}
	else{
		printf("Processo de alarmes rodando\n");
	}
	ProcessesHandles[0] = PI_ALARMES.hProcess;

	/* Criacao do processo de exibição dos dados */
	ZeroMemory(&st_Dados, sizeof(st_Dados));
	st_Dados.cb = sizeof(st_Dados);
	Processo_Dados = CreateProcess(NULL, "Processo_Dados.exe", NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &st_Dados, &PI_DADOS);
	if (Processo_Dados == 0) {
		printf("Erro na criacao do console! Codigo = %d\n", GetLastError());
		
	}
	else {
		printf("Processo de dados rodando\n");
	}
	ProcessesHandles[1] = PI_DADOS.hProcess;

	/* Criacao do processo de leitura dos CLPs e da retirada de mensagens */
	ZeroMemory(&st_Leitura, sizeof(st_Leitura));
	st_Leitura.cb = sizeof(st_Leitura);
	Processo_Leitura = CreateProcess(NULL, "Mensagens_CLP.exe", NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &st_Leitura, &PI_LEITURA);
	if (Processo_Leitura == 0) {
		printf("Erro na criacao do console! Codigo = %d\n", GetLastError());
	}
	else{
		printf("Processo de leitura e monitoramento rodando\n");
	}
	ProcessesHandles[2] = PI_LEITURA.hProcess;

	/* Leitura do teclado e notificação dos eventos */
	do
	{
		tecla = _getch();
		switch(tecla){
			case Tecla1: SetEvent(hSwitchLeituraCLP1); break;
			case Tecla2: SetEvent(hSwitchLeituraCLP2); break;
			case Teclam: SetEvent(hSwitchLeituraCLP3); break;
			case Teclar: SetEvent(hSwitchRetirada); break;
			case Teclap: SetEvent(hSwitchDados); break;
			case Teclaa: SetEvent(hSwitchAlarmes); break;
		}

	} while (tecla != Esc); 
	
	/* Notifica do esc e aguarda demais processos finalizarem */
	SetEvent(Tecla_Esc);
	printf("Processo de leitura do teclado encerrando sua execucao\n");
	Retorno = WaitForMultipleObjects(3, ProcessesHandles, TRUE, INFINITE);
	if (Retorno != WAIT_OBJECT_0)
	{
		CheckForError(Retorno);
	}

	/* Fechamento dos handles dos eventos */
	CloseHandle(hSwitchDados);
	CloseHandle(hSwitchLeituraCLP1);
	CloseHandle(hSwitchLeituraCLP2);
	CloseHandle(hSwitchLeituraCLP3);
	CloseHandle(hSwitchRetirada);
	CloseHandle(hSwitchAlarmes);
	CloseHandle(Tecla_Esc);
	CloseHandle(ProcessesHandles[0]);
	CloseHandle(ProcessesHandles[1]);
	CloseHandle(ProcessesHandles[2]);

}

  
