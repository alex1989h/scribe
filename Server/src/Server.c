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

struct ConnectionInfo connectionInfo[100];

int main(void) {
	int result;
	struct sockaddr_in isa;

	socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (socketFD < 0) {
		perror("socket");
		printf("ERROR on socket(): Erstellung eines Sockets fehlgeschlagen.\n");
		exit(EXIT_FAILURE);
	}

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

	connectionHandling();
	pthread_join(serverId, NULL);
	return EXIT_SUCCESS;
}

void* Server(void* not_used) {

	return NULL;
}

void connectionHandling() {
	while (1) {
		struct sockaddr_in tempIsa;

		memset((void*) &tempIsa, 0, sizeof(tempIsa));
		unsigned int tempIsaSize = sizeof(tempIsa);

		int connectionFD = accept(socketFD, (struct sockaddr *) &tempIsa,
				&tempIsaSize);

		if (connectionFD < 0) {
			perror("accept");
			printf(
					"ERROR on accept: Es konnte keine Verbindung aufgebaut werden.\n");
			continue;
		} else {
			printf("Accept successful!\n");
			printf("Ip: %s\n",inet_ntoa(tempIsa.sin_addr));
			printf("Port: %i\n",ntohs(tempIsa.sin_port));
		}
	}

}
