
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 





HANDLE Bloq_Dados;
HANDLE Tecla_Esc;
int Retorno = 0;
int evento_atual = -1;
int bloq = 1;
HANDLE eventos[2];



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
		    Sleep(1000);
			Retorno = WaitForMultipleObjects(2, eventos, FALSE, 200);
			evento_atual = Retorno - WAIT_OBJECT_0;
			if (evento_atual == 0)
			{
				break;
			}
			if(evento_atual !=0 && Retorno != WAIT_TIMEOUT)
			{
					printf("Thread de exibicao de dados desbloqueada\n");
					
				
			}
			else if (Retorno == WAIT_TIMEOUT)
			{
				printf("Thread de exibicao de dados Bloqueada\n");
			}



   } while (evento_atual != 0);

	CloseHandle(Bloq_Dados);
	CloseHandle(Tecla_Esc);
	CloseHandle(eventos[0]);
	CloseHandle(eventos[1]);

	printf("Processo de exibicao de dados encerrando sua execucao\n");
	Sleep(3000);
}

