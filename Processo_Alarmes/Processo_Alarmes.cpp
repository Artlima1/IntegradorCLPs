
#include <windows.h>a
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <time.h>
#include "CheckForError.h"; 

HANDLE hEventos[2];
HANDLE hSwitchAlarmes;
HANDLE hEsc;
int ret = 0;

int main()
{
	
	hSwitchAlarmes = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Alarmes");
	if(hSwitchAlarmes == NULL) {
		printf("ERROR : %d", GetLastError());
	}
	hEsc = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Esc");
	if (hEsc == NULL)
	{
		printf("ERROR : %d", GetLastError());
	}
	hEventos[0] = hEsc;
	hEventos[1] = hSwitchAlarmes;

	printf("Thread de alarmes Inicializada\n");

	while(1) {
		ret = WaitForMultipleObjects(2, hEventos, FALSE, 0);
		if (ret == WAIT_OBJECT_0) break;
		if(ret == WAIT_OBJECT_0 + 1){
			printf("Thread de alarmes Bloqueada\n");
			ret = WaitForMultipleObjects(2, hEventos, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0){
				break;
			}
			printf("Thread de alarmes Desbloqueada\n");
		}
	}
	CloseHandle(hSwitchAlarmes);
	CloseHandle(hEsc);

	printf("Processo de Alarmes encerrando execucao\n");
	Sleep(3000);
}