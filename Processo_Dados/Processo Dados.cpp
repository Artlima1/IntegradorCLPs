
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 

HANDLE hSwitchDados;
HANDLE hEsc;
int ret = 0;
HANDLE hEventos[2];

int main()
{
	hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	if (hEsc == NULL)
	{
		printf("ERROR : %d", GetLastError());
	}

	hSwitchDados = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Dados");
	if (hSwitchDados == NULL) {
		printf("ERROR : %d", GetLastError());
	}
	hEventos[0] = hEsc;
	hEventos[1] = hSwitchDados;

	printf("Thread de dados Inicializada\n");
   	while(1)
	{
		ret = WaitForMultipleObjects(2, hEventos, FALSE, 0);
		if (ret == WAIT_OBJECT_0) break;
		if(ret == WAIT_OBJECT_0 + 1){
			printf("Thread de dados Bloqueada\n");
			ret = WaitForMultipleObjects(2, hEventos, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0){
				break;
			}
			printf("Thread de dados Desbloqueada\n");
		}
   }

	CloseHandle(hSwitchDados);
	CloseHandle(hEsc);

	printf("Processo de exibicao de dados encerrando sua execucao\n");
	Sleep(3000);
}

