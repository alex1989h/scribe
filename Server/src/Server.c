/*
 ============================================================================
 Name        : Server.c
 Author      : Anushavan Melkonyan, Alexander Hoffmann
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Server.h"

int socketFD;
fd_set activefds, readfds;

struct ConnectionInfo connectionInfo[255];
int connectionInfoSize = 0;

struct ControlInfoBody body;
int tabelleSize = 0;

int main(void) {
	int exitWileLoop = 0;
	int result;
	struct sockaddr_in isa;
	memset((void *) &body, 0, sizeof(body));
	FD_ZERO(&activefds);
	socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD < 0) {
		perror("socket");
		printf("ERROR on socket(): Erstellung eines Sockets fehlgeschlagen.\n");
		exit(EXIT_FAILURE);
	}
	FD_SET(socketFD, &activefds);
	memset((void *) &isa, 0, sizeof(isa));

	isa.sin_port = htons(PORT);

	int yes = 1;
	result = setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (result == -1) {
		perror("setsockopt");
		printf("ERROR on setsockopt: Bind funktioniert nicht.\n");
		exit(EXIT_FAILURE);
	}

	result = bind(socketFD, (struct sockaddr *) &isa, sizeof(isa));
	if (result < 0) {
		perror("bind");
		printf("ERROR on bind: Bind funktioniert nicht.\n");
		exit(EXIT_FAILURE);
	}

	result = listen(socketFD, 7);

	if (result < 0) {
		perror("listen");
		printf("ERROR: Es konnte kein Listing aktiviert werden \n");
		exit(EXIT_FAILURE);
	}

	pthread_t serverId;
	pthread_create(&serverId, NULL, Server, NULL);
	char command[20];
		while (!exitWileLoop) {
			printf("Geben sie ein Befehle ein:\n");
			fgets(command,20,stdin);
			command[strcspn(command,"\n")] = 0;
			if(strcmp(command,"/CONNECT") == 0){
				char ipAdresse[20];
				printf("Geben Si die Ip Addresse ein:\n");
				fgets(ipAdresse,20,stdin);
				ipAdresse[strcspn(command,"\n")] = 0;
				connectToServer(ipAdresse);
			}
		}
	pthread_join(serverId, NULL);
	return EXIT_SUCCESS;
}

void* Server(void* not_used) {
	int i;
	while (1) {
		readfds = activefds;
		if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &readfds)) {
				if (i == socketFD) {
					newConnection();
					break;
				} else {
					receiveMessage(i);
					break;
				}
			}
		}
	}
	return NULL;
}

void newConnection() {
	struct sockaddr_in tempIsa;

	memset((void*) &tempIsa, 0, sizeof(tempIsa));
	unsigned int tempIsaSize = sizeof(tempIsa);

	int connectionFD = accept(socketFD, (struct sockaddr *) &tempIsa,
			&tempIsaSize);

	if (connectionFD < 0) {
		perror("accept");
		printf(
				"ERROR on accept: Es konnte keine Verbindung aufgebaut werden.\n");
	} else {
		FD_SET(connectionFD, &activefds);
		printf("Accept successful!\n");
		printf("Ip: %s\n", inet_ntoa(tempIsa.sin_addr));
		printf("Port: %i\n", ntohs(tempIsa.sin_port));
	}
}

void receiveMessage(int tempSocketFD) {
	int result;
	bool nameExist = false;
	char tempName[15];
	int j;
	struct CommonHeader commonHeader;
	memset(&commonHeader, 0, sizeof(commonHeader));
	result = recv(tempSocketFD, (void*) &commonHeader, sizeof(commonHeader), 0);
	if (result == -1) {
		printf("ERROR on recv: Unable to receive Commonheader\n");
	}
	if (commonHeader.type == LOG_IN_OUT) {
		if (commonHeader.flag == (SYN)) {
			printf("Login Request\n");
			struct LogInOutBody logInOutBody;
			memset(&logInOutBody, 0, sizeof(logInOutBody));
			result = recv(tempSocketFD, (void*) &logInOutBody,
					sizeof(logInOutBody), 0);
			if (result == -1) {
				printf("ERROR on recv: Unable to receive LogInOutBody\n");
			}
			memset(&tempName, 0, sizeof(tempName));
			strcpy(&tempName, logInOutBody.benutzername);
			for (j = 0; j < connectionInfoSize; j++) {
				if (strcmp(connectionInfo[j].name, tempName) == 0) {
					nameExist = true;
					break;
				}
			}
			struct LogInOut logInOut;
			memset(&logInOut, 0, sizeof(logInOut));
			if (nameExist) {
				createHeader(&logInOut.commonHeader, LOG_IN_OUT,
						(DUP | SYN | ACK), 1, 0);

				result = send(tempSocketFD, (void*) &logInOut, sizeof(logInOut),
						0);
				if (result == -1) {
					printf(
							"ERROR on send(): Unable to send LogInOut with DUB\n");
				} else {
					printf(
							"Login fehlgeschlagen Benutzename: %s bereits vergeben\n",
							tempName);
				}
			} else {
				createHeader(&logInOut.commonHeader, LOG_IN_OUT, (SYN | ACK), 1,
						0);

				result = send(tempSocketFD, (void*) &logInOut, sizeof(logInOut),
						0);
				if (result == -1) {
					printf(
							"ERROR on send(): Unable to send LogInOut with ACK\n");
				} else {
					printf("Login erfolgreich. Neuer Benutze: %s hinzugefügt\n",
							tempName);
				}
				strcpy(&connectionInfo[connectionInfoSize].name, tempName);
				connectionInfo[connectionInfoSize].socketFD = tempSocketFD;
				connectionInfo[connectionInfoSize].hops = 1;
				strcpy(&body.tabelle[tabelleSize].benutzername, tempName);
				body.tabelle[tabelleSize].hops = 1;
				tabelleSize++;
				connectionInfoSize++;
			}
		} else if (commonHeader.flag == (FIN)) {
			struct LogInOutBody logInOutBody;
			memset(&logInOutBody, 0, sizeof(logInOutBody));
			result = recv(tempSocketFD, (void*) &logInOutBody,
					sizeof(logInOutBody), 0);
			if (result == -1) {
				printf("ERROR on recv: Unable to receive LogInOutBody\n");
			} else {
				strcpy(&tempName, logInOutBody.benutzername);
				for (j = 0; j < connectionInfoSize; j++) {
					if (strcmp(connectionInfo[j].name, tempName) == 0) {
						memset(&connectionInfo[j], 0,
								sizeof(connectionInfo[j]));
						memcpy(&connectionInfo[j],
								&connectionInfo[connectionInfoSize - 1],
								sizeof(connectionInfo[j]));
						memset(&connectionInfo[connectionInfoSize - 1], 0,
								sizeof(connectionInfo[connectionInfoSize - 1]));
						connectionInfoSize--;
						break;
					}
				}
				for (j = 0; j < tabelleSize; j++) {
					if (strcmp(body.tabelle[j].benutzername, tempName) == 0) {
						memset(&body.tabelle[j], 0, sizeof(body.tabelle[j]));
						memcpy(&body.tabelle[j], &body.tabelle[tabelleSize - 1],
								sizeof(body.tabelle[j]));
						memset(&body.tabelle[tabelleSize - 1], 0,
								sizeof(body.tabelle[tabelleSize - 1]));
						tabelleSize--;
						break;
					}
				}
				struct LogInOut logInOut;
				memset(&logInOut, 0, sizeof(logInOut));
				createHeader(&logInOut.commonHeader, LOG_IN_OUT, (FIN | ACK), 1,
						0);
				result = send(tempSocketFD, (void*) &logInOut, sizeof(logInOut),
						0);
				if (result == -1) {
					printf(
							"ERROR on send(): Unable to send LogInOut with DUB\n");
				} else {
					FD_CLR(tempSocketFD, &activefds);
				}
			}
		}
	} else if (commonHeader.type == CONTROL_INFO) {
		if (commonHeader.flag == GET) {
			struct CommonHeader tempHeader;
			createHeader(&tempHeader, CONTROL_INFO, 0, 1, tabelleSize);

			result = send(tempSocketFD, (void*) &tempHeader, sizeof(tempHeader),
					0);
			if (result == -1) {
				printf(
						"ERROR on send(): Unable to send Control Info Lcontrol Info\n");
			}
			result = send(tempSocketFD, (void*) &body, sizeof(body), 0);
			if (result == -1) {
				printf(
						"ERROR on send(): Unable to send Control Info Lcontrol Info\n");
			}
		} else {

		}
	} else if (commonHeader.type == MESSAGE) {
		struct MessageBody messageBody;
		memset(&messageBody, 0, sizeof(messageBody));

		result = recv(tempSocketFD, (void*) &messageBody, sizeof(messageBody),
				0);
		if (result == -1) {
			printf("ERROR on recv: Nachricht nicht erhalten\n");
		} else {
			int sendSocketFD = 0;
			for (j = 0; j < tabelleSize; j++) {
				if (strcmp(connectionInfo[j].name, messageBody.zielbenutzername)
						== 0) {
					sendSocketFD = connectionInfo[j].socketFD;
					break;
				}
			}
			if (sendSocketFD > 0) {
				result = send(sendSocketFD, (void*) &commonHeader,
						sizeof(commonHeader), 0);
				if (result == -1) {
					printf(
							"ERROR on send(): Unable to send MessageHeader Info\n");
				}
				result = send(sendSocketFD, (void*) &messageBody,
						sizeof(messageBody), 0);
				if (result == -1) {
					printf("ERROR on send(): Unable to send Message Info\n");
				}
			}
		}

	}
}

void createHeader(struct CommonHeader* commonHeader, uint8_t type, uint8_t flag,
		uint8_t version, uint8_t lenght) {
	memset(commonHeader, 0, sizeof(commonHeader));
	commonHeader->type = type;
	commonHeader->flag = flag;
	commonHeader->version = version;
	commonHeader->lenght = lenght;
}

void connectToServer(char *ipAdresse){
	int result;
	struct sockaddr_in isa;
	int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD == -1) {
		printf("ERROR on socket(): Erstellung des Sockets fehlgeschlagen\n");
	}
	memset(&isa, 0, sizeof(isa));
	isa.sin_family = AF_INET;
	isa.sin_port = htons(PORT);
	result = inet_pton(AF_INET, ipAdresse, &isa.sin_addr);
	if (result == 0) {
		printf("ERROR on inet_pton():  Not valid network address\n");
	} else if (result == -1) {
		printf("ERROR on inet_pton(): Not valid address family\n");
	}
	result = connect(serverSocketFD, (struct sockaddr *) &isa, sizeof(isa));
	if (result == -1) {
		perror("connect");
		printf("ERROR on connect(): Verbindung fehlgeschlagen\n");
		close(serverSocketFD);
	} else {
		FD_SET(serverSocketFD,&activefds);
		printf("Verbindung Zum Server Erfolgreich\n");
	}
}
