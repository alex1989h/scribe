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
int loginSuccess = 0;
char name[NAME_SIZE];

int main(void) {

	char ipAdresse[20];
	printf("Mit welchem Server wollt ihr euch verbinden\nGeben sie die Ip Addresse ein:\n");
	fgets(ipAdresse, NAME_SIZE, stdin);
	ipAdresse[strcspn(ipAdresse, "\n")] = 0;
	connectToServer(ipAdresse);

	pthread_t clientId;
	pthread_create(&clientId, NULL, Client, NULL);
	command();
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
			result = recv(socketFD, (void*) &messageBody, sizeof(messageBody),0);
			if (result == -1) {
				printf("ERROR on recv():Unable to receive Message");
			} else {
				printf("(%s -> %s)\n Message:%s\n",messageBody.quellbenutzername, messageBody.zielbenutzername, messageBody.nachricht);
			}
		}else if (commonHeader.type == CONTROL_INFO) {
			struct ControlInfoBody controlInfoBody;
			memset((void*) &controlInfoBody, 0, sizeof(controlInfoBody));
			result = recv(socketFD, (void*) &controlInfoBody, 20*commonHeader.lenght, 0);//BYTE
			if (result == -1) {
				printf("ERROR on recv():Unable to receive Tabele namen");
			} else {
				int i = 0;
				for (i = 0; i < commonHeader.lenght; i++) {
					printf("%d: Name: %s\n", i,controlInfoBody.tabelle[i].benutzername);
				}
			}
		}else if (commonHeader.type == LOG_IN_OUT) {
			if (commonHeader.flag == (FIN | ACK)) {
				printf("Logout erfolgreich\n");
				exit(EXIT_SUCCESS);
			}else if (commonHeader.flag == (SYN | ACK)) {
				printf("Login erfolgreich\n");
				loginSuccess = 1;
			}else if(commonHeader.flag == (DUP | SYN | ACK)){
				printf("Login fehlgeschlagen: Name bereits vergeben \n");
			}
		}
	}
	return NULL;
}

void logIn(char* tempName) {
	struct LogInOut logInOut;
	memset(&logInOut, 0, sizeof(logInOut));
	createHeader(&logInOut.commonHeader,LOG_IN_OUT,SYN,1,strlen(tempName));

	strcpy(logInOut.logInOutBody.benutzername, tempName);

	ssize_t result = send(socketFD, (void*) &logInOut.commonHeader, sizeof(logInOut.commonHeader), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send LogInOut\n");
	}
	result = send(socketFD, (void*) &logInOut.logInOutBody, strlen(tempName), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send LogInOut\n");
	}
}

void logOut(char* tempName){
	struct LogInOut logInOut;
	memset(&logInOut, 0, sizeof(logInOut));
	createHeader(&logInOut.commonHeader, LOG_IN_OUT, FIN, 1, strlen(tempName));

	strcpy(logInOut.logInOutBody.benutzername, tempName);

	ssize_t result = send(socketFD, (void*) &logInOut.commonHeader,sizeof(logInOut.commonHeader), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send LogInOut\n");
	}
	result = send(socketFD, (void*) &logInOut.logInOutBody, strlen(tempName), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send LogInOut\n");
	}
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
	strcpy(messageStruct.messageBody.quellbenutzername,quelle);
	strcpy(messageStruct.messageBody.zielbenutzername,ziel);
	strcpy(messageStruct.messageBody.nachricht,message);
	result = send(socketFD,(void*)&messageStruct,sizeof(messageStruct),0);
	if(result == -1){
		printf("ERROR on send():Unable so send the Message");
	}
}

void connectToServer(char* ipAdresse){
	int result = 0;
	struct sockaddr_in isa;
		socketFD = socket(AF_INET, SOCK_STREAM, 0);
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
		result = connect(socketFD, (struct sockaddr *) &isa, sizeof(isa));

		if (result == -1) {
			perror("connect");
			printf("ERROR on connect(): Verbindung fehlgeschlagen\n");
			close(socketFD);
			exit(EXIT_FAILURE);

		}else{
			printf("Verbindung zum Server hergestellt\n");
		}
}

void command(){
	int exitWileLoop = 0;
	char command[20];
	while (!exitWileLoop) {
		printf("Geben sie ein Befehle ein:\n");

		fgets(command, 20, stdin);
		command[strcspn(command, "\n")] = 0;
		if (strcmp(command, "/INFO") == 0) {
			if (loginSuccess) {
				printf("Load info\n");
				loadInfo();
			}
		} else if (strcmp(command, "/LOGOUT") == 0) {
			if (loginSuccess) {
				printf("Close programm\n");
				logOut(name);
				exitWileLoop = 1;
			}
		} else if (strcmp(command, "/SEND") == 0) {
			if (loginSuccess) {
				printf("Send Message\n");
				char message[255];
				char ziel[NAME_SIZE];
				printf("Gebe Zielnamen ein:\n");
				fgets(ziel, NAME_SIZE, stdin);
				ziel[strcspn(ziel, "\n")] = 0;
				printf("Gebe Nachricht ein:\n");
				fgets(message, 255, stdin);
				message[strcspn(message, "\n")] = 0;
				sendMessage(name, ziel, message);
			}
		} else if (strcmp(command, "/LOGIN") == 0) {
			if (!loginSuccess) {
				printf("Geben Sie eine Benutzernamen ein:\n");
				fgets(name, NAME_SIZE, stdin);
				name[strcspn(name, "\n")] = 0;
				logIn(name);
			}
		}
	}
}

void createHeader(struct CommonHeader* commonHeader, uint8_t type, uint8_t flag, uint8_t version, uint8_t lenght) {
	memset((void*)commonHeader, 0, sizeof(commonHeader));
	commonHeader->type = type;
	commonHeader->flag = flag;
	commonHeader->version = version;
	commonHeader->lenght = lenght;
}

void receiveMessage(int currentSocketFD){

}
