
#include <windows.h>a
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 



HANDLE eventos[2];
HANDLE Bloq_Alarmes;
HANDLE Tecla_Esc;
int Retorno = 0;
int bloqueada = 0;
int evento_atual = -1;
int main()
{
	
	
	Bloq_Alarmes = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Alarmes");
	if(Bloq_Alarmes == NULL) {
		printf("ERROR : %d", GetLastError());
	}
	Tecla_Esc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	if (Tecla_Esc == NULL)
	{
		printf("ERROR : %d", GetLastError());
	}
	eventos[0] = Tecla_Esc;
	eventos[1] = Bloq_Alarmes;
	do
	{
		printf("Thread Alarmes\n");
		printf("Aguardando evento\n");
		Retorno = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
	    evento_atual = Retorno - WAIT_OBJECT_0;
		if (evento_atual ==  0)
		{
			break;
		}
	    
		printf("Thread Desbloqueada\n");
		
		
		
		
	} while (evento_atual != 0);
	CloseHandle(Bloq_Alarmes);
	CloseHandle(Tecla_Esc);
	CloseHandle(eventos);


	printf("Processo de Alarmes encerrando execucao\n");
	Sleep(3000);
}