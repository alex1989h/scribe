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

int serverfds[100];
int serverSize = 0;

struct ConnectionInfo connectionInfo[255];
struct ControlInfoBody localBody;
int tabelleSize = 0;

int main(void) {
	int result;
	struct sockaddr_in isa;
	memset((void *) &localBody, 0, sizeof(localBody));
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
	commands();
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

void receiveMessage(int currentSocketFD) {
	int result;
	bool nameExist = false;
	bool changesOnTabelle = false;
	char tempName[15];
	int j,i;
	struct CommonHeader receivedHeader;
	memset(&receivedHeader, 0, sizeof(receivedHeader));
	result = recv(currentSocketFD, (void*) &receivedHeader, sizeof(receivedHeader), 0);
	if (result == -1) {
		printf("ERROR on recv: Unable to receive Commonheader\n");
	}
	if (receivedHeader.type == LOG_IN_OUT) {
		if (receivedHeader.flag == (SYN)) {
			printf("Login Request\n");
			struct LogInOutBody logInOutBody;
			memset(&logInOutBody, 0, sizeof(logInOutBody));
			result = recv(currentSocketFD, (void*) &logInOutBody,
					sizeof(logInOutBody), 0);
			if (result == -1) {
				printf("ERROR on recv: Unable to receive LogInOutBody\n");
			}
			memset(&tempName, 0, sizeof(tempName));
			strcpy(tempName, logInOutBody.benutzername);
			for (j = 0; j < tabelleSize; j++) {
				if (strcmp(connectionInfo[j].name, tempName) == 0) {
					nameExist = true;
					break;

				}
			}
			struct LogInOut logInOut;
			memset(&logInOut, 0, sizeof(logInOut));
			if (nameExist) {
				createHeader(&logInOut.commonHeader, LOG_IN_OUT,(DUP | SYN | ACK), 1, 0);
				result = send(currentSocketFD, (void*) &logInOut, sizeof(logInOut), 0);
				if (result == -1) {
					printf("ERROR on send(): Unable to send LogInOut with DUB\n");
				} else {
					printf("Login fehlgeschlagen Benutzename: %s bereits vergeben\n",tempName);
				}
			} else {
				createHeader(&logInOut.commonHeader, LOG_IN_OUT, (SYN | ACK), 1, 0);
				result = send(currentSocketFD, (void*) &logInOut, sizeof(logInOut), 0);
				if (result == -1) {
					printf("ERROR on send(): Unable to send LogInOut with ACK\n");
				} else {
					printf("Login erfolgreich. Neuer Benutze: %s hinzugefügt\n",tempName);
				}
				strcpy(connectionInfo[tabelleSize].name, tempName);
				connectionInfo[tabelleSize].socketFD = currentSocketFD;
				connectionInfo[tabelleSize].hops = 1;
				strcpy(localBody.tabelle[tabelleSize].benutzername, tempName);
				localBody.tabelle[tabelleSize].hops = 1;
				tabelleSize++;

				notifyAllServers();
			}
		} else if (receivedHeader.flag == (FIN)) {
			printf("Logout Request\n");
			struct LogInOutBody logInOutBody;
			memset(&logInOutBody, 0, sizeof(logInOutBody));
			result = recv(currentSocketFD, (void*) &logInOutBody, sizeof(logInOutBody), 0);
			if (result == -1) {
				printf("ERROR on recv: Unable to receive LogInOutBody\n");
			} else {
				strcpy(tempName, logInOutBody.benutzername);
				for (j = 0; j < tabelleSize; j++) {
					if (strcmp(connectionInfo[j].name, tempName) == 0) {
						deleteEntry(j);
						break;
					}
				}

				struct LogInOut logInOut;
				memset(&logInOut, 0, sizeof(logInOut));
				createHeader(&logInOut.commonHeader, LOG_IN_OUT, (FIN | ACK), 1, 0);
				result = send(currentSocketFD, (void*) &logInOut, sizeof(logInOut), 0);
				if (result == -1) {
					printf("ERROR on send(): Unable to send LogInOut with DUB\n");
				} else {
					FD_CLR(currentSocketFD, &activefds);
				}
				notifyAllServers();
			}
		}
	} else if (receivedHeader.type == CONTROL_INFO) {
		if (receivedHeader.flag == GET) {
			printf("Give me all User Request\n");
			struct CommonHeader tempHeader;
			createHeader(&tempHeader, CONTROL_INFO, 0, 1, tabelleSize);

			result = send(currentSocketFD, (void*) &tempHeader, sizeof(tempHeader), 0);
			if (result == -1) {
				printf("ERROR on send(): Unable to send Control Info Lcontrol Info\n");
			}
			result = send(currentSocketFD, (void*) &localBody, sizeof(localBody), 0);
			if (result == -1) {
				printf("ERROR on send(): Unable to send Control Info Lcontrol Info\n");
			}
		} else {
			printf("ControlInfo erhalten\n");

			putNewServer(currentSocketFD);

			//TODO:Tabellen Austauschen
			struct ControlInfoBody receivedBody;
			memset(&receivedBody, 0, sizeof(receivedBody));
			result = recv(currentSocketFD, (void*) &receivedBody,sizeof(receivedBody), 0);
			if (result == -1) {
				printf("ERROR on recv: Unable to receive ControlInfobody\n");
			}else{
				changesOnTabelle = false;
				for (i = 0; i < receivedHeader.lenght; i++) {
					if (tabelleSize > 0) {
						for (j = 0; j < tabelleSize; j++) {
							if (strcmp(receivedBody.tabelle[i].benutzername,
									localBody.tabelle[j].benutzername) == 0) {
								if (receivedBody.tabelle[i].hops + 1 < localBody.tabelle[j].hops) {
									localBody.tabelle[j].hops = receivedBody.tabelle[i].hops + 1;
									connectionInfo[j].socketFD = currentSocketFD;
									connectionInfo[j].hops = receivedBody.tabelle[i].hops + 1;
									changesOnTabelle = true;
								}
							} else {
								memcpy(&localBody.tabelle[tabelleSize],&receivedBody.tabelle[i],sizeof(localBody.tabelle[tabelleSize]));
								localBody.tabelle[tabelleSize].hops++;

								strcpy(connectionInfo[tabelleSize].name,receivedBody.tabelle[i].benutzername);
								connectionInfo[tabelleSize].socketFD =currentSocketFD;
								connectionInfo[tabelleSize].hops =receivedBody.tabelle[i].hops + 1;
								tabelleSize++;
								changesOnTabelle = true;
							}
						}
					}else{
						memcpy(&localBody.tabelle[tabelleSize],&receivedBody.tabelle[i],sizeof(localBody.tabelle[tabelleSize]));
						localBody.tabelle[tabelleSize].hops++;

						strcpy(connectionInfo[tabelleSize].name,receivedBody.tabelle[i].benutzername);
						connectionInfo[tabelleSize].socketFD = currentSocketFD;
						connectionInfo[tabelleSize].hops =receivedBody.tabelle[i].hops + 1;
						tabelleSize++;
						changesOnTabelle = true;
					}
				}

				for (i = 0; i < tabelleSize; i++) {
					if (connectionInfo[i].socketFD == currentSocketFD) {
						nameExist = false;
						for (j = 0; j < receivedHeader.lenght; j++) {
							if(strcmp(receivedBody.tabelle[j].benutzername, connectionInfo[j].name) == 0){
								nameExist = true;
								break;
							}
						}
						if(!nameExist){
							deleteEntry(i);
							changesOnTabelle = true;
						}
					}
				}

				if(changesOnTabelle){
					notifyAllServers();
				}
			}
		}
	} else if (receivedHeader.type == MESSAGE) {
		printf("Message request\n");
		struct MessageBody messageBody;
		memset(&messageBody, 0, sizeof(messageBody));

		result = recv(currentSocketFD, (void*) &messageBody, sizeof(messageBody),
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
				result = send(sendSocketFD, (void*) &receivedHeader,
						sizeof(receivedHeader), 0);
				if (result == -1) {
					printf(
							"ERROR on send(): Unable to send MessageHeader Info\n");
				}
				result = send(sendSocketFD, (void*) &messageBody,sizeof(messageBody), 0);
				if (result == -1) {
					printf("ERROR on send(): Unable to send Message Info\n");
				}
			}
		}

	} else {
//		printf("Server oder Client ausgefallen\n");
//		changesOnTabelle = false;
//
//		for (i = 0; i < serverSize; i++) {
//			serverfds[i] = serverfds[serverSize - 1];
//			serverfds[serverSize - 1] = 0;
//			serverSize--;
//		}
//
//		for(i = 0;i<tabelleSize;i++){
//			if(connectionInfo[i].socketFD == currentSocketFD){
//				deleteEntry(i);
//				changesOnTabelle = true;
//			}
//		}
//		if(changesOnTabelle){
//			notifyAllServers();
//		}
//		FD_CLR(currentSocketFD,&activefds);
//		close(currentSocketFD);
	}
}

void createHeader(struct CommonHeader* commonHeader, uint8_t type, uint8_t flag,
		uint8_t version, uint8_t lenght) {
	memset((void*)commonHeader, 0, sizeof(commonHeader));
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
	} else {
		result = connect(serverSocketFD, (struct sockaddr *) &isa, sizeof(isa));
		if (result == -1) {
			perror("connect");
			printf("ERROR on connect(): Verbindung fehlgeschlagen\n");
			close(serverSocketFD);
		} else {
			FD_SET(serverSocketFD, &activefds);
			printf("Verbindung Zum Server Erfolgreich\n");
			sendControlInfo(serverSocketFD);
		}
	}
}

void sendControlInfo(int currentSocketFD){
	int result;
	struct CommonHeader commonHeader;
	createHeader(&commonHeader, CONTROL_INFO, 0, 1, tabelleSize);
	result = send(currentSocketFD, (void*) &commonHeader, sizeof(commonHeader), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send Control Info Lcontrol Info\n");
	}
	result = send(currentSocketFD, (void*) &localBody, sizeof(localBody), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send Control Info Lcontrol Info\n");
	}
}

void notifyAllServers(){
	printf("Tabelle hat sich verändert. Sag allen bekannten Servern bescheid\n");
	int i;
	for(i=0;i<serverSize;i++){
		sendControlInfo(serverfds[i]);
	}
}

void deleteEntry(int index){
	memset(&connectionInfo[index], 0, sizeof(connectionInfo[index]));
	memcpy(&connectionInfo[index], &connectionInfo[tabelleSize - 1], sizeof(connectionInfo[index]));
	memset(&connectionInfo[tabelleSize - 1], 0, sizeof(connectionInfo[tabelleSize - 1]));

	memset(&localBody.tabelle[index], 0, sizeof(localBody.tabelle[index]));
	memcpy(&localBody.tabelle[index], &localBody.tabelle[tabelleSize - 1], sizeof(localBody.tabelle[index]));
	memset(&localBody.tabelle[tabelleSize - 1], 0, sizeof(localBody.tabelle[tabelleSize - 1]));

	tabelleSize--;
}

void commands(){
	bool exitWileLoop = false;
	char command[20];
	while (!exitWileLoop) {
		printf("Geben sie ein Befehle ein:\n");
		fgets(command, 20, stdin);
		command[strcspn(command, "\n")] = 0;
		if (strcmp(command, "/CONNECT") == 0) {
			char ipAdresse[20];
			printf("Geben Si die Ip Addresse ein:\n");
			fgets(ipAdresse, 20, stdin);
			ipAdresse[strcspn(ipAdresse, "\n")] = 0;
			connectToServer(ipAdresse);
		}else if(strcmp(command, "/CLOSE") == 0){

			exit(EXIT_SUCCESS);
		}
	}
}

void putNewServer(int currentSocketFD){
	int i;
	bool serverExist = false;
	if (serverSize > 0) {
		for (i = 0; i < serverSize; i++) {
			if (serverfds[i] == currentSocketFD) {
				serverExist = true;
				break;
			}
		}
		if (!serverExist) {
			serverfds[serverSize] = currentSocketFD;
			serverSize++;
		}
	} else {
		serverfds[serverSize] = currentSocketFD;
		serverSize++;
	}
}
