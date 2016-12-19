/*
 ============================================================================
 Name        : ClientSCTP.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "ClientSCTP.h"

int baseSocketFD;
int loginSuccess = 0;
char name[NAME_SIZE];

int main(void) {

	char ipAdresse[IP_SIZE];
	printf("Mit welchem Server wollt ihr euch verbinden\nGeben sie die Ip Addresse ein:\n");
	fgets(ipAdresse, IP_SIZE, stdin);
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
		result = recv(baseSocketFD, (void*) &commonHeader, sizeof(commonHeader), 0);
		if(result == 0){
			printf("Server ausgefallen!\nSchließe Client\n");
			close(baseSocketFD);
			exit(EXIT_SUCCESS);
		}else if (result == -1) {
			printf("ERROR on recv: Unable to receive Commonheader\n");
		}else{
			if (commonHeader.type == MESSAGE) {
				receiveMessage(commonHeader.lenght);
			}else if (commonHeader.type == CONTROL_INFO) {
				getUserNames(commonHeader.lenght);
			}else if (commonHeader.type == LOG_IN_OUT) {
				if (commonHeader.flag == (FIN | ACK)) {
					printf("Logout erfolgreich\n");
					close(baseSocketFD);
					exit(EXIT_SUCCESS);
				}else if (commonHeader.flag == (SYN | ACK)) {
					printf("Login erfolgreich\n");
					loginSuccess = 1;
				}else if(commonHeader.flag == (DUP | SYN | ACK)){
					printf("Login fehlgeschlagen: Name bereits vergeben \n");
				}
			}
		}
	}
	return NULL;
}

void logIn(char* tempName) {
	int size = strlen(tempName);
	if(size > 0){
		struct LogInOut logInOut;
		memset(&logInOut, 0, sizeof(logInOut));
		createHeader(&logInOut.commonHeader,LOG_IN_OUT,SYN,PROTOCOL_VERSION,size);

		strcpy(logInOut.logInOutBody.benutzername, tempName);

		ssize_t result = send(baseSocketFD, (void*) &logInOut.commonHeader, sizeof(logInOut.commonHeader), 0);
		if (result == -1) {
			printf("ERROR on send(): Unable to send LogInOut\n");
		}
		result = send(baseSocketFD, (void*) &logInOut.logInOutBody, size, 0);
		if (result == -1) {
			printf("ERROR on send(): Unable to send LogInOut\n");
		}
	}
}

void logOut(char* tempName){
	int size = strlen(tempName);
	if(size > 0){
		struct LogInOut logInOut;
		memset(&logInOut, 0, sizeof(logInOut));
		createHeader(&logInOut.commonHeader, LOG_IN_OUT, FIN, PROTOCOL_VERSION, size);

		strcpy(logInOut.logInOutBody.benutzername, tempName);

		ssize_t result = send(baseSocketFD, (void*) &logInOut.commonHeader,sizeof(logInOut.commonHeader), 0);
		if (result == -1) {
			printf("ERROR on send(): Unable to send LogInOut\n");
		}
		result = send(baseSocketFD, (void*) &logInOut.logInOutBody, size, 0);
		if (result == -1) {
			printf("ERROR on send(): Unable to send LogInOut\n");
		}
	}
}

void loadInfo(){
	ssize_t result;
	struct CommonHeader commonHeader;
	memset(&commonHeader,0,sizeof(commonHeader));
	createHeader(&commonHeader, CONTROL_INFO , GET, PROTOCOL_VERSION, 0);

	result = send(baseSocketFD,(void*)&commonHeader,sizeof(commonHeader),0);
	if(result == -1){
		printf("ERROR on send():Unable to send GET");
	}
}

void sendMessage(char* quelle, char* ziel, char* message){
	int size = strlen(message);
	if(size > 0){
		ssize_t result;
		struct Message messageStruct;
		memset((void*)&messageStruct,0,sizeof(messageStruct));
		createHeader(&messageStruct.commonHeader,MESSAGE,UNDEFINE,PROTOCOL_VERSION,size);


		strcpy(messageStruct.messageBody.quellbenutzername,quelle);
		strcpy(messageStruct.messageBody.zielbenutzername,ziel);
		strcpy(messageStruct.messageBody.nachricht,message);

		result = send(baseSocketFD, (void*) &messageStruct.commonHeader,sizeof(messageStruct.commonHeader), 0);
		if (result == -1) {
			printf("ERROR on send():Unable so send the Message");
		}

		result = send(baseSocketFD,(void*) &messageStruct.messageBody.quellbenutzername, MSG_NAME_SIZE, 0);
		if (result == -1) {
			printf("ERROR on send():Unable so send the Message");
		}

		result = send(baseSocketFD, (void*) &messageStruct.messageBody.zielbenutzername, MSG_NAME_SIZE, 0);
		if (result == -1) {
			printf("ERROR on send():Unable so send the Message");
		}

		result = send(baseSocketFD, (void*) &messageStruct.messageBody.nachricht, size, 0);
		if (result == -1) {
			printf("ERROR on send():Unable so send the Message");
		}
	}
}

void connectToServer(char* ipAdresse){
	int result = 0;
	struct sockaddr_in isa;
		baseSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
		if (baseSocketFD == -1) {
			printf("ERROR on socket(): Erstellung des Sockets fehlgeschlagen\n");
			exit(EXIT_SUCCESS);
		}
		memset(&isa, 0, sizeof(isa));
		isa.sin_family = AF_INET;
		isa.sin_port = htons(PORT);
		result = inet_pton(AF_INET, ipAdresse, &isa.sin_addr);
		if (result == 0) {
			printf("ERROR on inet_pton():  Not valid network address\n");
			close(baseSocketFD);
			exit(EXIT_SUCCESS);
		} else if (result == -1) {
			printf("ERROR on inet_pton(): Not valid address family\n");
			close(baseSocketFD);
			exit(EXIT_SUCCESS);
		}
		result = connect(baseSocketFD, (struct sockaddr *) &isa, sizeof(isa));

		if (result == -1) {
			perror("connect");
			printf("ERROR on connect(): Verbindung fehlgeschlagen\n");
			close(baseSocketFD);
			exit(EXIT_FAILURE);

		}else{
			printf("Verbindung zum Server hergestellt\n");
		}
}

void command(){
	int exitWileLoop = 0;
	char command[COMAND_SIZE];
	printf(" _____________________________________________________________\n");
	printf("|                                                             |\n");
	printf("| Befehle:                                                    |\n");
	printf("|                                                             |\n");
	printf("| /LOGIN  für das Einlogen auf ein Sever.                     |\n");
	printf("| /INFO   für das Anfordern der Namen aller angemeldeten User.|\n");
	printf("| /LOGOUT für das Auslogen und/oder Schließen des Clients.    |\n");
	printf("|_____________________________________________________________|\n");
	while (!exitWileLoop) {
		printf("Geben sie ein Befehle ein:\n");

		fgets(command, COMAND_SIZE, stdin);
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
			}else{
				printf("Close programm\n");
				close(baseSocketFD);
				exit(EXIT_SUCCESS);
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

void receiveMessage(int size){
	int result;
	struct MessageBody messageBody;
	memset((void*) &messageBody, 0, sizeof(messageBody));

	result = recv(baseSocketFD, (void*) &messageBody.quellbenutzername, MSG_NAME_SIZE, 0);
	if (result == -1) {
		printf("ERROR on recv():Unable to receive Message Quell Namen");
	}
	result = recv(baseSocketFD, (void*) &messageBody.zielbenutzername, MSG_NAME_SIZE, 0);
	if (result == -1) {
		printf("ERROR on recv():Unable to receive Message Ziel Namen");
	}
	if(size > 0){
		result = recv(baseSocketFD, (void*) &messageBody.nachricht, size, 0);
		if (result == -1) {
			printf("ERROR on recv():Unable to receive Message Nachricht");
		}
	}
	printf("(%s -> %s)\n Message: %s\n", messageBody.quellbenutzername,messageBody.zielbenutzername, messageBody.nachricht);
}

void getUserNames(int size){
	int result;
	struct ControlInfoBody controlInfoBody;
	memset((void*) &controlInfoBody, 0, sizeof(controlInfoBody));
	printf("Dein Name: %s\n",name);
	if(size > 0){
		result = recv(baseSocketFD, (void*) &controlInfoBody, sizeof(struct Tabelle) * size, 0); //BYTE
		if (result == -1) {
			printf("ERROR on recv():Unable to receive Tabele namen");
		} else {
			int i = 0;
			for (i = 0; i < size; i++) {
				printf("%d: Name: %s\n", i+1, controlInfoBody.tabelle[i].benutzername);
			}
		}
	}
}
