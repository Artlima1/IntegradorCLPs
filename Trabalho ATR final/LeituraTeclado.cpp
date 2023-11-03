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
 //***** Leitura1 � a primeira leitura de dados do CLP/ Leitura2 � a segunda thread de leitura de dados// Leitura 3 � a thread de leitura de alarmes// Retirada � a thread de retirada das mensagens// Dados e Alarme s�o as mesmas
//Threads que ser�o utilizadas
DWORD WINAPI Thread_Retirada();
DWORD WINAPI Thread_Alarmes();
DWORD WINAPI Thread_Dados();
DWORD WINAPI Thread_Leitura1CLP();
DWORD WINAPI Thread_Leitura2CLP();
DWORD WINAPI Thread_Leitura3CLP(); //Mudar nome
// WINAPI Thread_Leitura_Teclado();

//Handles das threads

HANDLE ThreadRetirada;
HANDLE ThreadAlarmes;
HANDLE ThreadDados;
HANDLE ThreadLeituraCLP1;
HANDLE ThreadLeituraCLP2;
HANDLE ThreadLeituraCLP3;
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
HANDLE Eventos_LCLP1[2];
HANDLE Eventos_LCLP2[2];
HANDLE Eventos_LCLP3[2];
HANDLE Eventos_Dados[2];
HANDLE Eventos_Retirada[2];
HANDLE Eventos_Alarmes[2];



int tecla = 0;
int Comando = 0;
int evento_atual = 0;
int i = 0;


//CRIAR LISTA DE EVENTOS AQUI ************


BOOL retorno;
STARTUPINFO start;
PROCESS_INFORMATION PI;




int main()
{
	DWORD dwThreadRetirada, dwThreadAlarme, dwThreadDados, dwThreadLeitura1, dwThreadLeitura2, dwThreadLeitura3, dwThreadTeclado;
	//VER PROGRAMA44 PARA FAZER AS MENSAGENS peri�dicas 
	Tecla_Esc = CreateEvent(NULL, TRUE, FALSE, "Esc");
	CheckForError(Tecla_Esc);
	Bloq_Retirada = CreateEvent(NULL, TRUE, FALSE, "Retirada");
	CheckForError(Bloq_Retirada);
	Bloq_Alarmes = CreateEvent(NULL, TRUE, FALSE, "Alarmes");
	CheckForError(Bloq_Alarmes);
	Bloq_Dados = CreateEvent(NULL, TRUE, FALSE, "Dados");
	CheckForError(Bloq_Dados);
	Bloq_Leitura1 = CreateEvent(NULL, TRUE, FALSE, "Leitura1");
	CheckForError(Bloq_Leitura1);
	Bloq_Leitura2 = CreateEvent(NULL, TRUE, FALSE, "Leitura2");
	CheckForError(Bloq_Leitura2);
	Bloq_Leitura3 = CreateEvent(NULL, TRUE, FALSE, "Monitora��o"); // MUDAR NOME
	CheckForError(Bloq_Leitura3);






	











	HANDLE Thread_Gusa[7];
	Thread_Gusa[0] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Retirada, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadRetirada);
	if (Thread_Gusa[0] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadRetirada);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	i++;
	Thread_Gusa[1] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Alarmes, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadAlarme);
	if (Thread_Gusa[1] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadAlarme);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	i++;
	Thread_Gusa[2] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Dados, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadDados);
	if (Thread_Gusa[2] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadDados);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	i++;

	Thread_Gusa[3] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Leitura1CLP, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadLeitura1);
	if (Thread_Gusa[3] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadLeitura1);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	i++;
	Thread_Gusa[4] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Leitura2CLP, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadLeitura2);
	if (Thread_Gusa[4] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadLeitura2);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	i++;

	Thread_Gusa[5] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Leitura3CLP, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadLeitura3);
	if (Thread_Gusa[5] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadLeitura3);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	i++;

	/*Thread_Gusa[6] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)Thread_Leitura_Teclado, (LPVOID)(INT_PTR)i, 0, (CAST_LPDWORD)&dwThreadLeitura3);
	if (Thread_Gusa[6] != (HANDLE)-1L)
	{
		printf("Thread Criada %d com sucesso, ID = %0x\n", i, dwThreadLeitura3);
	}
	else
	{
		printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
		exit(0);
	}
	*/


	

	ZeroMemory(&start, sizeof(start));
	start.cb = sizeof(start);

	retorno = CreateProcess("Trabalho ATR Certo\\x64\\Debug\\Processo_Alarmes.exe",
		NULL,
		NULL,
		NULL,
		TRUE,
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


	retorno = CreateProcess("Trabalho ATR Certo\\x64\\Debug\\Processo_Dados.exe",
		NULL,
		NULL,
		NULL,
		TRUE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&start,
		&PI);
	if (retorno == 0)
	{
		printf("Erro na criacao do console! Codigo = %d\n", GetLastError());
	}
	else
	{
		printf("Processo de dados rodando\n");
	}
	
	do
	{
		tecla = _getch();
		if (tecla == Tecla1)
		{
			    SetEvent(Bloq_Leitura1); 
				Estado_Atual = WaitForSingleObject(Bloq_Leitura1, 0);
				if (Estado_Atual == WAIT_OBJECT_0)
				{
					ResetEvent(Bloq_Leitura1);
				}
		}
		if (tecla == Tecla2)
		{
			SetEvent(Bloq_Leitura2);
			Estado_Atual = WaitForSingleObject(Bloq_Leitura2, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Leitura2);
			}

		}
		if (tecla == Teclam)
		{
			SetEvent(Bloq_Leitura3);

			Estado_Atual = WaitForSingleObject(Bloq_Leitura3, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Leitura3);

			}
		}
		if (tecla == Teclar)
		{
			SetEvent(Bloq_Retirada);
			Estado_Atual = WaitForSingleObject(Bloq_Retirada, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Retirada);

			}
		}
		if (tecla == Teclap)
		{
			SetEvent(Bloq_Dados);



			Estado_Atual = WaitForSingleObject(Bloq_Dados, 0);
			if (Estado_Atual == WAIT_OBJECT_0)
			{
				ResetEvent(Bloq_Dados);

			}
		}
		if (tecla == Teclaa)
		{
			
				
				SetEvent(Bloq_Alarmes);

				
				Estado_Atual = WaitForSingleObject(Bloq_Alarmes, 100);
				if (Estado_Atual == WAIT_OBJECT_0)
				{
					ResetEvent(Bloq_Alarmes);
					
				}

			
		}
		



	} while (tecla != Esc); SetEvent(Tecla_Esc);
	printf("Thread do teclado encerrando sua execucao\n");





	Retorno = WaitForMultipleObjects(7, Thread_Gusa, TRUE, INFINITE); //Fun��o de espera do fechamento das threads
	if (Retorno != WAIT_OBJECT_0)//COLOCAR O CHECKFORERROR*****************
	{
		CheckForError(Retorno);
	}


	for (int i = 0; i < 10; i++) //Fechamento dos Handles
	{
		CloseHandle(Thread_Gusa[i]);

	}
	//Fechamento dos handles das threads
	CloseHandle(ThreadDados);
	CloseHandle(ThreadLeituraCLP1);
	CloseHandle(ThreadLeituraCLP2);
	CloseHandle(ThreadLeituraCLP3);
	CloseHandle(ThreadRetirada);
	CloseHandle(ThreadAlarmes);


	//Fechamento dos handles dos eventos
	CloseHandle(Bloq_Dados);
	CloseHandle(Bloq_Leitura1);
	CloseHandle(Bloq_Leitura2);
	CloseHandle(Bloq_Leitura3);
	CloseHandle(Bloq_Retirada);
	CloseHandle(Bloq_Alarmes);
	CloseHandle(Tecla_Esc);

	CloseHandle(Eventos_LCLP1);
	CloseHandle(Eventos_LCLP2);
	CloseHandle(Eventos_LCLP3);
	CloseHandle(Eventos_Dados);
	CloseHandle(Eventos_Retirada);
	CloseHandle(Eventos_Alarmes);

	CloseHandle(hin);




}

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

/*
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

  