#include <stdio.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include "Win32Platform.cpp"

#if 0
#define WINSOCK_VER  0x202
#define SOCKET_BUFFER_SIZE  1024
#define PORT 9999

int main()
{	
	WSADATA winsockData;
	if (!WSAStartup(WINSOCK_VER, &winsockData))
	{
		SOCKET soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (soc != INVALID_SOCKET)
		{
			char ipAddr[128];
			printf("Input IP...\n");
			scanf_s("\n%s", &ipAddr, 128);
			printf("\nIP: %s\n", ipAddr);
			SOCKADDR_IN serverAddress = {};
			serverAddress.sin_family = AF_INET;
			serverAddress.sin_port = htons(PORT);
			serverAddress.sin_addr.S_un.S_addr = inet_addr(ipAddr);
			
			char buffer[SOCKET_BUFFER_SIZE];

			int playerX = 0;
			int playerY = 0;

			int running = 1;
			while (running)
			{
				scanf_s("\n%c", &buffer[0], 1);

				SOCKADDR* to = (SOCKADDR*)&serverAddress;
				if (sendto(soc, buffer, SOCKET_BUFFER_SIZE, 0, to, sizeof(serverAddress)) != SOCKET_ERROR)
				{
					SOCKADDR_IN from;
					int fromSize = sizeof(from);
					int bytesRecieved =
						recvfrom(soc, buffer, SOCKET_BUFFER_SIZE, 0, (SOCKADDR*)&from, &fromSize);
					if (bytesRecieved != SOCKET_ERROR)
					{
						int readIndex = 0;

						memcpy(&playerX, &buffer[readIndex], sizeof(playerX));
						readIndex += sizeof(playerX);
						memcpy(&playerY, &buffer[readIndex], sizeof(playerY));
						readIndex += sizeof(playerY);
						memcpy(&running, &buffer[readIndex], sizeof(running));
						printf("Recieved: x:%d, y:%d, running:%d\n", playerX, playerY, running);
					}
					else
					{
						printf("recvfrom() error.\n");
						running = 0;
					}
				}
				else
				{
					printf("sendto() error\n");
					running = 0;
				}
			}
			gets_s(buffer, SOCKET_BUFFER_SIZE);
			
			if (sendto(soc, buffer, SOCKET_BUFFER_SIZE, 0,
					   (SOCKADDR*)&serverAddress, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
			{
				printf("sendto() succeded\n");
			}
			else
			{
				printf("sendto() error: %d\n", WSAGetLastError());
			}
		}
		else
		{
			printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		}
	}
	else
	{
		printf("Failed to init winsock\n");
	}

	
	return 0;
}
#endif
