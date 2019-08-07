#include <stdio.h>
#include <winsock2.h>
#include <stdlib.h>
#include <assert.h>

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
			SOCKADDR_IN localAddress = {0};
			localAddress.sin_family = AF_INET;
			localAddress.sin_port = htons(PORT);
			localAddress.sin_addr.s_addr = INADDR_ANY;
			if (!bind(soc, (SOCKADDR*)&localAddress, sizeof(SOCKADDR_IN)))
			{
				char buffer[SOCKET_BUFFER_SIZE];
				SOCKADDR_IN from;
				int fromSize = sizeof(SOCKADDR_IN);
				printf("Listening socket...\n");

				int playerX = 0;
				int playerY = 0;

				int running = 1;
				while (running)
				{
					int bytesRecieved =
						recvfrom(soc, buffer, SOCKET_BUFFER_SIZE, 0,
								 (SOCKADDR*)&from, &fromSize);
					if (bytesRecieved != SOCKET_ERROR)
					{
						char clientInput = buffer[0];
						printf("%d.%d.%d.%d:%d: %c\n",
							   from.sin_addr.S_un.S_un_b.s_b1,
							   from.sin_addr.S_un.S_un_b.s_b2,
							   from.sin_addr.S_un.S_un_b.s_b3,
							   from.sin_addr.S_un.S_un_b.s_b4,
							   ntohs(from.sin_port),
							   clientInput);

						switch (clientInput)
						{
						case 'w': { playerY++; } break;
						case 'a': { playerX--; } break;
						case 's': { playerY--; } break;
						case 'd': { playerX++; } break;
						case 'q': { running = 0; } break;
						default: { printf("Unhandled input: %c\n", clientInput); } break; 
						}

						int writeIndex = 0;
						memcpy(&buffer[writeIndex], &playerX, sizeof(playerX));
						writeIndex += sizeof(playerX);

						memcpy(&buffer[writeIndex], &playerY, sizeof(playerY));
						writeIndex += sizeof(playerY);

						memcpy(&buffer[writeIndex], &running, sizeof(running));

						int bufferLen = sizeof(int) * 3;
						SOCKADDR* to = (SOCKADDR*)&from;
						if (sendto(soc, buffer, bufferLen, 0, to, sizeof(from)) != SOCKET_ERROR)
						{
							
						}
						else
						{
							printf("sendto() failed: %d", WSAGetLastError());
						}
					}
					else
					{
						printf("Error occurs when recieving packet: %d", WSAGetLastError());
						running = 0;
					}
					
				}
			}
			else
			{
				printf("Socket binding error: %d", WSAGetLastError());
			}
		}
		else
		{
			printf("Failed to create socket. Error: %d", WSAGetLastError());
		}
	}
	else
	{
		printf("Failed to init winsock\n");		
	}
	system("pause");
	return 0;
}
