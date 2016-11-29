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

int main(void) {
	pthread_t serverId;
	pthread_create(&serverId, NULL, Server, NULL);
	pthread_join(serverId, NULL);
	return EXIT_SUCCESS;
}

void* Server(void* not_used) {
	int result;
	struct sockaddr_in isa;

	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) {
		printf("ERROR on socket: Socket erstellen fehlgeschlagen.\n");
	}
	memset((void *) &isa, 0, sizeof(isa));
	isa.sin_family = AF_INET;
	isa.sin_port = htons(PORT);
	isa.sin_addr.s_addr = inet_addr(IP);
	result = bind(socketFD, (struct sockaddr *) &isa, sizeof(isa));
	if (result < 0) {
		printf("ERROR on bind: Unable assign an address to socket.\n");
	}
	while (1) {

	}
	return NULL;
}
