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

int baseSocketFD;
int localSocketFD;
fd_set activefds, readfds;

int serverfds[100];
int serverSize = 0;

struct ConnectionInfo connectionInfo[255];
struct ControlInfoBody localBody;
int tabelleSize = 0;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int main(void) {
	int result;
	struct sockaddr_in isa;
	memset((void *) &localBody, 0, sizeof(localBody));
	FD_ZERO(&activefds);
	baseSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (baseSocketFD < 0) {
		perror("socket");
		printf("ERROR on socket(): Erstellung eines Sockets fehlgeschlagen.\n");
		exit(EXIT_FAILURE);
	}
	FD_SET(baseSocketFD, &activefds);
	memset((void *) &isa, 0, sizeof(isa));

	isa.sin_port = htons(PORT);

	int yes = 1;
	result = setsockopt(baseSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (result == -1) {
		perror("setsockopt");
		printf("ERROR on setsockopt: Bind funktioniert nicht.\n");
		exit(EXIT_FAILURE);
	}

	result = bind(baseSocketFD, (struct sockaddr *) &isa, sizeof(isa));
	if (result < 0) {
		perror("bind");
		printf("ERROR on bind: Bind funktioniert nicht.\n");
		exit(EXIT_FAILURE);
	}

	result = listen(baseSocketFD, 7);

	if (result < 0) {
		perror("listen");
		printf("ERROR: Es konnte kein Listing aktiviert werden \n");
		exit(EXIT_FAILURE);
	}

	pthread_t serverId;
	pthread_create(&serverId, NULL, Server, NULL);
	connectToMyself();
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
				if (i == baseSocketFD) {
					newConnection();
					break;
				} else {
					receivePackages(i);
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

	int connectionFD = accept(baseSocketFD, (struct sockaddr *) &tempIsa, &tempIsaSize);

	if (connectionFD < 0) {
		perror("accept");
		printf("ERROR on accept: Es konnte keine Verbindung aufgebaut werden.\n");
	} else {
		FD_SET(connectionFD, &activefds);
		printf("Accept successful!\n");
		printf("Ip: %s\n", inet_ntoa(tempIsa.sin_addr));
		printf("Port: %i\n", ntohs(tempIsa.sin_port));
	}
}

void receivePackages(int currentSocketFD) {
	int result;
	struct CommonHeader receivedHeader;
	memset(&receivedHeader, 0, sizeof(receivedHeader));
	result = recv(currentSocketFD, (void*) &receivedHeader,sizeof(receivedHeader), 0);

	if (result == -1) {
		printf("ERROR on recv: Unable to receive Commonheader\n");
	} else if (result == 0) {
		verbindungTrennen(currentSocketFD);
	} else {
		if (receivedHeader.type == LOG_IN_OUT) {
			if (receivedHeader.flag == (SYN)) {
				logInRequest(currentSocketFD, receivedHeader.lenght);
			} else if (receivedHeader.flag == (FIN)) {
				logOutRequest(currentSocketFD, receivedHeader.lenght);
			}
		} else if (receivedHeader.type == CONTROL_INFO) {
			if (receivedHeader.flag == GET) {
				printf("Give me all User Request\n");
				sendControlInfo(currentSocketFD, UNDEFINE, tabelleSize);
			} else {
				getControlInfo(currentSocketFD, receivedHeader.lenght);
			}
		} else if (receivedHeader.type == MESSAGE) {
			passMessage(currentSocketFD, receivedHeader.lenght);
		} else if (receivedHeader.type == CONNECT){
			printf("Verbindung zum anderem Server\n");
		} else {
			printf("Unbekannter Header Typ\n");
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

void connectToServer(char *ipAdresse){
	int result;
	struct sockaddr_in isa;
	int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocketFD == -1) {
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
			struct CommonHeader header;
			memset(&header, 0, sizeof(header));
			createHeader(&header,CONNECT,0,1,0);
			putNewServer(serverSocketFD);
			send(localSocketFD,(void*) &header, sizeof(header), 0);
			sendControlInfo(serverSocketFD, GET, 0);
			printf("Verbindung Zum Server Erfolgreich\n");
		}
	}
}

void sendControlInfo(int currentSocketFD, uint8_t flags, int size){

	int result;

	struct CommonHeader commonHeader;
	createHeader(&commonHeader, CONTROL_INFO, flags, 1 , size);
	result = send(currentSocketFD, (void*) &commonHeader, sizeof(commonHeader), 0);
	if (result == -1) {
		printf("ERROR on send(): Unable to send Control Info Lcontrol Info\n");
	}
	if (size > 0) {
		result = send(currentSocketFD, (void*) &localBody, 20 * size, 0); //20 Byte
		if (result == -1) {
			printf("ERROR on send(): Unable to send Control Info Lcontrol Info\n");
		}
	}

}

void notifyAllServers(){
	printf("Tabelle hat sich verändert. Sag allen bekannten Servern bescheid\n");
	int i;
	for(i=0;i < serverSize;i++){
		sendControlInfo(serverfds[i],UNDEFINE,tabelleSize);
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
			close(baseSocketFD);
			exit(EXIT_SUCCESS);
		}
	}
}

void deleteServer(int currentSocketFD){
	pthread_mutex_lock(&mtx);
	int i;
	for (i = 0; i < serverSize; i++) {
		if (serverfds[i] == currentSocketFD) {
			serverfds[i] = serverfds[serverSize - 1];
			serverfds[serverSize - 1] = 0;
			serverSize--;
			break;
		}
	}
	pthread_mutex_unlock(&mtx);
}

void putNewServer(int currentSocketFD){
	pthread_mutex_lock(&mtx);
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
	pthread_mutex_unlock(&mtx);
}

void logInRequest(int currentSocketFD, int size) {
	int result;
	int j;
	char tempName[15];
	bool nameExist = false;
	printf("Login Request\n");
	struct LogInOutBody logInOutBody;
	memset(&logInOutBody, 0, sizeof(logInOutBody));
	if(size > 0){
		result = recv(currentSocketFD, (void*) &logInOutBody, size, 0);
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
		struct CommonHeader logInOutHeader;
		memset(&logInOutHeader, 0, sizeof(logInOutHeader));
		if (nameExist) {
			createHeader(&logInOutHeader, LOG_IN_OUT, (DUP | SYN | ACK), 1,0);
			result = send(currentSocketFD, (void*) &logInOutHeader, sizeof(logInOutHeader), 0);
			if (result == -1) {
				printf("ERROR on send(): Unable to send LogInOut with DUB\n");
			} else {
				printf("Login fehlgeschlagen Benutzename: %s bereits vergeben\n",tempName);
			}
		} else {
			createHeader(&logInOutHeader, LOG_IN_OUT, (SYN | ACK), 1, 0);
			result = send(currentSocketFD, (void*) &logInOutHeader,sizeof(logInOutHeader), 0);
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
	}else{
		printf("Es wurde ein kein Name eingegeben\n");
	}
}

void logOutRequest(int currentSocketFD,int size){
	int result;
	int j;
	char tempName[15];
	printf("Logout Request\n");
	struct LogInOutBody logInOutBody;
	memset(&logInOutBody, 0, sizeof(logInOutBody));
	if(size > 0){
		result = recv(currentSocketFD, (void*) &logInOutBody, size ,0);
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
			result = send(currentSocketFD, (void*) &logInOut.commonHeader, sizeof(logInOut.commonHeader), 0);
			if (result == -1) {
				printf("ERROR on send(): Unable to send LogInOut Header  with DUB\n");
			}
			result = send(currentSocketFD, (void*) &logInOut.logInOutBody, 0, 0);
			if (result == -1) {
				printf("ERROR on send(): Unable to send LogInOut Body with DUB\n");
			}

			FD_CLR(currentSocketFD, &activefds);
			notifyAllServers();
		}
	}else{
		printf("Fehler.\nBeim Auslogen wurde die Größe 0 des Benutzernames übermittelt\n");
	}
}

void passMessage(int currentSocketFD, int size){
	int result;
	printf("Message request\n");
	struct Message message;
	memset(&message, 0, sizeof(message));

	createHeader(&message.commonHeader,MESSAGE,0,1,size);

	result = recv(currentSocketFD, (void*) &message.messageBody.quellbenutzername, 16,0);
	result = recv(currentSocketFD, (void*) &message.messageBody.zielbenutzername, 16,0);
	if(size > 0){
		result = recv(currentSocketFD, (void*) &message.messageBody.nachricht, size,0);
	}
	int sendSockedFD = sucheSocketFD(message.messageBody.zielbenutzername);

	if(sendSockedFD > 0){
		result = send(sendSockedFD, (void*) &message.commonHeader, sizeof(message.commonHeader),0);
		result = send(sendSockedFD, (void*) &message.messageBody.quellbenutzername, 16,0);
		result = send(sendSockedFD, (void*) &message.messageBody.zielbenutzername, 16,0);
		if(size > 0){
			result = send(sendSockedFD, (void*) &message.messageBody.nachricht, size,0);
		}
	}

}

int sucheSocketFD(char* ziehlname){
	int j;
	for (j = 0; j < tabelleSize; j++) {
		if (strcmp(connectionInfo[j].name, ziehlname) == 0) {
			return connectionInfo[j].socketFD;
		}
	}
	return 0;
}

void getControlInfo(int currentSocketFD, int size){
	int result;
	int i,j;
	bool nameExist = false;
	bool changesOnTabelle = false;
	printf("ControlInfo erhalten\n");
	putNewServer(currentSocketFD);

	//TODO:Tabellen Austauschen
	struct ControlInfoBody receivedBody;
	memset(&receivedBody, 0, sizeof(receivedBody));

	if(size > 0){
		result = recv(currentSocketFD, (void*) &receivedBody,20*size, 0);
	}

	if (result == -1) {
		printf("ERROR on recv: Unable to receive ControlInfobody\n");
	}else{
		for(i = 0; i < size; i++){
			nameExist = false;
			for(j = 0; j < tabelleSize; j++){
				if(strcmp(receivedBody.tabelle[i].benutzername,localBody.tabelle[j].benutzername) == 0){
					nameExist = true;
					if(receivedBody.tabelle[i].hops+1 < localBody.tabelle[j].hops){
						//Ersetze eintage in der localen Tabelle
						localBody.tabelle[j].hops = receivedBody.tabelle[i].hops + 1;
						connectionInfo[j].socketFD = currentSocketFD;
						connectionInfo[j].hops = receivedBody.tabelle[i].hops + 1;
						changesOnTabelle = true;
					}
					break;
				}
			}
			if(!nameExist){//Name war nicht drin also neuer Eintag
				memcpy(&localBody.tabelle[tabelleSize],&receivedBody.tabelle[i],sizeof(localBody.tabelle[tabelleSize]));
				localBody.tabelle[tabelleSize].hops++;

				strcpy(connectionInfo[tabelleSize].name,receivedBody.tabelle[i].benutzername);
				connectionInfo[tabelleSize].socketFD = currentSocketFD;
				connectionInfo[tabelleSize].hops = receivedBody.tabelle[i].hops + 1;
				tabelleSize++;
				changesOnTabelle = true;
			}
		}
		//Jetz soll geprüft werden ob irgendwelche schon gespeicherten Client vom anderen Server ausgefallen sind
		for (i = 0; i < tabelleSize; i++) {
			if (connectionInfo[i].socketFD == currentSocketFD) {
				nameExist = false;
				for(j = 0; j < size; j++){
					if(strcmp(connectionInfo[i].name,receivedBody.tabelle[j].benutzername) == 0){
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

void verbindungTrennen(int currentSocketFD){
	int i;
	bool changesOnTabelle = false;
	printf("Server oder Client ausgefallen\n");
	deleteServer(currentSocketFD);
	for (i = 0; i < tabelleSize; i++) {
		if (connectionInfo[i].socketFD == currentSocketFD) {
			deleteEntry(i);
			changesOnTabelle = true;
		}
	}

	if (changesOnTabelle) {
		notifyAllServers();
	}
	FD_CLR(currentSocketFD, &activefds);
	close(currentSocketFD);
}

void incomingConnection(){

}

void outgoingConnection(){

}

void connectToMyself(){
	int result = 0;
	struct sockaddr_in isa;
	localSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (localSocketFD == -1) {
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
	result = connect(localSocketFD, (struct sockaddr *) &isa, sizeof(isa));
	if (result == -1) {
		perror("connect");
		printf("ERROR on connect(): Verbindung fehlgeschlagen\n");
		close(localSocketFD);
		exit(EXIT_FAILURE);
	} else {
		printf("Verbindung zum sich selber hergestellt\n");
	}
}
