/*
 ============================================================================
 Name        : Client.c
 Author      : Anushavan Melkonyan, Alexander Hoffmann
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Client.h"

int socketFD;

int main(void) {
	int result;
	struct sockaddr_in isa;
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD == -1) {
		printf("ERROR on socket(): Erstellung des Sockets fehlgeschlagen\n");
	}
	memset(&isa, 0, sizeof(isa));
	isa.sin_family = AF_INET;
	isa.sin_port = htons(PORT);
	result = inet_pton(AF_INET, IP, &isa.sin_addr);
	if (result == 0) {
		printf("ERROR on inet_pton():  Not valid network address\n");
	} else if (result == -1) {
		printf("ERROR on inet_pton(): Not valid address family\n");
	}
	result = connect(socketFD, (struct sockaddr *) &isa, sizeof(isa));
	if (result == -1) {
		perror("connect");
		printf("ERROR on connect(): Verbindung fehlgeschlagen\n");
		close(socketFD);
	}else{
		printf("Verbindung erfolgreich\n");
	}
	char name[NAME_SIZE];
	do {
		printf("Geben sie ihnen Benutzernamen ein: \n");
		fgets(name, NAME_SIZE, stdin);
		name[strcspn(name, "\n")] = 0;
		result = logIn(socketFD, name);
		if (result == -1) {
			printf("Name bereits vergeben \n");
		}
	} while (result < 0);
	pthread_t clientId;
	pthread_create(&clientId, NULL, Client, NULL);
	char command[20];
	while (1) {
		fgets(command,20,stdin);
		command[strcspn(command,"\n")] = 0;
		if(strcmp(command,"/INFO")==0){
			printf("Load info\n");
			loadInfo();
		}else if(strcmp(command,"/CLOSE")==0){
			printf("Close programm\n");
			closeProgram(name);
		}else if(strcmp(command,"/SEND") == 0){
			printf("Send Message\n");
			char message[255];
			char ziel[NAME_SIZE];
			printf("Gebe Zielnamen ein:\n");
			fgets(ziel,NAME_SIZE,stdin);
			ziel[strcspn(ziel, "\n")] = 0;
			printf("Gebe Nachricht ein:\n");
			fgets(message,255,stdin);
			message[strcspn(message, "\n")] = 0;
			sendMessage(name,ziel,message);
		}
	}
	pthread_join(clientId, NULL);
	return EXIT_SUCCESS;
}

void *Client(void* not_used) {
	int result;
	while (1) {
		struct CommonHeader commonHeader;
		memset(&commonHeader, 0, sizeof(commonHeader));
		result = recv(socketFD, (void*) &commonHeader, sizeof(commonHeader), 0);
		if (result == -1) {
			printf("ERROR on recv: Unable to receive Commonheader\n");
		}
		if (commonHeader.type == MESSAGE) {
			struct MessageBody messageBody;
			memset((void*) &messageBody, 0, sizeof(messageBody));
			result = recv(socketFD, (void*) &messageBody, sizeof(messageBody),
					0);
			if (result == -1) {
				printf("ERROR on recv():Unable to receive Message");
			} else {
				printf("(%s -> %s)\n Message:%s\n",
						messageBody.quellbenutzername,
						messageBody.zielbenutzername, messageBody.nachricht);
			}
		}
		if (commonHeader.type == CONTROL_INFO) {
			struct ControlInfoBody controlInfoBody;
			memset((void*) &controlInfoBody, 0, sizeof(controlInfoBody));
			result = recv(socketFD, (void*) &controlInfoBody,
					sizeof(controlInfoBody), 0);
			if (result == -1) {
				printf("ERROR on recv():Unable to receive Tabele namen");
			} else {
				int i = 0;
				for (i = 0; i < commonHeader.lenght; i++) {
					printf("%d: Name: %s\n", i,
							controlInfoBody.tabelle[i].benutzername);
				}
			}
		}
		if (commonHeader.type == LOG_IN_OUT) {
			if (commonHeader.flag == (FIN | ACK)) {
				printf("Logout\n");
				return NULL;
			}
		}
	}
	return NULL;
}

int logIn(int socketFD, char* name) {
	struct LogInOut logInOut;
	memset(&logInOut, 0, sizeof(logInOut));
	logInOut.commonHeader.type = LOG_IN_OUT;
	logInOut.commonHeader.flag = SYN;
	logInOut.commonHeader.version = 1;
	logInOut.commonHeader.lenght = NAME_SIZE;
	strcpy(&logInOut.LogInOutBody.benutzername, name);

	ssize_t result = send(socketFD, (void*) &logInOut, sizeof(logInOut), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send LogInOut\n");
	}
	memset(&logInOut, 0, sizeof(logInOut));
	result = recv(socketFD, (void*) &logInOut, sizeof(logInOut), 0);
	if (result == -1) {
		printf("ERROR on recv(): Unable to receive LogInOut\n");
	}

	if (logInOut.commonHeader.flag == (DUP | SYN | ACK)) {
		return -1;
	}

	if (logInOut.commonHeader.flag == (SYN | ACK)) {
		return 0;
	}

	return -2;
}

void loadInfo(){
	ssize_t result;
	struct CommonHeader commonHeader;
	memset(&commonHeader,0,sizeof(commonHeader));
	commonHeader.type = CONTROL_INFO;
	commonHeader.flag = GET;
	commonHeader.version = 1;
	commonHeader.lenght = 0;
	result = send(socketFD,(void*)&commonHeader,sizeof(commonHeader),0);
	if(result == -1){
		printf("ERROR on send():Unable to send GET");
	}
}

void sendMessage(char* quelle, char* ziel, char* message){
	ssize_t result;
	struct Message messageStruct;
	memset((void*)&messageStruct,0,sizeof(messageStruct));
	messageStruct.commonHeader.type = MESSAGE;
	messageStruct.commonHeader.flag = UNDEFINE;
	messageStruct.commonHeader.version = 1;
	messageStruct.commonHeader.lenght = 255;
	strcpy(&messageStruct.messageBody.quellbenutzername,quelle);
	strcpy(&messageStruct.messageBody.zielbenutzername,ziel);
	strcpy(&messageStruct.messageBody.nachricht,message);
	result = send(socketFD,(void*)&messageStruct,sizeof(messageStruct),0);
	if(result == -1){
		printf("ERROR on send():Unable so send the Message");
	}
}

void closeProgram(char* username){
	struct LogInOut logInOut;
	memset((void*)&logInOut,0,sizeof(logInOut));
	logInOut.commonHeader.type = LOG_IN_OUT;
	logInOut.commonHeader.flag = FIN;
	logInOut.commonHeader.version = 1;
	logInOut.commonHeader.lenght = NAME_SIZE;
	strcpy(&logInOut.LogInOutBody.benutzername,username);
	ssize_t result = send(socketFD,(void*)&logInOut,sizeof(logInOut),0);
	if(result == -1){
		printf("ERROR on send():Unable to logout");
	}
}
